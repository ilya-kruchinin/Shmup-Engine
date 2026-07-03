// main.cpp — Galaga-style Shoot 'Em Up (Raylib)
#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ──────────────────────────── Constants ────────────────────────────
const int SCREEN_W = 600;
const int SCREEN_H = 800;
const int PLAYER_SPEED = 350;
const float PLAYER_BULLET_SPEED = -600.0f;
const float ENEMY_BULLET_SPEED = 280.0f;
const int MAX_PLAYER_BULLETS = 3;
const int PLAYER_LIVES = 3;
const float INVINCIBILITY_TIME = 2.0f;

// ──────────────────────────── Types ────────────────────────────────
enum class GameState { TITLE, PLAYING, GAMEOVER };
enum class EnemyType : int {
    GRUNT = 0,   // bottom rows — 100 pts
    FIGHTER = 1, // middle rows — 200 pts
    LEADER = 2,  // top row     — 400 pts
    BOSS = 3     // every 10 waves — 2000 pts
};

struct Vec2 { float x, y; };

struct Bullet {
    Rectangle rect;
    float vy;
    float vx = 0.0f;
    bool active = false;
    Color color = WHITE;
};

struct Particle {
    Vec2 pos;
    Vec2 vel;
    Color color;
    float life;
    float maxLife;
};

struct Star {
    float x, y, speed, brightness;
};

struct Enemy {
    Rectangle rect;
    EnemyType type;
    bool active = false;
    int hp = 1;
    float baseX, baseY;          // formation anchor
    float animTimer = 0.0f;
    bool swooping = false;
    float swoopPhase = 0.0f;
    Vec2 swoopVel = { 0, 0 };
};

struct Player {
    Rectangle rect;
    int lives = PLAYER_LIVES;
    float invTimer = 0.0f;
    bool alive = true;
    float shootCooldown = 0.0f;
};

// ──────────────────────────── Globals ──────────────────────────────
GameState state = GameState::TITLE;
Player player;
std::vector<Enemy> enemies;
std::vector<Bullet> playerBullets;
std::vector<Bullet> enemyBullets;
std::vector<Particle> particles;
std::vector<Star> stars;

int score = 0;
int highScore = 0;
int wave = 1;
float enemyDir = 1.0f;
float enemyMoveTimer = 0.0f;
float enemyMoveInterval = 1.2f;
float enemyShootTimer = 0.0f;
float waveDelayTimer = 0.0f;
bool waveCleared = false;
float globalTime = 0.0f;

// ──────────────────────────── Helpers ──────────────────────────────
Color EnemyColor(EnemyType t) {
    switch (t) {
    case EnemyType::GRUNT:   return GREEN;
    case EnemyType::FIGHTER: return YELLOW;
    case EnemyType::LEADER:  return RED;
    case EnemyType::BOSS:    return PURPLE;
    }
    return WHITE;
}

int EnemyScore(EnemyType t) {
    switch (t) {
    case EnemyType::GRUNT:   return 100;
    case EnemyType::FIGHTER: return 200;
    case EnemyType::LEADER:  return 400;
    case EnemyType::BOSS:    return 2000;
    }
    return 0;
}

void SpawnExplosion(float cx, float cy, Color col, int count = 16) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)i / count * PI * 2.0f + ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        float speed = 80.0f + (float)rand() / RAND_MAX * 200.0f;
        Particle p;
        p.pos = { cx, cy };
        p.vel = { cosf(angle) * speed, sinf(angle) * speed };
        p.color = col;
        p.life = p.maxLife = 0.4f + (float)rand() / RAND_MAX * 0.5f;
        particles.push_back(p);
    }
}

void InitStars() {
    stars.clear();
    for (int i = 0; i < 120; ++i) {
        Star s;
        s.x = (float)rand() / RAND_MAX * SCREEN_W;
        s.y = (float)rand() / RAND_MAX * SCREEN_H;
        s.speed = 30.0f + (float)rand() / RAND_MAX * 120.0f;
        s.brightness = 0.3f + (float)rand() / RAND_MAX * 0.7f;
        stars.push_back(s);
    }
}

