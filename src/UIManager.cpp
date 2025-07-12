#include "TuxArena/UIManager.h"
#include "TuxArena/UI.h"
#include "TuxArena/Log.h"
#include "TuxArena/Constants.h"
#include "TuxArena/Game.h"
#include "TuxArena/NetworkClient.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <algorithm> // For std::min/max

namespace TuxArena {

UIManager::UIManager(Game* game) : m_game(game), m_lastUIEvent({UIEvent::NONE, "", 0}) {
    Log::Info("UIManager created.");
}

UIManager::~UIManager() {
    Log::Info("UIManager destroyed.");
}

bool UIManager::initialize(SDL_Window* window, SDL_Renderer* renderer) {
    if (!window || !renderer) {
        Log::Error("UIManager: SDL_Window or SDL_Renderer is null.");
        return false;
    }

    m_sdlWindow = window;
    m_sdlRenderer = renderer;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    UI::SetGothicTheme(); // Apply custom theme

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_sdlWindow, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 130");

    // Load default font
    io.Fonts->AddFontFromFileTTF("assets/fonts/nokia.ttf", 18.0f);
    if (io.Fonts->Fonts.empty()) {
        Log::Error("UIManager: Failed to load default font. ImGui might crash.");
    } else {
        Log::Info("UIManager: Default font loaded.");
    }

    Log::Info("UIManager initialized successfully.");
    return true;
}

void UIManager::shutdown() {
    Log::Info("Shutting down UIManager...");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    m_sdlWindow = nullptr;
    m_sdlRenderer = nullptr;
    Log::Info("UIManager shutdown complete.");
}

void UIManager::processSDLEvent(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

UIEventData UIManager::getAndClearUIEvent() {
    UIEventData event = m_lastUIEvent;
    m_lastUIEvent = {UIEvent::NONE, "", 0}; // Clear the event after reading
    return event;
}

void UIManager::setNetworkStatus(const std::string& status) {
    m_networkStatus = status;
}

void UIManager::setCharacterSelectionData(const std::vector<std::string>& characterNames, int selectedCharIndex) {
    m_characterNames = characterNames;
    m_selectedCharacterIndex = selectedCharIndex;
}

void UIManager::setupCenteredWindow(int windowWidth, int windowHeight, float widthRatio, float heightRatio) {
    ImGui::SetNextWindowPos(
        ImVec2(windowWidth * 0.5f, windowHeight * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::SetNextWindowSize(
        ImVec2(windowWidth * widthRatio, windowHeight * heightRatio),
        ImGuiCond_Always
    );
}

void UIManager::pushCommonButtonStyles() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.15f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.2f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.2f, 1.0f));
}

void UIManager::popCommonButtonStyles() {
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}

void UIManager::pushCommonWindowStyles() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.05f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
}

