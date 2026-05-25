# Heroes of Taboo — A Roguelike Adventure

> **Current Version:** ALPHA-0.0.4 (Music Update)

Turn-based roguelike dungeon crawler built with C and raylib. Descend into procedurally generated dungeons, battle monsters, level up, and try to survive.

---

## Gameplay

Each turn you move, attack, or wait. Enemies act after you. Clear every monster in the dungeon to win — die and it's game over.

- **Move / Attack:** Arrow keys or WASD
- **Wait (rest 1 HP):** Period (`.`) or Space
- **Pause / Menu:** Escape
- **Restart:** R

## Features

- **Procedural Dungeons** — Branching corridors and rooms, every run is different
- **3 Monster Types** — Floating Eye, Fungal Myconid, and Ogre, each with unique stats and animated sprites
- **Line-of-Sight AI** — Monsters detect, chase, and attack using Bresenham pathfinding
- **Leveling System** — Gain EXP from kills, level up with +5 HP, +1 ATK, +1 DEF
- **Fog of War** — 7-tile circular visibility radius, dimmed previously-seen areas
- **Healing Pickups** — Green cross items restore 8 HP, placed in larger rooms
- **Combat Log** — On-screen log of all combat events
- **Smooth Movement** — Interpolated tile-to-tile animations (0.15s)
- **Dynamic Camera** — Follows the player with 4x zoom
- **Resizable Window** — 1024x768 minimum

## Audio

- **5 Game Music Tracks** — Randomly shuffled, seamless transitions (OGG)
- **Menu Music** — Dedicated track on the main menu
- **Sound Effects** — 10 hit sounds, 4 pickup sounds
- **Volume Controls** — Independent music and SFX sliders

## Monsters

| Monster | HP | ATK | DEF | EXP | Spawn Rate |
|---------|----|-----|-----|-----|------------|
| Floating Eye | 6 | 3 | 0 | 5 | 40% |
| Fungal Myconid | 10 | 4 | 1 | 10 | 35% |
| Ogre | 20 | 4 | 1 | 25 | 25% |

## Controls

| Action | Key |
|--------|-----|
| Move / Attack Up | Up Arrow / W |
| Move / Attack Down | Down Arrow / S |
| Move / Attack Left | Left Arrow / A |
| Move / Attack Right | Right Arrow / D |
| Wait (rest 1 HP) | Period (.) / Space |
| Pause / Game Menu | Escape |
| Restart | R |

## Credits

**Development:** Greddode  
**Tileset:** Artisan Of Artifacts — VelmoraRealms  
**Player Sprite:** Kenney  
**Music:** Tallbeard Studios  
**Sound Effects:** Dabolka  
**Engine:** raylib  

---

*Built with C17 and raylib. Cross-platform (Windows, Linux, macOS).*
