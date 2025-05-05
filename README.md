# Space Shooter

A 2D space shooter game built with C++ and SFML.

![Gameplay](screenshots/gameplay.gif)

## Features

- Player-controlled ship with movement and shooting.
- Multiple enemy types:
  - Standard (white)
  - Fast scouts (green)
  - Tank (red)
  - Zigzag movement (blue)
- Power-ups:
  - Spread Shot (yellow)
  - Rapid Fire (red)
  - Shield (blue)
- Wave progression system.
- Score tracking.
- Basic particle effects and screen shake.

## Requirements

- C++ compiler
- SFML 2.6.2 or later
- Sprites: `player.png`, `enemy.png`, `bullet.png`, `powerup.png`

## Build Instructions (macOS)

```bash
brew install sfml
g++ main.cpp -o space_shooter -lsfml-graphics -lsfml-window -lsfml-system
./space_shooter
