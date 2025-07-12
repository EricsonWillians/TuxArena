// src/InputManager.cpp
#include "TuxArena/InputManager.h"
#include "SDL2/SDL_events.h"
#include "SDL2/SDL.h" // For SDL_Log, SDL_GetTicks, etc.
#include "SDL2/SDL_gamecontroller.h" // For SDL_GameController, SDL_GameControllerButton, etc.
#include "TuxArena/Log.h" // Added for logging
#include "imgui_impl_sdl2.h" // Added for ImGui event processing

#include <iostream> // For basic logging/error messages
#include <cmath>    // For std::abs in deadzone check

namespace TuxArena {

// Gamepad axis range for normalization
const float GAMEPAD_AXIS_MIN = -32768.0f;
const float GAMEPAD_AXIS_MAX = 32767.0f;


InputManager::InputManager() {
    Log::Info("Initializing InputManager...");
    loadDefaultBindings(); // Setup basic WSAD/Mouse/Gamepad controls
    // SDL_Init(SDL_INIT_GAMEPAD) should be called in main.cpp
    // Check for already connected gamepads at startup
    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            addGamepad(i);
            // For simplicity, only handle the first gamepad found initially
            if (m_gamepadState.isConnected) break;
        }
    }
     Log::Info(std::string("InputManager initialized. ") + (m_gamepadState.isConnected ? "Gamepad detected." : "No gamepad detected."));

}

InputManager::~InputManager() {
    Log::Info("Shutting down InputManager...");
    // Close any open gamepads
    if (m_gamepadState.instance) {
        SDL_GameControllerClose(m_gamepadState.instance);
        m_gamepadState.instance = nullptr;
        m_gamepadState.isConnected = false;
    }
     // Clean up m_activeGamepads map if supporting multiple
    Log::Info("InputManager shut down.");

}

void InputManager::loadDefaultBindings() {
     Log::Info("Loading default input bindings...");
    // Keyboard - Movement
    bindKey(SDL_SCANCODE_W, GameAction::MOVE_FORWARD);
    bindKey(SDL_SCANCODE_UP, GameAction::MOVE_FORWARD);
    bindKey(SDL_SCANCODE_S, GameAction::MOVE_BACKWARD);
    bindKey(SDL_SCANCODE_DOWN, GameAction::MOVE_BACKWARD);
    bindKey(SDL_SCANCODE_A, GameAction::STRAFE_LEFT);
    bindKey(SDL_SCANCODE_LEFT, GameAction::TURN_LEFT);
    bindKey(SDL_SCANCODE_D, GameAction::STRAFE_RIGHT);
    bindKey(SDL_SCANCODE_RIGHT, GameAction::TURN_RIGHT);
    bindKey(SDL_SCANCODE_LSHIFT, GameAction::SPRINT);
    bindKey(SDL_SCANCODE_SPACE, GameAction::JUMP); // Or maybe SHOOT?

    // Keyboard - Actions
    bindKey(SDL_SCANCODE_E, GameAction::INTERACT);
    bindKey(SDL_SCANCODE_TAB, GameAction::SHOW_SCORES);
    bindKey(SDL_SCANCODE_ESCAPE, GameAction::PAUSE);

    // Mouse
    bindMouseButton(SDL_BUTTON_LEFT, GameAction::FIRE_PRIMARY);
    bindMouseButton(SDL_BUTTON_RIGHT, GameAction::FIRE_SECONDARY);

    // Keyboard - Weapon Slots
    bindKey(SDL_SCANCODE_1, GameAction::WEAPON_SLOT_1);
    bindKey(SDL_SCANCODE_2, GameAction::WEAPON_SLOT_2);
    bindKey(SDL_SCANCODE_3, GameAction::WEAPON_SLOT_3);
    bindKey(SDL_SCANCODE_4, GameAction::WEAPON_SLOT_4);
    bindKey(SDL_SCANCODE_5, GameAction::WEAPON_SLOT_5);
    bindKey(SDL_SCANCODE_6, GameAction::WEAPON_SLOT_6);
    bindKey(SDL_SCANCODE_7, GameAction::WEAPON_SLOT_7);
    bindKey(SDL_SCANCODE_8, GameAction::WEAPON_SLOT_8);
    bindKey(SDL_SCANCODE_9, GameAction::WEAPON_SLOT_9);

    // Gamepad - Common Mappings (XInput style)
    bindGamepadButton(SDL_CONTROLLER_BUTTON_A, GameAction::JUMP); // Or Interact
    bindGamepadButton(SDL_CONTROLLER_BUTTON_B, GameAction::SPRINT); // Or Crouch/Back
    bindGamepadButton(SDL_CONTROLLER_BUTTON_X, GameAction::INTERACT); // Or Reload
    //bindGamepadButton(SDL_CONTROLLER_BUTTON_Y, GameAction::SWITCH_WEAPON);
    bindGamepadButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, GameAction::FIRE_SECONDARY);
    //bindGamepadButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, GameAction::NEXT_WEAPON);
    bindGamepadButton(SDL_CONTROLLER_BUTTON_START, GameAction::PAUSE_MENU);
    bindGamepadButton(SDL_CONTROLLER_BUTTON_BACK, GameAction::SHOW_SCORES);
    bindGamepadButton(SDL_CONTROLLER_BUTTON_LEFTSTICK, GameAction::SPRINT);
    //bindGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP, GameAction::SWITCH_WEAPON_UP);
    // ... map other DPAD directions if needed

    // Note: Gamepad movement (left stick) and aiming (right stick/triggers)
    // are usually handled by reading axis values directly, not binding to actions.
    // Right Trigger is often used for SHOOT, read via getGamepadTriggerRight().
}