void ResetPlayer() {
    player.rect = { SCREEN_W / 2.0f - 16, (float)SCREEN_H - 80, 32, 32 };
    player.lives = PLAYER_LIVES;
    player.invTimer = 0.0f;
    player.alive = true;
    player.shootCooldown = 0.0f;
}

void SpawnWave(int w) {
    enemies.clear();
    enemyBullets.clear();
    waveCleared = false;

    int rows = std::min(3 + w / 2, 7);
    int cols = std::min(6 + w / 3, 10);
    float spacingX = 50.0f;
    float spacingY = 46.0f;
    float startX = (SCREEN_W - (cols - 1) * spacingX) / 2.0f;
    float startY = 60.0f;

    for (int r = 0; r < rows; ++r) {
        EnemyType type;
        if (r == 0)           type = EnemyType::LEADER;
        else if (r <= 2)      type = EnemyType::FIGHTER;
        else                  type = EnemyType::GRUNT;

        for (int c = 0; c < cols; ++c) {
            Enemy e;
            e.baseX = startX + c * spacingX;
            e.baseY = startY + r * spacingY;
            e.rect = { e.baseX - 16, e.baseY - 16, 32, 32 };
            e.type = type;
            e.active = true;
            e.hp = 1;
            e.animTimer = (float)rand() / RAND_MAX * 6.28f;
            e.swooping = false;
            enemies.push_back(e);
        }
    }

    // Spawn Boss every 10 waves
    if (w % 10 == 0) {
        Enemy boss;
        boss.baseX = SCREEN_W / 2.0f;
        boss.baseY = 100.0f;
        boss.rect = { boss.baseX - 48, boss.baseY - 48, 96, 96 };
        boss.type = EnemyType::BOSS;
        boss.active = true;
        boss.hp = 10 + w; // Scale HP with wave
        boss.animTimer = 0.0f;
        boss.swooping = false;
        enemies.push_back(boss);
    }

    enemyDir = 1.0f;
    enemyMoveInterval = std::max(0.35f, 1.2f - w * 0.08f);
    enemyMoveTimer = 0.0f;
    enemyShootTimer = 0.0f;
}

void ResetGame() {
    score = 0;
    wave = 1;
    playerBullets.clear();
    enemyBullets.clear();
    particles.clear();
    ResetPlayer();
    SpawnWave(wave);
    state = GameState::PLAYING;
}

// ──────────────────────────── Init ─────────────────────────────────
void Init() {
    InitStars();
    ResetPlayer();
}

