#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e
# Treat unset variables as an error when substituting.
set -u
# Pipelines return the exit status of the last command to exit with a non-zero status,
# or zero if all commands exit successfully.
set -o pipefail

# --- Configuration ---
PROJECT_ROOT="TuxArena"

# --- Safety Check ---
if [ -e "$PROJECT_ROOT" ]; then
  echo "Error: '$PROJECT_ROOT' already exists in the current directory." >&2
  echo "Please remove or rename it before running this script." >&2
  exit 1
fi

# --- Main Script ---
echo "Creating project structure for '$PROJECT_ROOT'..."

# Create root directory and navigate into it
mkdir "$PROJECT_ROOT"
cd "$PROJECT_ROOT" || exit 1 # Exit if cd fails

echo "Creating root files..."
touch CMakeLists.txt LICENSE README.md .gitignore clang-format Doxyfile docker-compose.yml

echo "Creating directories..."
mkdir -p \
  docs \
  .github/workflows \
  third_party/SDL3 \
  include/TuxArena \
  src \
  assets/characters \
  assets/tilesets \
  assets/audio \
  assets/fonts \
  maps/example_map/resources \
  mods/example_mod/characters_override \
  mods/example_mod/scripts \
  scripts \
  tools \
  examples \
  tests

echo "Creating files within directories..."

# docs
touch docs/architecture.md docs/modding_guide.md docs/network_protocol.md docs/map_format.md

# .github/workflows
touch .github/workflows/ci.yml .github/workflows/release.yml

# include/TuxArena
touch include/TuxArena/Game.h include/TuxArena/Entity.h include/TuxArena/Player.h \
      include/TuxArena/Map.h include/TuxArena/Renderer.h include/TuxArena/Network.h \
      include/TuxArena/InputManager.h include/TuxArena/ModAPI.h

# src
touch src/main.cpp src/Game.cpp src/Entity.cpp src/Player.cpp src/Map.cpp \
      src/Renderer.cpp src/NetworkServer.cpp src/NetworkClient.cpp \
      src/InputManager.cpp src/ModManager.cpp

# assets
touch assets/characters/tux.png assets/characters/rustacean.png \
      assets/characters/gnu.png assets/characters/go_gopher.png
touch assets/tilesets/topdown_tileset.png
touch assets/audio/shoot.wav assets/audio/explosion.wav
touch assets/fonts/nokia.ttf

# maps
touch maps/README.md maps/arena1.tmx
touch maps/example_map/example_map.tmx

# mods
touch mods/README.md
touch mods/example_mod/mod.json
touch mods/example_mod/characters_override/medieval_tux.png

# scripts (and make them executable)
touch scripts/build.sh scripts/run_server.sh scripts/run_client.sh
chmod +x scripts/build.sh scripts/run_server.sh scripts/run_client.sh
echo "Made scripts executable."

# tools
touch tools/tmx_inspector.cpp

# examples
touch examples/dedicated_server_config.json

# tests
touch tests/CMakeLists.txt tests/test_network.cpp

# --- Completion ---
echo "Project structure created successfully in './$PROJECT_ROOT'"
exit 0
