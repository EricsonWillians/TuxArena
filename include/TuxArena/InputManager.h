#ifndef TUXARENA_INPUTMANAGER_H
#define TUXARENA_INPUTMANAGER_H

#include <SDL2/SDL.h>
#include <map>
#include <set>
#include <string>

namespace TuxArena {

enum class GameAction {
    // Movement
    MOVE_FORWARD,
    MOVE_BACKWARD,
    STRAFE_LEFT,
    STRAFE_RIGHT,
    TURN_LEFT,      // For keyboard-only turning
    TURN_RIGHT,     // For keyboard-only turning
    JUMP,
    SPRINT,

    // Combat
    FIRE_PRIMARY,
    FIRE_SECONDARY,
    RELOAD,
    NEXT_WEAPON,
    PREVIOUS_WEAPON,

    // Weapon Slots
    WEAPON_SLOT_1,
    WEAPON_SLOT_2,
    WEAPON_SLOT_3,
    WEAPON_SLOT_4,
    WEAPON_SLOT_5,
    WEAPON_SLOT_6,
    WEAPON_SLOT_7,
    WEAPON_SLOT_8,
    WEAPON_SLOT_9,

    // UI / Game
    INTERACT,
    SHOW_SCORES,
    PAUSE_MENU,
    PAUSE, // Added PAUSE action

    // Mouse specific (usually not bound directly, but useful for state checking)
    LOOK_UP,
    LOOK_DOWN,
    LOOK_LEFT,
    LOOK_RIGHT,

    // Gamepad specific
    GAMEPAD_LOOK_UP,
    GAMEPAD_LOOK_DOWN,
    GAMEPAD_LOOK_LEFT,
    GAMEPAD_LOOK_RIGHT,
};


// Struct to hold mouse state
struct MouseState {
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    std::set<uint8_t> currentButtons;
    std::set<uint8_t> justPressedButtons;
    std::set<uint8_t> justReleasedButtons;
};

// Struct to hold gamepad state
struct GamepadState {
    SDL_GameController* instance = nullptr;
    SDL_JoystickID instanceId = 0;
    bool isConnected = false;
    std::map<SDL_GameControllerButton, bool> currentButtons;
    std::map<SDL_GameControllerButton, bool> justPressedButtons;
    std::map<SDL_GameControllerButton, bool> justReleasedButtons;
    std::map<SDL_GameControllerAxis, float> axisValues;
    float triggerLeft = 0.0f;
    float triggerRight = 0.0f;
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    void processSDLEvent(SDL_Event& event);
    void updateActionStates();
    void clearTransientStates();

    bool isActionPressed(GameAction action) const;
    bool isActionJustPressed(GameAction action) const;
    bool isActionJustReleased(GameAction action) const;

    // Mouse input
    void getMousePosition(float& x, float& y) const;
    void getMouseDelta(float& dx, float& dy) const;
    void getMouseScrollDelta(float& dx, float& dy) const;
    bool isMouseButtonPressed(uint8_t button) const;
    bool isMouseButtonJustPressed(uint8_t button) const;
    bool isMouseButtonJustReleased(uint8_t button) const;

    // Keyboard input
    bool isKeyPressed(SDL_Keycode key) const;
    bool isKeyJustPressed(SDL_Keycode key) const;
    bool isKeyJustReleased(SDL_Keycode key) const;

    // Gamepad input
    float getGamepadAxis(SDL_GameControllerAxis axis, float deadZone = 0.1f) const;
    float getGamepadTriggerLeft() const;
    float getGamepadTriggerRight() const;
    bool isGamepadConnected() const;

    bool quitRequested() const { return m_quitRequested; }

    // Binding configuration
    bool loadBindings(const std::string& filePath);
    void bindKey(SDL_Keycode key, GameAction action);
    void bindMouseButton(uint8_t button, GameAction action);
    void bindGamepadButton(SDL_GameControllerButton button, GameAction action);
    void unbindKey(SDL_Keycode key);

private:
    // Map SDL_Scancode to GameAction
    std::map<SDL_Keycode, GameAction> m_keyBindings;
    // Map SDL_MouseButtonID to GameAction
    std::map<uint8_t, GameAction> m_mouseButtonBindings;
    // Map SDL_GamepadButton to GameAction
    std::map<SDL_GameControllerButton, GameAction> m_gamepadButtonBindings;

    // Current state of actions (true if pressed)
    std::map<GameAction, bool> m_actionPressed;
    // Actions that were just pressed in the current frame
    std::map<GameAction, bool> m_actionJustPressed;
    // Actions that were just released in the current frame
    std::map<GameAction, bool> m_actionJustReleased;

    // Raw input states
    std::set<SDL_Keycode> m_currentKeys;
    std::set<SDL_Keycode> m_justPressedKeys;
    std::set<SDL_Keycode> m_justReleasedKeys;

    MouseState m_mouseState;
    MouseState m_previousMouseState; // For delta calculation

    GamepadState m_gamepadState;

    bool m_quitRequested = false;

    void loadDefaultBindings();
    void addGamepad(SDL_JoystickID which);
    void removeGamepad(SDL_JoystickID which);
    float normalizeAxisValue(Sint16 rawValue) const;
};

} // namespace TuxArena

#endif // TUXARENA_INPUTMANAGER_H