// ──────────────────────────── Update ───────────────────────────────
void Update(float dt) {
    globalTime += dt;

    // ── Stars ──
    for (auto& s : stars) {
        s.y += s.speed * dt;
        if (s.y > SCREEN_H) { s.y = 0; s.x = (float)rand() / RAND_MAX * SCREEN_W; }
    }

    // ── Particles ──
    for (auto& p : particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0; }), particles.end());

    if (state == GameState::TITLE) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) ResetGame();
        return;
    }

    if (state == GameState::GAMEOVER) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) { state = GameState::TITLE; }
        return;
    }

    // ── Wave transition ──
    if (waveCleared) {
        waveDelayTimer -= dt;
        if (waveDelayTimer <= 0.0f) {
            ++wave;
            SpawnWave(wave);
        }
        // still update player movement during delay
    }

    // ── Player Movement ──
    if (player.lives > 0) { // Allows movement & shooting while invincible
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.rect.x -= PLAYER_SPEED * dt;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.rect.x += PLAYER_SPEED * dt;
        player.rect.x = std::max(0.0f, std::min((float)SCREEN_W - player.rect.width, player.rect.x));

        player.shootCooldown -= dt;
        if ((IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) && player.shootCooldown <= 0.0f) {
            int activeCount = 0;
            for (auto& b : playerBullets) if (b.active) activeCount++;
            if (activeCount < MAX_PLAYER_BULLETS) {
                Bullet b;
                b.rect = { player.rect.x + player.rect.width / 2 - 3, player.rect.y - 4, 6, 14 };
                b.vy = PLAYER_BULLET_SPEED;
                b.active = true;
                b.color = WHITE;
                playerBullets.push_back(b);
                player.shootCooldown = 0.18f;
            }
        }
    }

    // ── Invincibility ──
    if (player.invTimer > 0) {
        player.invTimer -= dt;
        if (player.invTimer <= 0 && player.lives > 0) { player.alive = true; }
    }

    // ── Player Bullets ──
    for (auto& b : playerBullets) {
        if (!b.active) continue;
        b.rect.x += b.vx * dt;
        b.rect.y += b.vy * dt;
        if (b.rect.y + b.rect.height < 0 || b.rect.x < -20 || b.rect.x > SCREEN_W + 20) b.active = false;
    }

    // ── Enemy Formation Movement ──
    if (!waveCleared) {
        enemyMoveTimer += dt;
        if (enemyMoveTimer >= enemyMoveInterval) {
            enemyMoveTimer = 0;
            // shift formation
            float shift = 18.0f * enemyDir;
            bool edgeHit = false;
            for (auto& e : enemies) {
                if (!e.active || e.swooping || e.type == EnemyType::BOSS) continue;
                float nx = e.baseX + shift;
                if (nx < 30 || nx > SCREEN_W - 30) edgeHit = true;
            }
            if (edgeHit) {
                enemyDir *= -1.0f;
                shift = 0;
                for (auto& e : enemies) {
                    if (!e.active || e.swooping || e.type == EnemyType::BOSS) continue;
                    e.baseY += 12.0f;
                }
            }
            for (auto& e : enemies) {
                if (!e.active || e.swooping || e.type == EnemyType::BOSS) continue;
                e.baseX += shift;
            }
        }

        // update non-swooping enemy positions
        for (auto& e : enemies) {
            if (!e.active) continue;
            e.animTimer += dt * 3.0f;
            if (!e.swooping) {
                if (e.type == EnemyType::BOSS) {
                    e.rect.x = e.baseX - e.rect.width / 2 + sinf(e.animTimer) * 150.0f;
                    e.rect.y = e.baseY - e.rect.height / 2 + cosf(e.animTimer * 0.5f) * 30.0f;
                }
                else {
                    e.rect.x = e.baseX - e.rect.width / 2 + sinf(e.animTimer) * 4.0f;
                    e.rect.y = e.baseY - e.rect.height / 2;
                }
            }
        }

        // ── Enemy Swooping ──
        float swoopChance = 0.002f + wave * 0.001f;
        for (auto& e : enemies) {
            if (!e.active || e.swooping || e.type == EnemyType::BOSS) continue;
            if ((float)rand() / RAND_MAX < swoopChance * dt * 60.0f) {
                e.swooping = true;
                e.swoopPhase = 0;
                // aim loosely at player
                float dx = (player.rect.x + 16) - (e.rect.x + 16);
                float dy = (player.rect.y + 16) - (e.rect.y + 16);
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 1) dist = 1;
                float spd = 200.0f + wave * 15.0f;
                e.swoopVel = { dx / dist * spd, dy / dist * spd };
            }
        }

        for (auto& e : enemies) {
            if (!e.active || !e.swooping) continue;
            e.swoopPhase += dt;
            e.rect.x += e.swoopVel.x * dt;
            e.rect.y += e.swoopVel.y * dt;
            // if off screen, loop back to formation
            if (e.rect.y > SCREEN_H + 40 || e.rect.x < -60 || e.rect.x > SCREEN_W + 60) {
                e.swooping = false;
                e.baseY = std::min(e.baseY, (float)SCREEN_H - 200); // don't go too low
            }
        }

        // ── Enemy Shooting ──
        enemyShootTimer -= dt;
        if (enemyShootTimer <= 0.0f) {
            enemyShootTimer = std::max(0.3f, 1.5f - wave * 0.1f);
            // pick random active enemy
            std::vector<int> indices;
            for (int i = 0; i < (int)enemies.size(); ++i)
                if (enemies[i].active) indices.push_back(i);

            if (!indices.empty()) {
                int idx = indices[rand() % indices.size()];
                Enemy& e = enemies[idx];

                if (e.type == EnemyType::BOSS) {
                    // Boss shoots a spread of bullets
                    for (int i = -2; i <= 2; ++i) {
                        Bullet b;
                        b.rect = { e.rect.x + e.rect.width / 2 - 4, e.rect.y + e.rect.height, 8, 16 };
                        b.vy = ENEMY_BULLET_SPEED + wave * 10.0f;
                        b.vx = i * 60.0f;
                        b.active = true;
                        b.color = RED;
                        enemyBullets.push_back(b);
                    }
                }
                else {
                    Bullet b;
                    b.rect = { e.rect.x + e.rect.width / 2 - 3, e.rect.y + e.rect.height, 6, 12 };
                    b.vy = ENEMY_BULLET_SPEED + wave * 10.0f;
                    b.active = true;
                    b.color = MAGENTA;
                    enemyBullets.push_back(b);
                }
            }
        }
    }

    // ── Enemy Bullets ──
    for (auto& b : enemyBullets) {
        if (!b.active) continue;
        b.rect.x += b.vx * dt;
        b.rect.y += b.vy * dt;
        if (b.rect.y > SCREEN_H || b.rect.x < -20 || b.rect.x > SCREEN_W + 20) b.active = false;
    }

    // ── Collision: Player Bullets → Enemies ──
    for (auto& b : playerBullets) {
        if (!b.active) continue;
        for (auto& e : enemies) {
            if (!e.active) continue;
            if (CheckCollisionRecs(b.rect, e.rect)) {
                b.active = false;
                e.hp--;
                if (e.hp <= 0) {
                    e.active = false;
                    score += EnemyScore(e.type) * (e.swooping ? 2 : 1);
                    SpawnExplosion(e.rect.x + e.rect.width / 2.0f, e.rect.y + e.rect.height / 2.0f, EnemyColor(e.type), e.type == EnemyType::BOSS ? 50 : 20);
                }
                else {
                    // Hit but not destroyed
                    SpawnExplosion(b.rect.x, b.rect.y, WHITE, 5);
                }
                break;
            }
        }
    }

    // ── Collision: Enemy Bullets → Player ──
    if (player.alive && player.invTimer <= 0) {
        for (auto& b : enemyBullets) {
            if (!b.active) continue;
            if (CheckCollisionRecs(b.rect, player.rect)) {
                b.active = false;
                player.lives--;
                SpawnExplosion(player.rect.x + 16, player.rect.y + 16, WHITE, 24);
                if (player.lives <= 0) {
                    player.alive = false;
                    if (score > highScore) highScore = score;
                    state = GameState::GAMEOVER;
                }
                else {
                    player.invTimer = INVINCIBILITY_TIME;
                    player.alive = false; // will be set true after invTimer
                }
                break;
            }
        }
    }

    // ── Collision: Swooping Enemies → Player ──
    if (player.alive && player.invTimer <= 0) {
        for (auto& e : enemies) {
            if (!e.active) continue;
            if (CheckCollisionRecs(e.rect, player.rect)) {
                e.active = false;
                SpawnExplosion(e.rect.x + 16, e.rect.y + 16, EnemyColor(e.type), 20);
                player.lives--;
                SpawnExplosion(player.rect.x + 16, player.rect.y + 16, WHITE, 24);
                if (player.lives <= 0) {
                    player.alive = false;
                    if (score > highScore) highScore = score;
                    state = GameState::GAMEOVER;
                }
                else {
                    player.invTimer = INVINCIBILITY_TIME;
                    player.alive = false;
                }
                break;
            }
        }
    }

    // ── Enemies reaching bottom (Wrap around instead of Game Over) ──
    for (auto& e : enemies) {
        if (e.active && !e.swooping && e.rect.y > SCREEN_H) {
            if (e.type == EnemyType::BOSS) {
                e.baseY = 100.0f;
            }
            else {
                e.baseY = 60.0f;
            }
            e.rect.y = e.baseY - e.rect.height / 2;
        }
    }

    // ── Cleanup inactive bullets ──
    playerBullets.erase(std::remove_if(playerBullets.begin(), playerBullets.end(),
        [](const Bullet& b) { return !b.active; }), playerBullets.end());
    enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(),
        [](const Bullet& b) { return !b.active; }), enemyBullets.end());

    // ── Check wave clear ──
    if (!waveCleared) {
        bool anyAlive = false;
        for (auto& e : enemies) if (e.active) { anyAlive = true; break; }
        if (!anyAlive) {
            waveCleared = true;
            waveDelayTimer = 2.0f;
        }
    }
}