void UIManager::popCommonWindowStyles() {
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void UIManager::render(GameState currentState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    switch (currentState) {
        case GameState::PLAYING: {
            // Render network status text
            std::string networkStatus = "Offline";
            if (m_game->getNetworkClient()) {
                networkStatus = m_game->getNetworkClient()->getStatusString();
            }
            else if (m_game->getNetworkServer()) {
                networkStatus = "Server Running";
            }
            m_game->getRenderer()->drawText(networkStatus, 10, 10, 
                               "assets/fonts/nokia.ttf", 
                               16, {255, 255, 255, 255});
            break;
        }
        
        case GameState::CHARACTER_SELECTION: {
            if (m_game->getCharacterManager()) {
                // Get character names for UI
                std::vector<std::string> charNames;
                for (const auto& character : m_game->getCharacterManager()->getAvailableCharacters()) {
                    charNames.push_back(character.name);
                }
                setCharacterSelectionData(charNames, m_selectedCharacterIndex);
                renderCharacterSelection(m_game->getConfig().windowWidth, m_game->getConfig().windowHeight);
            }
            break;
        }
        
        case GameState::MAIN_MENU: {
            renderMainMenu(m_game->getConfig().windowWidth, m_game->getConfig().windowHeight);
            break;
        }
        
        case GameState::LOBBY: {
            renderLobby(m_game->getConfig().windowWidth, m_game->getConfig().windowHeight, m_game->getAvailableMaps(), m_game->getSelectedMapIndex());
            break;
        }
        
        case GameState::CONNECT_TO_SERVER: {
            renderConnectToServer(m_game->getConfig().windowWidth, m_game->getConfig().windowHeight, m_game->getServerIpBuffer(), m_game->getConfig().serverPort);
            break;
        }

        case GameState::PAUSED: {
            renderPauseMenu(m_game->getConfig().windowWidth, m_game->getConfig().windowHeight);
            break;
        }
        
        default: {
            // Handle all other non-playing states
            std::string statusText;
            bool showReturnToMainMenuButton = false;
            switch (currentState) {
                case GameState::INITIALIZING:
                    statusText = "Initializing game systems...";
                    break;
                case GameState::LOADING:
                    statusText = "Loading...";
                    if (m_game->getNetworkClient()) {
                        statusText += " " + m_game->getNetworkClient()->getStatusString();
                    }
                    break;
                case GameState::ERROR_STATE:
                    statusText = "Error: ";
                    if (m_game->getNetworkClient()) {
                        statusText += m_game->getNetworkClient()->getStatusString();
                    } else {
                        statusText += "Initialization failed";
                    }
                    showReturnToMainMenuButton = true;
                    break;
                case GameState::SHUTTING_DOWN:
                    statusText = "Shutting down...";
                    break;
                default:
                    statusText = "Unknown State";
                    break;
            }
            renderNonPlayingState(m_game->getConfig().windowWidth, m_game->getConfig().windowHeight, statusText, showReturnToMainMenuButton);
            break;
        }
    }
    
    // Render ImGui elements
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::clear() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::renderPauseMenu(int windowWidth, int windowHeight) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

    pushCommonWindowStyles();
    setupCenteredWindow(windowWidth, windowHeight, 0.4f, 0.6f);

    ImGui::Begin("Pause Menu", nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);

    const float buttonWidth = ImGui::GetContentRegionAvail().x;
    const float buttonHeight = 50.0f;

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);

    const char* title = "PAUSED";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
    ImGui::Text("%s", title);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40.0f);

    pushCommonButtonStyles();
    if (ImGui::Button("Resume", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Resume button clicked");
        m_lastUIEvent = {UIEvent::RESUME_GAME_CLICKED, "", 0};
    }
    ImGui::Spacing();
    if (ImGui::Button("Options", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Options button clicked");
        // m_lastUIEvent = {UIEvent::OPTIONS_CLICKED, "", 0};
    }
    ImGui::Spacing();
    if (ImGui::Button("Quit to Main Menu", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Quit to Main Menu button clicked");
        m_lastUIEvent = {UIEvent::BACK_TO_MAIN_MENU_CLICKED, "", 0};
    }
    popCommonButtonStyles();

    ImGui::End();
    popCommonWindowStyles();
}

void UIManager::renderMainMenu(int windowWidth, int windowHeight) {

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

    pushCommonWindowStyles();
    setupCenteredWindow(windowWidth, windowHeight, 0.4f, 0.6f);

    ImGui::Begin("Main Menu", nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);

    const float buttonWidth = ImGui::GetContentRegionAvail().x;
    const float buttonHeight = 50.0f;

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);

    const char* title = "TUX ARENA";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
    ImGui::Text("%s", title);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40.0f);

    pushCommonButtonStyles();
    if (ImGui::Button("Host Game", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Host Game button clicked");
        m_lastUIEvent = {UIEvent::HOST_GAME_CLICKED, "", 0};
    }
    ImGui::Spacing();
    if (ImGui::Button("Connect to Server", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Connect to Server button clicked");
        m_lastUIEvent = {UIEvent::CONNECT_TO_SERVER_CLICKED, "", 0};
    }
    ImGui::Spacing();
    if (ImGui::Button("Quit", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Quit button clicked");
        m_lastUIEvent = {UIEvent::QUIT_CLICKED, "", 0};
    }
    popCommonButtonStyles();

    ImGui::End();
    popCommonWindowStyles();
}

void UIManager::renderLobby(int windowWidth, int windowHeight, const std::vector<std::string>& availableMaps, int& selectedMapIndex) {

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

    pushCommonWindowStyles();
    setupCenteredWindow(windowWidth, windowHeight, 0.6f, 0.8f);

    ImGui::Begin("Lobby", nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
    const char* title = "GAME LOBBY";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
    ImGui::Text("%s", title);
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Game Settings");
    ImGui::Spacing();

    ImGui::Text("Select Map:");
    if (availableMaps.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                         "No maps found in the 'maps' directory!");
    } else {
        if (selectedMapIndex >= (int)availableMaps.size()) {
            selectedMapIndex = 0;
        }
        const char* current_map = availableMaps[selectedMapIndex].c_str();
        if (ImGui::BeginCombo("##map_combo", current_map)) {
            for (size_t i = 0; i < availableMaps.size(); i++) {
                const bool is_selected = (selectedMapIndex == static_cast<int>(i));
                if (ImGui::Selectable(availableMaps[i].c_str(), is_selected)) {
                    selectedMapIndex = i;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Server Port: %d", DEFAULT_SERVER_PORT);
    ImGui::Text("Max Players: %d", MAX_PLAYERS);

    ImGui::Spacing();
    ImGui::Spacing();

    const float buttonWidth = ImGui::GetContentRegionAvail().x;
    const float buttonHeight = 40.0f;

    ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 110.0f);

    pushCommonButtonStyles();
    if (ImGui::Button("Start Game", ImVec2(buttonWidth, buttonHeight))) {
        if (!availableMaps.empty()) {
            m_lastUIEvent = {UIEvent::START_GAME_CLICKED, availableMaps[selectedMapIndex], 0};
        } else {
            Log::Warning("Cannot start game: no maps available");
        }
    }
    ImGui::Spacing();
    if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Returning to main menu from lobby");
        m_lastUIEvent = {UIEvent::BACK_TO_MAIN_MENU_CLICKED, "", 0};
    }
    popCommonButtonStyles();

    ImGui::End();
    popCommonWindowStyles();
}

void UIManager::renderConnectToServer(int windowWidth, int windowHeight, char* serverIpBuffer, int serverPort) {


    pushCommonWindowStyles();
    setupCenteredWindow(windowWidth, windowHeight, 0.3f, 0.3f);

    ImGui::Begin("Connect to Server", nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
    const char* title = "CONNECT TO SERVER";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
    ImGui::Text("%s", title);
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Server IP:");
    ImGui::InputText("##server_ip", serverIpBuffer, 256); // Use 256 as buffer size

    ImGui::Text("Port: %d", serverPort);

    ImGui::Spacing();
    ImGui::Spacing();

    const float buttonWidth = ImGui::GetContentRegionAvail().x;
    const float buttonHeight = 40.0f;

    ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 110.0f);

    pushCommonButtonStyles();
    if (ImGui::Button("Connect", ImVec2(buttonWidth, buttonHeight))) {
        m_lastUIEvent = {UIEvent::CONNECT_ATTEMPT, serverIpBuffer, 0}; // Pass IP as stringValue
    }
    ImGui::Spacing();
    if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Returning to main menu from connect screen");
        m_lastUIEvent = {UIEvent::BACK_TO_MAIN_MENU_CLICKED, "", 0};
    }
    popCommonButtonStyles();

    ImGui::End();
    popCommonWindowStyles();
}

void UIManager::renderCharacterSelection(int windowWidth, int windowHeight) {
    pushCommonWindowStyles();
    setupCenteredWindow(windowWidth, windowHeight, 0.7f, 0.8f);

    ImGui::Begin("Character Selection", nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
    const char* title = "SELECT YOUR MASCOT";
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
    ImGui::Text("%s", title);
    ImGui::Separator();
    ImGui::Spacing();

    // Character grid/list
    if (m_characterNames.empty()) {
        ImGui::Text("No characters available.");
    } else {
        // Ensure selected index is valid
        m_selectedCharacterIndex = static_cast<int>(std::min(static_cast<size_t>(m_selectedCharacterIndex), m_characterNames.size() - 1));
            m_selectedCharacterIndex = static_cast<int>(std::max(0, m_selectedCharacterIndex));

        // Simple list for now, can be expanded to a grid later
        ImGui::BeginChild("##character_list", ImVec2(0, ImGui::GetContentRegionAvail().y - 100), true);
        for (size_t i = 0; i < m_characterNames.size(); ++i) {
            if (ImGui::Selectable(m_characterNames[i].c_str(), m_selectedCharacterIndex == (int)i)) {
                m_selectedCharacterIndex = i;
                m_lastUIEvent = {UIEvent::CHARACTER_SELECTED, m_characterNames[i], (int)i}; // Pass name and index
            }
        }
        ImGui::EndChild();
    }

    ImGui::Spacing();

    const float buttonWidth = ImGui::GetContentRegionAvail().x;
    const float buttonHeight = 40.0f;

    pushCommonButtonStyles();
    if (ImGui::Button("Start Game", ImVec2(buttonWidth, buttonHeight))) {
        if (!m_characterNames.empty()) {
            m_lastUIEvent = {UIEvent::START_GAME_CLICKED, m_characterNames[m_selectedCharacterIndex], m_selectedCharacterIndex};
        } else {
            Log::Warning("Cannot start game: no character selected");
        }
    }
    ImGui::Spacing();
    if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight))) {
        Log::Info("Returning to main menu from character selection");
        m_lastUIEvent = {UIEvent::BACK_TO_MAIN_MENU_CLICKED, "", 0};
    }
    popCommonButtonStyles();

    ImGui::End();
    popCommonWindowStyles();
}

void UIManager::renderNonPlayingState(int windowWidth, int windowHeight, const std::string& statusText, bool showReturnToMainMenuButton) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

    pushCommonWindowStyles();
    setupCenteredWindow(windowWidth, windowHeight, 0.4f, 0.2f); // Smaller window for status

    ImGui::Begin("Game Status", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s", statusText.c_str());

    if (showReturnToMainMenuButton) {
        ImGui::Spacing();
        if (ImGui::Button("Return to Main Menu")) {
            Log::Info("Return to main menu button clicked");
            m_lastUIEvent = {UIEvent::BACK_TO_MAIN_MENU_CLICKED, "", 0};
        }
    }

    ImGui::End();
    popCommonWindowStyles();
}

} // namespace TuxArena
