# TuxArena Architecture

## 1. Introduction
This document describes the high-level architecture of **TuxArena**, a top-down multiplayer deathmatch shooter built with C++ and SDL3. The architecture emphasizes modularity, performance, and extensibility, enabling community-driven modding and rapid integration of user-generated content.

### 1.1 Goals
- **Modularity:** Clear separation of core systems (rendering, input, networking, game logic).  
- **Extensibility:** Plugin-based ModAPI for asset overrides and gameplay scripting.  
- **Performance:** Efficient event loop with SDL3, data-driven map loading, and lightweight networking.  
- **Ease of Use:** Simple conventions for dropping maps and mod assets into designated folders.


## 2. System Overview

```plaintext
+----------------+        +----------------+        +----------------+
|                |        |                |        |                |
|   Client App   | <--->  |  Networking    | <--->  |   Server App   |
| (Renderer +    |  UDP   | (Server/Client)|  UDP   | (Game State +  |
|  Input Manager)|        |                |        |   Map Manager) |
+----------------+        +----------------+        +----------------+
         |                          ^                         |
         |                          |                         v
         v                          |                  +----------------+
+----------------+                 +----------------->|    Maps (.tmx)  |
|   Game Loop    |                                     +----------------+
| - EntityMgr    |
| - Physics      |
| - ModManager   |
+----------------+
         |
         v
+----------------+
| Renderer (SDL3)|
+----------------+
```  


## 3. Core Components

### 3.1 Game Loop
- **Responsibility:** Central update loop coordinating input, game state, physics, and rendering.  
- **Phases:**  
  1. **Input Polling:** Gather events via `InputManager`.  
  2. **Game Update:** Advance `EntityManager`, apply physics & collisions.  
  3. **Mod Hooks:** Invoke `ModAPI` callbacks (e.g., `onUpdate`, `onRender`).  
  4. **Network Sync:** Serialize state deltas and dispatch via `NetworkClient` or `NetworkServer`.  
  5. **Rendering:** Issue draw calls to SDL3 renderer.

### 3.2 Renderer (SDL3)
- **Module:** `Renderer`  
- **Responsibilities:**  
  - Initialize SDL3 subsystems (video, audio).  
  - Batch and render sprites, UI overlays, and debug information.  
  - Manage texture caching for character logos, tilesets, and mod assets.

### 3.3 InputManager
- **Module:** `InputManager`  
- **Responsibilities:**  
  - Poll keyboard, mouse, and gamepad events via SDL3.  
  - Map raw events to game actions (move, shoot, switch weapon).  
  - Expose fluent API for querying current input state.

### 3.4 Networking
- **Modules:** `NetworkClient`, `NetworkServer`  
- **Protocol:** Custom UDP for low-latency state synchronization, reliable commands overlaid via ACK tracking.  
- **Responsibilities:**  
  - **Server:** Maintain authoritative game state, process client commands, broadcast snapshots.  
  - **Client:** Send input commands, interpolate remote state, corrective reconciliation.

### 3.5 MapManager
- **Module:** `Map`  
- **Responsibilities:**  
  - Parse Tiled TMX files and associated tilesets.  
  - Generate collision polygons and spawn points.  
  - Expose data to `EntityManager` for level instantiation.

### 3.6 EntityManager
- **Module:** `Entity`  
- **Responsibilities:**  
  - Track all in-game entities (players, projectiles, pickups).  
  - Provide interfaces for querying, iterating, and modifying entity state.  
  - Trigger lifecycle events for mod scripts (spawn, death).

### 3.7 ModManager & ModAPI
- **Module:** `ModManager`, `ModAPI`  
- **Responsibilities:**  
  - Dynamically discover mods in `/mods/*`.  
  - Load native plugins (`.dll`/`.so`) or scripts.  
  - Expose hooks (`onInit`, `onUpdate`, `onRender`, `onEvent`) to modify game behavior and assets.

## 4. Data Flow

1. **Startup**  
   - Load core configuration (client/server mode).  
   - Initialize SDL3 and subsystems.  
   - `ModManager` registers available plugins and applies asset overrides.  
   - Load selected TMX map via `MapManager`.

2. **Main Loop**  
   a. **Input:** `InputManager` polls SDL events.  
   b. **Update:** `EntityManager` & physics step.  
   c. **Mod Hooks:** Execute mod callbacks.  
   d. **Networking:** Serialize local inputs, send to server / process incoming snapshots.  
   e. **Render:** `Renderer` draws current frame.

3. **Shutdown**  
   - Gracefully unload mods.  
   - Clean up SDL3 and networking resources.


## 5. Sequence Diagram (Client Join)

```plaintext
Client                            Server
  |-- Connect Request --------->  |
  |                              |
  | <---- Welcome + MapData -----|
  |                              |
  |-- Input Commands (UDP) ----->|
  |                              |
  | <--- State Snapshots ------- |
  |                              |
  v                              v
Game Loop continues…
```


## 6. Extensibility & Modding
- **Asset Overrides:** Mods declare replacement PNGs for characters or tiles in `mod.json`.  
- **Script Hooks:** If scriptable, mods can register custom behaviors (e.g., powerups).  
- **Configuration:** `mod.json` schema allows metadata (name, version, dependencies).


## 7. Deployment & CI/CD
- **Build:** CMake generates cross-platform Makefiles / VS projects.  
- **Testing:** Automated unit tests in `tests/` run via `ctest`.  
- **Releases:** GitHub Actions workflows build, package, and publish binaries for Windows, Linux, macOS.


## 8. Summary
TuxArena’s architecture cleanly separates core engine subsystems, networking, and modding interfaces. This foundation enables both fast-paced multiplayer action and community-driven customization, satirizing tech rivalries with open-source mascots.