void InputManager::clearTransientStates() {
    m_justPressedKeys.clear();
    m_justReleasedKeys.clear();
    m_mouseState.justPressedButtons.clear();
    m_mouseState.justReleasedButtons.clear();
    m_mouseState.deltaX = 0.0f;
    m_mouseState.deltaY = 0.0f;
    m_mouseState.scrollX = 0.0f;
    m_mouseState.scrollY = 0.0f;

    if (m_gamepadState.isConnected) {
        m_gamepadState.justPressedButtons.clear();
        m_gamepadState.justReleasedButtons.clear();
    }

    m_actionJustPressed.clear();
    m_actionJustReleased.clear();
}

void InputManager::processSDLEvent(SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event); // Pass event to ImGui

    switch (event.type) {
        case SDL_QUIT:
            m_quitRequested = true;
            break;

        // --- Keyboard ---
        case SDL_KEYDOWN:
            // Ignore key repeats for "just pressed" logic
            if (event.key.repeat == 0) {
                SDL_Keycode key = event.key.keysym.sym;
                bool wasPressed = m_currentKeys.count(key);
                m_currentKeys.insert(key);
                if (!wasPressed) {
                    m_justPressedKeys.insert(key);
                }
            }
            break;

        case SDL_KEYUP:
            if (event.key.repeat == 0) { // Should always be 0 for up event
                SDL_Keycode key = event.key.keysym.sym;
                if (m_currentKeys.count(key)) {
                    m_currentKeys.erase(key);
                    m_justReleasedKeys.insert(key);
                }
            }
            break;

        // --- Mouse ---
        case SDL_MOUSEMOTION:
            m_mouseState.x = (float)event.motion.x;
            m_mouseState.y = (float)event.motion.y;
            m_mouseState.deltaX = (float)event.motion.xrel;
            m_mouseState.deltaY = (float)event.motion.yrel;
            break;

        case SDL_MOUSEBUTTONDOWN:
            {
                Uint8 button = event.button.button;
                bool wasPressed = m_mouseState.currentButtons.count(button);
                m_mouseState.currentButtons.insert(button);
                if (!wasPressed) {
                    m_mouseState.justPressedButtons.insert(button);
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
             {
                Uint8 button = event.button.button;
                 if (m_mouseState.currentButtons.count(button)) {
                     m_mouseState.currentButtons.erase(button);
                     m_mouseState.justReleasedButtons.insert(button);
                 }
            }
            break;

        case SDL_MOUSEWHEEL:
             // SDL2 uses int for wheel events
             m_mouseState.scrollX = (float)event.wheel.x;
             m_mouseState.scrollY = (float)event.wheel.y;
            break;

        // --- Gamepad ---
        case SDL_CONTROLLERDEVICEADDED:
             addGamepad(event.cdevice.which);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            removeGamepad(event.cdevice.which);
            break;

        case SDL_CONTROLLERBUTTONDOWN:
            // Only process if it's our active gamepad
            if (m_gamepadState.isConnected && event.cbutton.which == m_gamepadState.instanceId) {
                SDL_GameControllerButton button = (SDL_GameControllerButton)event.cbutton.button;
                bool wasPressed = m_gamepadState.currentButtons[button]; // operator[] creates if not exist
                m_gamepadState.currentButtons[button] = true;
                if (!wasPressed) {
                    m_gamepadState.justPressedButtons[button] = true;
                }
            }
            break;

        case SDL_CONTROLLERBUTTONUP:
            if (m_gamepadState.isConnected && event.cbutton.which == m_gamepadState.instanceId) {
                SDL_GameControllerButton button = (SDL_GameControllerButton)event.cbutton.button;
                 if (m_gamepadState.currentButtons.count(button)) { // Check if it was actually pressed
                     m_gamepadState.currentButtons.erase(button);
                     m_gamepadState.justReleasedButtons[button] = true;
                 }
            }
            break;

        case SDL_CONTROLLERAXISMOTION:
            if (m_gamepadState.isConnected && event.caxis.which == m_gamepadState.instanceId) {
                 SDL_GameControllerAxis axis = (SDL_GameControllerAxis)event.caxis.axis;
                 float value = normalizeAxisValue(event.caxis.value);
                 m_gamepadState.axisValues[axis] = value;

                 // Update trigger states specifically
                 if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                     m_gamepadState.triggerLeft = value;
                 } else if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                     m_gamepadState.triggerRight = value;
                 }
            }
            break;

         // Add window events if needed (resize, focus gain/loss)
         // case SDL_EVENT_WINDOW_RESIZED:
         // case SDL_EVENT_WINDOW_FOCUS_GAINED:
         // case SDL_EVENT_WINDOW_FOCUS_LOST:
    }
}

