#pragma once

#include <glm/vec2.hpp>
#include <SDL.h>
#include <unordered_map>
#include <unordered_set>

class Input
{
public:
    void beginUpdate(SDL_Window *window);
    void processEvent(const SDL_Event &evt);

    void setCursorCaptured(bool captured);

    bool isKeyPressed(SDL_Keycode code, bool firstTime = false) const;
    bool isKeyReleased(SDL_Keycode code) const;

    auto getMouseMotion() const -> glm::vec2;
    bool isMouseButtonDown(uint8_t button, bool firstTime = false) const;
    bool isMouseButtonReleased(uint8_t button) const;

private:
    SDL_Window *window = nullptr;

    bool hasMouseFocus = false;
    bool hasKeyboardFocus = false;
    bool firstUpdate = true;

    std::unordered_map<SDL_Keycode, bool> pressedKeys;
    std::unordered_set<SDL_Keycode> releasedKeys;

    int32_t mouseDeltaX = 0;
    int32_t mouseDeltaY = 0;
    std::unordered_map<uint8_t, bool> pressedMouseButtons;
    std::unordered_set<uint8_t> releasedMouseButtons;

    void prepareMouseState();
    void prepareKeyboardState();
    void readWindowState(SDL_Window *window);
    void processKeyboardEvent(const SDL_Event &evt);
    void processMouseEvent(const SDL_Event &evt);
};
