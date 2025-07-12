// TuxArena ImGui Configuration
#pragma once

// Enable docking
#define IMGUI_ENABLE_DOCKING

// Enable viewports
#define IMGUI_ENABLE_VIEWPORTS

// Disable obsolete functions
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// Memory allocator overrides (optional)
// #define IMGUI_USE_CUSTOM_ALLOCATOR

// Assert macro override (optional)
// #define IM_ASSERT(_EXPR) assert(_EXPR)

// Additional TuxArena-specific configurations
#define TUXARENA_IMGUI_CONFIG_VERSION 1
#define IMGUI_DISABLE_FREETYPE
#undef IMGUI_ENABLE_FREETYPE