void InputManager::updateActionStates() {
     m_actionPressed.clear(); // Recalculate based on current state

     // Check keyboard bindings
     for (const auto& pair : m_keyBindings) {
         if (m_currentKeys.count(pair.first)) {
             m_actionPressed[pair.second] = true;
         }
     }

     // Check mouse button bindings
     for (const auto& pair : m_mouseButtonBindings) {
          if (m_mouseState.currentButtons.count(pair.first)) {
              m_actionPressed[pair.second] = true;
          }
     }

     // Check gamepad button bindings
     if (m_gamepadState.isConnected) {
         for (const auto& pair : m_gamepadButtonBindings) {
             if (m_gamepadState.currentButtons[pair.first]) { // Uses map's default false if not set
                 m_actionPressed[pair.second] = true;
             }
         }
         // Special handling for trigger action (e.g., if SHOOT is bound to right trigger)
         // Example: Treat trigger > 50% as pressed for SHOOT action
         if (m_gamepadState.triggerRight > 0.5f) {
             // Check if an action like SHOOT is mapped *conceptually* to the trigger
             // This part needs refinement - maybe have specific Trigger Actions?
             // Or simply require game logic to check getGamepadTriggerRight() directly.
             // Let's comment this out for now, logic should check trigger directly.
             // if (m_gamepadButtonBindings.count(SDL_GAMEPAD_BUTTON_INVALID)) { // Find if SHOOT is bound conceptually
             //    // Find the action bound to trigger somehow... complex.
             // }
         }

     }
}

// --- Action Mapping Queries ---

bool InputManager::isActionPressed(GameAction action) const {
    auto it = m_actionPressed.find(action);
    return (it != m_actionPressed.end() && it->second);
}

bool InputManager::isActionJustPressed(GameAction action) const {
    auto it = m_actionJustPressed.find(action);
    return (it != m_actionJustPressed.end() && it->second);
}

bool InputManager::isActionJustReleased(GameAction action) const {
     auto it = m_actionJustReleased.find(action);
     return (it != m_actionJustReleased.end() && it->second);
}

// --- Direct State Queries ---

void InputManager::getMousePosition(float& x, float& y) const {
    x = m_mouseState.x;
    y = m_mouseState.y;
}

void InputManager::getMouseDelta(float& dx, float& dy) const {
     dx = m_mouseState.deltaX;
     dy = m_mouseState.deltaY;
}

void InputManager::getMouseScrollDelta(float& dx, float& dy) const {
     dx = m_mouseState.scrollX;
     dy = m_mouseState.scrollY;
}

bool InputManager::isMouseButtonPressed(Uint8 button) const {
     return m_mouseState.currentButtons.count(button);
}

bool InputManager::isMouseButtonJustPressed(Uint8 button) const {
     return m_mouseState.justPressedButtons.count(button);
}

bool InputManager::isMouseButtonJustReleased(Uint8 button) const {
     return m_mouseState.justReleasedButtons.count(button);
}

bool InputManager::isKeyPressed(SDL_Keycode key) const {
    return m_currentKeys.count(key);
}

bool InputManager::isKeyJustPressed(SDL_Keycode key) const {
     return m_justPressedKeys.count(key);
}

bool InputManager::isKeyJustReleased(SDL_Keycode key) const {
     return m_justReleasedKeys.count(key);
}


// --- Binding Configuration ---

bool InputManager::loadBindings(const std::string& filePath) {
    // TODO: Implement loading bindings from a file (e.g., JSON, INI)
    Log::Warning("InputManager::loadBindings('" + filePath + "') not implemented yet.");
    return false;
}

void InputManager::bindKey(SDL_Keycode key, GameAction action) {
    m_keyBindings[key] = action;
     // Log::Info("Bound key " + std::string(SDL_GetKeyName(key)) + " to action " + std::to_string(static_cast<int>(action)));
}