// ──────────────────────────── Draw ─────────────────────────────────
void DrawShip(float cx, float cy, float w, float h, Color col) {
    // Fuselage triangle
    Vector2 tip = { cx, cy - h / 2 };
    Vector2 bl = { cx - w / 2, cy + h / 2 };
    Vector2 br = { cx + w / 2, cy + h / 2 };
    DrawTriangle(tip, bl, br, col);
    // Cockpit
    DrawCircle((int)cx, (int)cy, 4, WHITE);
    // Wings
    DrawRectangle((int)(cx - w / 2 - 4), (int)(cy + 2), 6, 10, col);
    DrawRectangle((int)(cx + w / 2 - 2), (int)(cy + 2), 6, 10, col);
    // Engine glow
    Color flame = (int)(globalTime * 15) % 2 == 0 ? ORANGE : YELLOW;
    DrawRectangle((int)(cx - 3), (int)(cy + h / 2), 6, 6 + rand() % 4, flame);
}

void DrawEnemy(float cx, float cy, float w, float h, EnemyType type, float anim) {
    Color col = EnemyColor(type);
    float pulse = 1.0f + sinf(anim * 4.0f) * 0.1f;
    float pw = w * pulse, ph = h * pulse;
    switch (type) {
    case EnemyType::GRUNT:
        DrawRectangle((int)(cx - pw / 2), (int)(cy - ph / 2), (int)pw, (int)ph, col);
        DrawRectangle((int)(cx - pw / 4), (int)(cy - ph / 4), (int)(pw / 2), (int)(ph / 2), DARKGREEN);
        break;
    case EnemyType::FIGHTER:
        DrawPoly({ cx, cy }, 6, pw / 2, 0, col);
        DrawCircle((int)cx, (int)cy, 5, ORANGE);
        break;
    case EnemyType::LEADER:
        DrawPoly({ cx, cy }, 5, pw / 2, 0, col);
        DrawPoly({ cx, cy }, 5, pw / 4, PI, WHITE);
        break;
    case EnemyType::BOSS:
        DrawPoly({ cx, cy }, 8, pw / 2, anim, PURPLE);
        DrawPoly({ cx, cy }, 8, pw / 3, anim + PI / 8, VIOLET);
        DrawCircle((int)cx, (int)cy, pw / 6, RED);
        break;
    }
}

