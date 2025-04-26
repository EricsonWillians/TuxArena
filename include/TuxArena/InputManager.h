// include/TuxArena/InputManager.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set> // To track current/previous states efficiently

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_mouse.h"

namespace TuxArena {

// Define the possible game actions that can be mapped to input
// Add more actions as needed (reload, use item, next/prev weapon, etc.)
enum class GameAction {
    UNKNOWN, // Default/unassigned
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    SHOOT,
    ALT_SHOOT, // e.g., secondary fire
    SPRINT,
    JUMP, // If applicable
    INTERACT,
    SHOW_SCORES,
    PAUSE_MENU,
    // Add more...
};

// Structure to hold mouse state
struct MouseState {
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    float scrollX = 0.0f; // Horizontal scroll
    float scrollY = 0.0f; // Vertical scroll
    std::set<Uint8> currentButtons; // Set of currently pressed buttons (SDL_BUTTON_LEFT, etc.)
    std::set<Uint8> justPressedButtons;
    std::set<Uint8> justReleasedButtons;
};

// Structure to hold gamepad state (for a single gamepad)
// Expand later to support multiple gamepads if needed
struct GamepadState {
    SDL_Gamepad* instance = nullptr;
    SDL_JoystickID instanceId = 0;
    std::map<SDL_GamepadButton, bool> currentButtons; // Map of button enum -> pressed state
    std::map<SDL_GamepadButton, bool> justPressedButtons;
    std::map<SDL_GamepadButton, bool> justReleasedButtons;
    std::map<SDL_GamepadAxis, float> axisValues; // Map of axis enum -> value (-1.0 to 1.0 after normalization)
    bool isConnected = false;
    float triggerLeft = 0.0f; // Normalized trigger value (0.0 to 1.0)
    float triggerRight = 0.0f;
};


class InputManager {
public:
    InputManager();
    ~InputManager();

    // Prevent copying
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    /**
     * @brief Processes the SDL event queue, updating internal input states.
     * Should be called once per frame before checking input states.
     */
    void pollEvents();

    // --- Action Mapping Queries ---

    /**
     * @brief Checks if a specific game action is currently active (key/button held).
     * @param action The game action to query.
     * @return True if the action is active, false otherwise.
     */
    bool isActionPressed(GameAction action) const;

    /**
     * @brief Checks if a specific game action was activated in the current frame.
     * @param action The game action to query.
     * @return True if the action was just activated, false otherwise.
     */
    bool isActionJustPressed(GameAction action) const;

    /**
     * @brief Checks if a specific game action was deactivated in the current frame.
     * @param action The game action to query.
     * @return True if the action was just deactivated, false otherwise.
     */
    bool isActionJustReleased(GameAction action) const;

    // --- Direct State Queries ---

    /**
     * @brief Gets the current mouse position.
     * @param x Output parameter for the x-coordinate.
     * @param y Output parameter for the y-coordinate.
     */
    void getMousePosition(float& x, float& y) const;

     /**
      * @brief Gets the mouse movement since the last call to pollEvents().
      * @param dx Output parameter for the change in x.
      * @param dy Output parameter for the change in y.
      */
    void getMouseDelta(float& dx, float& dy) const;

     /**
      * @brief Gets the mouse wheel scroll amount since the last call to pollEvents().
      * @param dx Output parameter for the horizontal scroll amount.
      * @param dy Output parameter for the vertical scroll amount.
      */
    void getMouseScrollDelta(float& dx, float& dy) const;

     /**
      * @brief Checks if a specific mouse button is currently held down.
      * @param button SDL button index (e.g., SDL_BUTTON_LEFT).
      * @return True if the button is pressed.
      */
     bool isMouseButtonPressed(Uint8 button) const;

     /**
      * @brief Checks if a specific mouse button was pressed this frame.
      * @param button SDL button index.
      * @return True if the button was just pressed.
      */
     bool isMouseButtonJustPressed(Uint8 button) const;

      /**
       * @brief Checks if a specific mouse button was released this frame.
       * @param button SDL button index.
       * @return True if the button was just released.
       */
      bool isMouseButtonJustReleased(Uint8 button) const;


    /**
     * @brief Checks if a specific keyboard key is currently held down.
     * @param key The SDL_Keycode representing the key.
     * @return True if the key is pressed.
     */
    bool isKeyPressed(SDL_Keycode key) const;

     /**
      * @brief Checks if a specific keyboard key was pressed this frame.
      * @param key The SDL_Keycode representing the key.
      * @return True if the key was just pressed.
      */
    bool isKeyJustPressed(SDL_Keycode key) const;

