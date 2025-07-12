# GEMINI - Your AI Development Partner for TuxArena

Welcome to the TuxArena project! Your primary mission is to help build the most awesome 90s-style top-down shooter imaginable. This document outlines the core vision, architecture, and your role in making it a reality.

**Our Philosophy:**
* **90s Style, Modern Power:** Capture the spirit of classic top-down shooters like *Smash TV* and *Gauntlet*, but with modern, robust engineering.
* **Extreme Robustness & Scalability:** The game must *never* fail to run. Implement intelligent fallbacks for every possible missing component, from assets to maps. The architecture must be scalable to support extensive community modding.
* **Open Source Mayhem:** The theme is a deathmatch of open-source mascots. Embrace the fun, chaotic energy of seeing Tux, the GIMP logo, the Rust gear, and others battle it out.
* **Modding is a Feature, Not an Afterthought:** Design every system with modding as a first-class citizen. The community should be able to add characters, weapons, maps, and even new game logic with ease.

---

## Core Development Directives

Your task is to extend the existing C++/SDL codebase. Analyze `CMakeLists.txt` and the `src` directory to understand the current foundation. Your new contributions must adhere to these directives.

### 1. Game Concept: Open Source Mascot Deathmatch

* **Theme:** The game is a fast-paced, arena-based deathmatch.
* **Characters:** The fighters are iconic mascots from the open-source world.
    * **Existing Assets:** Use all PNG files found in the `assets/` directory as playable characters.
    * **Examples:** Tux (Linux), Ferris (Rust), a GIMP logo, the Python snake, a C language block, etc.
    * **Implementation:** The `CharacterManager.cpp` should be responsible for loading character definitions. Each character should have stats (health, speed) and potentially a unique passive or active ability defined in a simple configuration file (e.g., JSON).

### 2. Asset System: Prioritize Extreme Robustness

This is a **critical directive**. The game must always have a visual representation for any entity.

* **Primary Asset Source:** Load character, weapon, and projectile sprites from `.png` files in the `assets/` directory.
* **Extreme Fallback - Procedural Generation:** If a required `.png` asset is **not found**, you **must** procedurally generate a placeholder using vector graphics at runtime. This prevents crashes and ensures the game is always playable.
    * **Characters:** Generate a simple, representative vector shape. For example:
        * If `rust_mascot.png` is missing, render a simple orange gear.
        * If `python_logo.png` is missing, render a two-toned, intertwined snake-like shape.
        * If `gimp_logo.png` is missing, render a coyote-like head with a paintbrush in its mouth.
    * **Weapons/Projectiles:** Generate basic geometric shapes (e.g., a simple rectangle for a pistol, a circle for a basic bullet).
    * **Implementation:** This logic should be integrated into the `Renderer.cpp` or a new `AssetManager.cpp` class. The goal is visual functionality, not high art.

---

## Technical Architecture & Implementation

### 3. Weapons System: Doom-Style Slots

Implement a weapon management system inspired by classic shooters like Doom and Quake.

* **Weapon Slots:** The player can carry multiple weapons, accessible via number keys (1-9).
    * **Slot 1:** Melee weapon (e.g., a sword, a rusty pipe).
    * **Slot 2:** Basic pistol.
    * **Slot 3:** Shotgun-type weapon.
    * **Slot 4:** Machine gun/rapid-fire weapon.
    * **And so on...**
* **HUD:** The UI should display the available weapon slots and the currently selected weapon.
* **Implementation:**
    * `Weapon.cpp`: Define a base `Weapon` class.
    * Create subclasses for different weapon types (`Pistol`, `Shotgun`, etc.) that inherit from `Weapon` and define behavior like fire rate, projectile type, and spread.
    * `Player.cpp`: Manage the player's weapon inventory and handle input for weapon switching.

### 4. Map System: Tiled Map Support

The game must use maps created with the Tiled Map Editor.

* **Map Format:** Load maps from `.tmx` files located in the `maps/` directory.
* **Library:** Use the `tmxlite` library, which is already configured in the `CMakeLists.txt`.
* **Implementation:**
    * `MapManager.cpp`: This class will be responsible for loading, parsing, and managing the current map. It should handle tile layers, object layers (for spawn points, item locations), and collision data.
* **Fallback Directive:** If the `maps/` directory is empty or contains no valid `.tmx` files, the `MapManager` must generate a simple, procedural default arena (e.g., a rectangular room with a few randomly placed pillars) to ensure a level is always available.

### 5. Modding Support: The Key to Longevity

Design a powerful and easy-to-use modding system.

* **Mod Directory:** All mods will be loaded from the `mods/` directory. Each subdirectory within `mods/` is a separate mod.
* **Mod Manager:**
    * `ModManager.cpp`: This class will scan the `mods/` directory on startup.
    * It should load new **characters, weapons, and maps** from mods.
    * It must handle **asset overriding**. If a mod provides `assets/tux_alternative.png`, it can be selected as a new skin or character.
* **Data-Driven Design:** Define clear, simple data formats for game content. **JSON is preferred.**
    * **`character.json`:** Defines a character's name, asset file, health, speed, and special ability.
    * **`weapon.json`:** Defines a weapon's name, asset file, fire rate, damage, projectile type, and sound effect.
* **Scripting (Future Goal):** Plan for the integration of a scripting language like Lua to allow modders to implement complex custom abilities and game logic.

By following these instructions, you will create **TuxArena**, a game that is not only fun and chaotic but also incredibly robust, scalable, and a tribute to the open-source community. Let the deathmatch begin!