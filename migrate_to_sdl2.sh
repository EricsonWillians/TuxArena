#!/bin/bash

set -e

echo "🔧 Installing SDL2 dependencies..."
sudo apt update
sudo apt install -y \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    libsdl2-net-dev

echo "🔄 Replacing SDL3 includes with SDL2..."
find . -type f \( -name "*.cpp" -o -name "*.h" \) \
    -exec sed -i 's#<SDL3/#<SDL2/#g' {} \; \
    -exec sed -i 's#"SDL3/#"SDL2/#g' {} \;

echo "📝 Updating CMakeLists.txt..."
sed -i 's/SDL3/SDL2/g' CMakeLists.txt
sed -i 's/SDL3_/SDL2_/g' CMakeLists.txt

echo "✅ Migration complete. Cleaning and rebuilding..."
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)

echo "🎉 Done. Your project now uses SDL2."