      /**
       * @brief Checks if a specific keyboard key was released this frame.
       * @param key The SDL_Keycode representing the key.
       * @return True if the key was just released.
       */
    bool isKeyJustReleased(SDL_Keycode key) const;

    /**
     * @brief Checks if a quit event (e.g., closing the window) has been requested.
     * @return True if a quit event occurred, false otherwise.
     */
    bool quitRequested() const { return m_quitRequested; }

    // --- Binding Configuration ---

    /**
     * @brief Loads input bindings from a configuration file (Not implemented yet).
     * @param filePath Path to the bindings configuration file.
     * @return True if loading was successful, false otherwise.
     */
    bool loadBindings(const std::string& filePath);

    /**
     * @brief Binds a keyboard key to a game action.
     * @param key The SDL_Keycode to bind.
     * @param action The GameAction to trigger.
     */
    void bindKey(SDL_Keycode key, GameAction action);

    /**
     * @brief Binds a mouse button to a game action.
     * @param button The SDL mouse button index (e.g., SDL_BUTTON_LEFT).
     * @param action The GameAction to trigger.
     */
    void bindMouseButton(Uint8 button, GameAction action);

    /**
    * @brief Binds a gamepad button to a game action.
    * @param button The SDL_GamepadButton to bind.
    * @param action The GameAction to trigger.
    */
    void bindGamepadButton(SDL_GamepadButton button, GameAction action);

    /**
    * @brief Unbinds a keyboard key.
    * @param key The SDL_Keycode to unbind.
    */
    void unbindKey(SDL_Keycode key);
    // Add unbindMouseButton, unbindGamepadButton similarly...

    // --- Gamepad Specific ---
    /**
     * @brief Gets the current value of a gamepad axis.
     * @param axis The SDL_GamepadAxis to query.
     * @param deadZone The threshold below which the axis value is treated as zero (range 0.0 to 1.0).
     * @return The axis value, normalized between -1.0 and 1.0 (or 0.0 if within deadzone). Returns 0.0 if no gamepad connected.
     */
    float getGamepadAxis(SDL_GamepadAxis axis, float deadZone = 0.15f) const;

    /**
     * @brief Gets the current value of the left trigger.
     * @return The trigger value, normalized between 0.0 and 1.0. Returns 0.0 if no gamepad connected.
     */
    float getGamepadTriggerLeft() const;

    /**
     * @brief Gets the current value of the right trigger.
     * @return The trigger value, normalized between 0.0 and 1.0. Returns 0.0 if no gamepad connected.
     */
    float getGamepadTriggerRight() const;

    /**
    * @brief Checks if a gamepad is currently connected.
    * @return True if a gamepad is connected and recognized.
    */
    bool isGamepadConnected() const;


private:
    // --- Internal State ---
    bool m_quitRequested = false;

    // Keyboard state
    std::set<SDL_Keycode> m_currentKeys;
    std::set<SDL_Keycode> m_justPressedKeys;
    std::set<SDL_Keycode> m_justReleasedKeys;

    // Mouse state
    MouseState m_mouseState;
    MouseState m_previousMouseState; // To calculate delta

    // Gamepad state (simplified to one gamepad for now)
    GamepadState m_gamepadState;
    std::map<SDL_JoystickID, SDL_Gamepad*> m_activeGamepads; // Manage multiple later

    // --- Bindings ---
    std::map<SDL_Keycode, GameAction> m_keyBindings;
    std::map<Uint8, GameAction> m_mouseButtonBindings;
    std::map<SDL_GamepadButton, GameAction> m_gamepadButtonBindings;

    // --- Action States ---
    // Updated based on bindings and current input states
    std::map<GameAction, bool> m_actionPressed;
    std::map<GameAction, bool> m_actionJustPressed;
    std::map<GameAction, bool> m_actionJustReleased;

    // --- Helper Methods ---
    /**
     * @brief Resets the "just pressed/released" states at the start of a frame.
     */
    void clearTransientStates();

    /**
     * @brief Updates the state of all mapped actions based on current input.
     */
    void updateActionStates();

    /**
     * @brief Handles adding a new gamepad device.
     * @param which The SDL_JoystickID of the device.
     */
    void addGamepad(SDL_JoystickID which);

     /**
      * @brief Handles removing a gamepad device.
      * @param which The SDL_JoystickID of the device instance.
      */
    void removeGamepad(SDL_JoystickID which);

    /**
     * @brief Sets up default input bindings.
     */
    void loadDefaultBindings();

    /**
     * @brief Normalizes raw axis value from SDL to -1.0 to 1.0 range.
     */
    float normalizeAxisValue(Sint16 rawValue) const;
};

} // namespace TuxArena