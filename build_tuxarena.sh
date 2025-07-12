#!/bin/sh
set -eu

# === Config ===
PROJECT_DIR=$(dirname "$(readlink -f "$0")")
BUILD_DIR="$PROJECT_DIR/build"
INSTALL_PREFIX="${INSTALL_PREFIX:-$HOME/.local}"
IMGUI_CONFIG_HEADER="$PROJECT_DIR/third_party/imgui/tuxarena_imgui_config.h"
LOG_FILE="$BUILD_DIR/build.log"

# === Timestamp logging ===
log() {
  echo "[$(date +%H:%M:%S)] $*" | tee -a "$LOG_FILE"
}

# === Ensure dependencies ===
required_tools="cmake g++ make nproc git"
for tool in $required_tools; do
  if ! which "$tool" >/dev/null; then
    echo "Required tool '$tool' is not installed."
    exit 1
  fi
done

# === Create ImGui config if missing ===
if [ ! -f "$IMGUI_CONFIG_HEADER" ]; then
  log "Generating missing tuxarena_imgui_config.h..."
  cat > "$IMGUI_CONFIG_HEADER" <<EOF
#ifndef TUXARENA_IMGUI_CONFIG_H
#define TUXARENA_IMGUI_CONFIG_H

// ImGui custom configuration
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_ENABLE_FREETYPE

#endif // TUXARENA_IMGUI_CONFIG_H
EOF
fi

# === Parse arguments ===
ACTION="${1:-build}"
case "$ACTION" in
  clean)
    log "Cleaning build and vendor dirs..."
    rm -rf "$BUILD_DIR" \
      "$PROJECT_DIR/third_party/SDL/build" \
      "$PROJECT_DIR/third_party/SDL_net/build" \
      "$PROJECT_DIR/third_party/SDL_image" \
      "$PROJECT_DIR/third_party/SDL_ttf" \
      "$PROJECT_DIR/third_party/tmxlite"
    exit 0
    ;;
  install)
    INSTALL=1
    ;;
  build)
    INSTALL=0
    ;;
  *)
    echo "Usage: $0 [clean|build|install]"
    exit 1
    ;;
esac

mkdir -p "$BUILD_DIR"
echo "" > "$LOG_FILE"  # Reset log

# === Clone third-party libraries if not present ===
log "Checking for third-party libraries..."

if [ ! -d "$PROJECT_DIR/third_party/SDL_image" ]; then
  log "Cloning SDL_image..."
  git clone https://github.com/libsdl-org/SDL_image.git "$PROJECT_DIR/third_party/SDL_image"
fi

if [ ! -d "$PROJECT_DIR/third_party/SDL_ttf" ]; then
  log "Cloning SDL_ttf..."
  git clone https://github.com/libsdl-org/SDL_ttf.git "$PROJECT_DIR/third_party/SDL_ttf"
fi

if [ ! -d "$PROJECT_DIR/third_party/tmxlite" ]; then
  log "Cloning tmxlite..."
  git clone https://github.com/fallahn/tmxlite.git "$PROJECT_DIR/third_party/tmxlite"
fi

# === CMake flags ===
CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DBUILD_SHARED_LIBS=OFF -DSDL3_VENDOR=ON -DSDL3_DISABLE_INSTALL=ON -DSDL3_STATIC=ON -DSDL3_SHARED=OFF -DSDL3NET_VENDOR=ON -DSDL3NET_DISABLE_INSTALL=ON -DSDL3NET_STATIC=ON -DSDL3NET_SHARED=OFF -DSDL3IMAGE_VENDOR=ON -DSDL3IMAGE_DISABLE_INSTALL=ON -DSDL3IMAGE_STATIC=ON -DSDL3IMAGE_SHARED=OFF -DSDL3TTF_VENDOR=ON -DSDL3TTF_DISABLE_INSTALL=ON -DSDL3TTF_STATIC=ON -DSDL3TTF_SHARED=OFF -DTMXLITE_VENDOR=ON -DTMXLITE_DISABLE_INSTALL=ON -DTMXLITE_STATIC=ON -DTMXLITE_SHARED=OFF"


# === Configure ===
log "Configuring with CMake..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" $CMAKE_FLAGS -DCMAKE_CXX_FLAGS="-include $IMGUI_CONFIG_HEADER" 2>&1 | tee -a "$LOG_FILE"

# === Build ===
log "Building project..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)" 2>&1 | tee -a "$LOG_FILE"

# === Install ===
if [ "$INSTALL" -eq 1 ]; then
  log "Installing to $INSTALL_PREFIX..."
  cmake --install "$BUILD_DIR" 2>&1 | tee -a "$LOG_FILE"
fi

log "Build completed successfully."