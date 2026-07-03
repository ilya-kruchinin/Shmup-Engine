# SHMUP ENGINE

**A classic Galaga-style Shoot 'Em Up (Shmup) Engine built with C++ and Raylib.**

![C++](https://img.shields.io/badge/C%2B%2B-11%2F14%2F17-blue?logo=c%2B%2B)
![Raylib](https://img.shields.io/badge/Raylib-4.x-purple?logo=raylib)

> *Created by I. Kruchinin*

## 📖 Overview
**SHMUP ENGINE** is a lightweight, single-file C++ game engine tailored for vertical scrolling arcade shooters. Inspired by classics like *Galaga* and *1942*, it features a robust entity management system, dynamic wave generation, particle effects, and classic arcade mechanics—all rendered using the powerful [Raylib](https://www.raylib.com/) library.

Whether you want to play a nostalgic arcade shooter or use this as a foundational template for your own 2D space shooter, this engine provides a clean, extensible architecture.

---

## ✨ Key Features

### 🎮 Gameplay Mechanics
- **Classic Arcade Action**: 3 lives, invincibility frames (with blinking effect), and restricted bullet limits (max 3 on screen) for strategic gameplay.
- **Dynamic Wave System**: Endless waves with scaling difficulty. Enemy count, movement speed, fire rate, and swoop probability increase as you progress.
- **Boss Fights**: Every 10 waves, a massive Boss spawns with scaling HP and devastating 5-way spread shot patterns.
- **Score Multipliers**: Destroy enemies while they are in a **Swooping** dive to earn **double points**!

### 👾 Enemy AI & Behaviors
- **Formation Movement**: Grid-based enemies that march side-to-side and step down upon hitting screen edges.
- **Swooping Dives**: Enemies can break formation to dive-bomb the player with aimed trajectories.
- **4 Enemy Classes**:
  - 🟩 **Grunt** (100 pts) - Bottom rows, standard fodder.
  - 🟨 **Fighter** (200 pts) - Middle rows, faster shooters.
  - 🟥 **Leader** (400 pts) - Top row, elite enemies.
  - 🟪 **Boss** (2000 pts) - Massive HP, complex movement patterns, and bullet hell spreads.

### 🎨 Visuals & Juice
- **Procedural Vector Graphics**: Ships and enemies are drawn using geometric primitives (triangles, polygons) with pulsing animations.
- **Particle System**: Custom explosion particles with velocity, gravity, and alpha-fade lifecycle.
- **Parallax Starfield**: Dynamic, randomized background stars to simulate deep-space travel.
- **Engine Thrusters**: Flickering animated engine flames on the player ship.

### 🏗️ Engine Architecture
- **Finite State Machine (FSM)**: Clean handling of `TITLE`, `PLAYING`, and `GAMEOVER` states.
- **Entity-Component Style**: Lightweight `std::vector` based entity pooling for Bullets, Enemies, and Particles with automatic cleanup of inactive objects.
- **Collision Detection**: Fast AABB (Axis-Aligned Bounding Box) rectangle collision checks.

---

## 🕹️ Controls

| Action | Keys |
| :--- | :--- |
| **Move Left / Right** | `←` / `→` Arrow Keys **OR** `A` / `D` |
| **Shoot** | `SPACE` **OR** `↑` **OR** `W` |
| **Start / Confirm** | `SPACE` **OR** `ENTER` |

---

## 🛠️ Requirements & Compilation

### Prerequisites
- A C++ compiler (GCC, Clang, or MSVC) supporting C++11 or later.
- [Raylib](https://www.raylib.com/) (v4.0 or higher recommended).

### Building on Linux / macOS
If you have Raylib installed system-wide, you can compile the single-file engine directly from your terminal:

```bash
g++ -o shmup Shmup.cpp -lraylib -lm -lpthread -ldl
./shmup