void Draw() {
    BeginDrawing();
    ClearBackground({ 5, 5, 20, 255 });

    // ── Stars ──
    for (auto& s : stars) {
        unsigned char b = (unsigned char)(s.brightness * 255);
        DrawPixel((int)s.x, (int)s.y, { b, b, b, 255 });
    }

    if (state == GameState::TITLE) {
        DrawText("SHMUP ENGINE", SCREEN_W / 2 - 220, 200, 64, RED);
        DrawText("C++ , By I. Kruchinin", SCREEN_W / 2 - 95, 270, 24, WHITE);
        DrawText("ARROW KEYS / WASD  to move", SCREEN_W / 2 - 175, 380, 20, LIGHTGRAY);
        DrawText("SPACE / UP / W  to shoot", SCREEN_W / 2 - 165, 415, 20, LIGHTGRAY);
        if ((int)(globalTime * 2) % 2 == 0)
            DrawText("PRESS SPACE TO START", SCREEN_W / 2 - 160, 520, 24, YELLOW);
        if (highScore > 0) {
            DrawText(TextFormat("HIGH SCORE: %d", highScore), SCREEN_W / 2 - 110, 600, 22, GOLD);
        }
        EndDrawing();
        return;
    }

    // ── HUD ──
    DrawText(TextFormat("SCORE: %d", score), 16, 12, 22, WHITE);
    DrawText(TextFormat("WAVE %d", wave), SCREEN_W / 2 - 45, 12, 22, YELLOW);

    // Lives
    for (int i = 0; i < player.lives; ++i) {
        DrawShip(SCREEN_W - 30 - i * 36, 24, 18, 18, SKYBLUE);
    }

    if (waveCleared) {
        DrawText("WAVE CLEAR!", SCREEN_W / 2 - 120, SCREEN_H / 2 - 20, 40, GREEN);
        EndDrawing();
        // still draw remaining particles below...
        goto drawParticles;
    }

    // ── Enemies ──
    for (auto& e : enemies) {
        if (!e.active) continue;
        DrawEnemy(e.rect.x + e.rect.width / 2.0f, e.rect.y + e.rect.height / 2.0f, e.rect.width, e.rect.height, e.type, e.animTimer);
    }

    // ── Player ──
    if (player.alive || player.invTimer > 0) {
        bool blink = ((int)(globalTime * 10) % 2 == 0);
        if (player.alive || blink) {
            DrawShip(player.rect.x + 16, player.rect.y + 16, 32, 32, SKYBLUE);
        }
    }

    // ── Player Bullets ──
    for (auto& b : playerBullets) {
        if (!b.active) continue;
        DrawRectangleRec(b.rect, WHITE);
        DrawRectangle((int)b.rect.x - 1, (int)b.rect.y, (int)b.rect.width + 2, (int)b.rect.height, { 100, 255, 255, 120 });
    }

    // ── Enemy Bullets ──
    for (auto& b : enemyBullets) {
        if (!b.active) continue;
        DrawRectangleRec(b.rect, b.color);
        DrawCircle((int)(b.rect.x + b.rect.width / 2), (int)(b.rect.y + b.rect.height / 2), b.rect.width / 2 + 2, { b.color.r, b.color.g, b.color.b, 150 });
    }

drawParticles:
    // ── Particles ──
    for (auto& p : particles) {
        float alpha = p.life / p.maxLife;
        Color c = p.color;
        c.a = (unsigned char)(alpha * 255);
        int sz = (int)(3.0f * alpha + 1);
        DrawRectangle((int)p.pos.x, (int)p.pos.y, sz, sz, c);
    }

    // ── Game Over Overlay ──
    if (state == GameState::GAMEOVER) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, { 0, 0, 0, 160 });
        DrawText("GAME OVER", SCREEN_W / 2 - 140, SCREEN_H / 2 - 80, 48, RED);
        DrawText(TextFormat("FINAL SCORE: %d", score), SCREEN_W / 2 - 135, SCREEN_H / 2 - 10, 28, WHITE);
        if (score >= highScore && score > 0)
            DrawText("NEW HIGH SCORE!", SCREEN_W / 2 - 130, SCREEN_H / 2 + 30, 24, GOLD);
        if ((int)(globalTime * 2) % 2 == 0)
            DrawText("PRESS SPACE", SCREEN_W / 2 - 90, SCREEN_H / 2 + 80, 24, YELLOW);
    }

    EndDrawing();
}

// ──────────────────────────── Main ─────────────────────────────────
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "ShootEmUp_Engine by I. Kruchinin");
    SetTargetFPS(60);
    srand((unsigned)GetTime());
    Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Update(dt);
        Draw();
    }

    CloseWindow();
    return 0;
}