void InputManager::bindMouseButton(Uint8 button, GameAction action) {
    m_mouseButtonBindings[button] = action;
     // Log::Info("Bound mouse button " + std::to_string(static_cast<int>(button)) + " to action " + std::to_string(static_cast<int>(action)));
}

void InputManager::bindGamepadButton(SDL_GameControllerButton button, GameAction action) {
     m_gamepadButtonBindings[button] = action;
     // Log::Info("Bound gamepad button " + std::string(SDL_GameControllerGetStringForButton(button)) + " to action " + std::to_string(static_cast<int>(action)));
}

void InputManager::unbindKey(SDL_Keycode key) {
     m_keyBindings.erase(key);
}

// --- Gamepad Specific ---

float InputManager::getGamepadAxis(SDL_GameControllerAxis axis, float deadZone) const {
    if (!m_gamepadState.isConnected) return 0.0f;

    auto it = m_gamepadState.axisValues.find(axis);
    if (it != m_gamepadState.axisValues.end()) {
        float value = it->second;
        // Apply deadzone
        if (std::abs(value) < deadZone) {
            return 0.0f;
        }
         // Optional: Remap axis value from [deadZone, 1.0] to [0.0, 1.0] for smoother control start
         // float range = 1.0f - deadZone;
         // value = (value - std::copysign(deadZone, value)) / range;
        return value;
    }
    return 0.0f; // Axis not found or not moved
}

float InputManager::getGamepadTriggerLeft() const {
    return m_gamepadState.isConnected ? m_gamepadState.triggerLeft : 0.0f;
}

float InputManager::getGamepadTriggerRight() const {
     return m_gamepadState.isConnected ? m_gamepadState.triggerRight : 0.0f;
}

bool InputManager::isGamepadConnected() const {
     return m_gamepadState.isConnected;
}


// --- Helper Methods ---

void InputManager::addGamepad(SDL_JoystickID which) {
     // Only handle one gamepad for now
     if (m_gamepadState.isConnected) {
         SDL_Log("InputManager: Already have an active gamepad, ignoring new one (ID: %d)", which);
         return;
     }

     if (!SDL_IsGameController(which)) {
          SDL_Log("InputManager: Device (ID: %d) is not a gamepad.", which);
          return;
     }

     SDL_GameController* gamepad = SDL_GameControllerOpen(which);
     if (!gamepad) {
         SDL_Log("InputManager: Could not open gamepad (ID: %d): %s", which, SDL_GetError());
         return;
     }

     m_gamepadState.instance = gamepad;
     m_gamepadState.instanceId = which; // Store the ID
     m_gamepadState.isConnected = true;
     // Clear previous states just in case
     m_gamepadState.currentButtons.clear();
     m_gamepadState.justPressedButtons.clear();
     m_gamepadState.justReleasedButtons.clear();
     m_gamepadState.axisValues.clear();
     m_gamepadState.triggerLeft = 0.0f;
     m_gamepadState.triggerRight = 0.0f;


     SDL_Log("InputManager: Gamepad Added (ID: %d, Name: %s)", which, SDL_GameControllerName(gamepad));
}

void InputManager::removeGamepad(SDL_JoystickID which) {
     // Only handle removal if it's our active gamepad
     if (m_gamepadState.isConnected && which == m_gamepadState.instanceId) {
         SDL_Log("InputManager: Gamepad Removed (ID: %d, Name: %s)", which, SDL_GameControllerName(m_gamepadState.instance));
         SDL_GameControllerClose(m_gamepadState.instance);
         m_gamepadState.instance = nullptr;
         m_gamepadState.instanceId = 0;
         m_gamepadState.isConnected = false;
         // Clear states
         m_gamepadState.currentButtons.clear();
         m_gamepadState.justPressedButtons.clear();
         m_gamepadState.justReleasedButtons.clear();
         m_gamepadState.axisValues.clear();
         m_gamepadState.triggerLeft = 0.0f;
         m_gamepadState.triggerRight = 0.0f;
         // Clear related actions
         updateActionStates();
     } else {
          SDL_Log("InputManager: Removed gamepad (ID: %d) was not the active one.", which);
     }
}

float InputManager::normalizeAxisValue(Sint16 rawValue) const {
     // Normalize SInt16 range to -1.0 to 1.0 (or 0.0 to 1.0 for triggers)
     // Note: SDL_GAMEPAD_AXIS_LEFT_TRIGGER and RIGHT_TRIGGER are 0 to 32767
     // Other axes are -32768 to 32767
     if (rawValue >= 0) { // Positive values or triggers
         return static_cast<float>(rawValue) / GAMEPAD_AXIS_MAX;
     } else { // Negative values
         return static_cast<float>(rawValue) / -GAMEPAD_AXIS_MIN; // Divide by positive 32768
     }
}


} // namespace TuxArena