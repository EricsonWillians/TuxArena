// src/InputManager.cpp
#include "TuxArena/InputManager.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL.h" // For SDL_Log, SDL_GetTicks, etc.

#include <iostream> // For basic logging/error messages
#include <cmath>    // For std::abs in deadzone check

namespace TuxArena {

// Gamepad axis range for normalization
const float GAMEPAD_AXIS_MIN = -32768.0f;
const float GAMEPAD_AXIS_MAX = 32767.0f;


InputManager::InputManager() {
    std::cout << "Initializing InputManager..." << std::endl;
    loadDefaultBindings(); // Setup basic WSAD/Mouse/Gamepad controls
    // SDL_Init(SDL_INIT_GAMEPAD) should be called in main.cpp
    // Check for already connected gamepads at startup
    int numJoysticks = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
    if (joysticks) {
        for (int i = 0; i < numJoysticks; ++i) {
             if (SDL_IsGamepad(joysticks[i])) {
                 addGamepad(joysticks[i]);
                 // For simplicity, only handle the first gamepad found initially
                 if (m_gamepadState.isConnected) break;
             }
        }
        SDL_free(joysticks);
    }
     std::cout << "InputManager initialized. " << (m_gamepadState.isConnected ? "Gamepad detected." : "No gamepad detected.") << std::endl;

}

InputManager::~InputManager() {
    std::cout << "Shutting down InputManager..." << std::endl;
    // Close any open gamepads
    if (m_gamepadState.instance) {
        SDL_CloseGamepad(m_gamepadState.instance);
        m_gamepadState.instance = nullptr;
         m_gamepadState.isConnected = false;
    }
     // Clean up m_activeGamepads map if supporting multiple
    std::cout << "InputManager shut down." << std::endl;

}

void InputManager::loadDefaultBindings() {
     std::cout << "Loading default input bindings..." << std::endl;
    // Keyboard - Movement
    bindKey(SDLK_w, GameAction::MOVE_FORWARD);
    bindKey(SDLK_UP, GameAction::MOVE_FORWARD);
    bindKey(SDLK_s, GameAction::MOVE_BACKWARD);
    bindKey(SDLK_DOWN, GameAction::MOVE_BACKWARD);
    bindKey(SDLK_a, GameAction::MOVE_LEFT);
    bindKey(SDLK_LEFT, GameAction::MOVE_LEFT);
    bindKey(SDLK_d, GameAction::MOVE_RIGHT);
    bindKey(SDLK_RIGHT, GameAction::MOVE_RIGHT);
    bindKey(SDLK_LSHIFT, GameAction::SPRINT);
    bindKey(SDLK_SPACE, GameAction::JUMP); // Or maybe SHOOT?

    // Keyboard - Actions
    bindKey(SDLK_e, GameAction::INTERACT);
    bindKey(SDLK_TAB, GameAction::SHOW_SCORES);
    bindKey(SDLK_ESCAPE, GameAction::PAUSE_MENU);

    // Mouse
    bindMouseButton(SDL_BUTTON_LEFT, GameAction::SHOOT);
    bindMouseButton(SDL_BUTTON_RIGHT, GameAction::ALT_SHOOT);

    // Gamepad - Common Mappings (XInput style)
    bindGamepadButton(SDL_GAMEPAD_BUTTON_A, GameAction::JUMP); // Or Interact
    bindGamepadButton(SDL_GAMEPAD_BUTTON_B, GameAction::SPRINT); // Or Crouch/Back
    bindGamepadButton(SDL_GAMEPAD_BUTTON_X, GameAction::INTERACT); // Or Reload
    //bindGamepadButton(SDL_GAMEPAD_BUTTON_Y, GameAction::SWITCH_WEAPON);
    bindGamepadButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, GameAction::ALT_SHOOT);
    //bindGamepadButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, GameAction::NEXT_WEAPON);
    bindGamepadButton(SDL_GAMEPAD_BUTTON_START, GameAction::PAUSE_MENU);
    bindGamepadButton(SDL_GAMEPAD_BUTTON_BACK, GameAction::SHOW_SCORES);
    bindGamepadButton(SDL_GAMEPAD_BUTTON_LEFT_STICK, GameAction::SPRINT);
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

void InputManager::pollEvents() {
    clearTransientStates();
    m_previousMouseState = m_mouseState; // Store previous state for delta calculation

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                m_quitRequested = true;
                break;

            // --- Keyboard ---
            case SDL_EVENT_KEY_DOWN:
                // Ignore key repeats for "just pressed" logic
                if (event.key.repeat == 0) {
                    SDL_Keycode key = event.key.keysym.sym;
                    bool wasPressed = m_currentKeys.count(key);
                    m_currentKeys.insert(key);
                    if (!wasPressed) {
                        m_justPressedKeys.insert(key);
                        // Update action state if bound
                         if (m_keyBindings.count(key)) {
                             m_actionJustPressed[m_keyBindings[key]] = true;
                         }
                    }
                }
                break;

            case SDL_EVENT_KEY_UP:
                if (event.key.repeat == 0) { // Should always be 0 for up event
                    SDL_Keycode key = event.key.keysym.sym;
                    if (m_currentKeys.count(key)) {
                        m_currentKeys.erase(key);
                        m_justReleasedKeys.insert(key);
                         // Update action state if bound
                         if (m_keyBindings.count(key)) {
                             m_actionJustReleased[m_keyBindings[key]] = true;
                         }
                    }
                }
                break;

            // --- Mouse ---
            case SDL_EVENT_MOUSE_MOTION:
                m_mouseState.x = event.motion.x;
                m_mouseState.y = event.motion.y;
                m_mouseState.deltaX = event.motion.xrel;
                m_mouseState.deltaY = event.motion.yrel;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    Uint8 button = event.button.button;
                    bool wasPressed = m_mouseState.currentButtons.count(button);
                    m_mouseState.currentButtons.insert(button);
                    if (!wasPressed) {
                        m_mouseState.justPressedButtons.insert(button);
                         // Update action state if bound
                         if (m_mouseButtonBindings.count(button)) {
                             m_actionJustPressed[m_mouseButtonBindings[button]] = true;
                         }
                    }
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                 {
                    Uint8 button = event.button.button;
                     if (m_mouseState.currentButtons.count(button)) {
                         m_mouseState.currentButtons.erase(button);
                         m_mouseState.justReleasedButtons.insert(button);
                          // Update action state if bound
                          if (m_mouseButtonBindings.count(button)) {
                              m_actionJustReleased[m_mouseButtonBindings[button]] = true;
                          }
                     }
                 }
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                 // SDL3 uses float precision for wheel events
                 m_mouseState.scrollX = event.wheel.x;
                 m_mouseState.scrollY = event.wheel.y;
                break;

            // --- Gamepad ---
            case SDL_EVENT_GAMEPAD_ADDED:
                 addGamepad(event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                removeGamepad(event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                // Only process if it's our active gamepad
                if (m_gamepadState.isConnected && event.gbutton.which == m_gamepadState.instanceId) {
                    SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;
                    bool wasPressed = m_gamepadState.currentButtons[button]; // operator[] creates if not exist
                    m_gamepadState.currentButtons[button] = true;
                    if (!wasPressed) {
                        m_gamepadState.justPressedButtons[button] = true;
                         // Update action state if bound
                         if (m_gamepadButtonBindings.count(button)) {
                              m_actionJustPressed[m_gamepadButtonBindings[button]] = true;
                         }
                    }
                }
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (m_gamepadState.isConnected && event.gbutton.which == m_gamepadState.instanceId) {
                    SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;
                     if (m_gamepadState.currentButtons[button]) { // Check if it was actually pressed
                         m_gamepadState.currentButtons[button] = false;
                         m_gamepadState.justReleasedButtons[button] = true;
                          // Update action state if bound
                          if (m_gamepadButtonBindings.count(button)) {
                              m_actionJustReleased[m_gamepadButtonBindings[button]] = true;
                          }
                     }
                }
                break;

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                if (m_gamepadState.isConnected && event.gaxis.which == m_gamepadState.instanceId) {
                     SDL_GamepadAxis axis = (SDL_GamepadAxis)event.gaxis.axis;
                     float value = normalizeAxisValue(event.gaxis.value);
                     m_gamepadState.axisValues[axis] = value;

                     // Update trigger states specifically
                     if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) {
                         m_gamepadState.triggerLeft = value;
                     } else if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
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

    // After processing all events, update the continuous "pressed" action states
    updateActionStates();
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
             // Or check if SHOOT action is bound to a specific trigger button enum if SDL provides one.
             // For now, let's assume if SHOOT is bound to *any* gamepad button, the trigger might also activate it.
             // A better way is needed. Maybe bind GameAction::SHOOT_TRIGGER explicitly?
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
    std::cerr << "Warning: InputManager::loadBindings('" << filePath << "') not implemented yet." << std::endl;
    return false;
}

void InputManager::bindKey(SDL_Keycode key, GameAction action) {
    m_keyBindings[key] = action;
     // std::cout << "Bound key " << SDL_GetKeyName(key) << " to action " << static_cast<int>(action) << std::endl;
}

void InputManager::bindMouseButton(Uint8 button, GameAction action) {
    m_mouseButtonBindings[button] = action;
     // std::cout << "Bound mouse button " << (int)button << " to action " << static_cast<int>(action) << std::endl;
}

void InputManager::bindGamepadButton(SDL_GamepadButton button, GameAction action) {
     m_gamepadButtonBindings[button] = action;
     // std::cout << "Bound gamepad button " << SDL_GetGamepadStringForButton(button) << " to action " << static_cast<int>(action) << std::endl;
}

void InputManager::unbindKey(SDL_Keycode key) {
     m_keyBindings.erase(key);
}

// --- Gamepad Specific ---

float InputManager::getGamepadAxis(SDL_GamepadAxis axis, float deadZone) const {
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

     if (!SDL_IsGamepad(which)) {
          SDL_Log("InputManager: Device (ID: %d) is not a gamepad.", which);
          return;
     }

     SDL_Gamepad* gamepad = SDL_OpenGamepad(which);
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


     SDL_Log("InputManager: Gamepad Added (ID: %d, Name: %s)", which, SDL_GetGamepadName(gamepad));
}

void InputManager::removeGamepad(SDL_JoystickID which) {
     // Only handle removal if it's our active gamepad
     if (m_gamepadState.isConnected && which == m_gamepadState.instanceId) {
         SDL_Log("InputManager: Gamepad Removed (ID: %d, Name: %s)", which, SDL_GetGamepadName(m_gamepadState.instance));
         SDL_CloseGamepad(m_gamepadState.instance);
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