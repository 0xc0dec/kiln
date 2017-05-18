#include "Input.h"

void Input::processEvent(const SDL_Event &evt)
{
    if (!firstUpdate)
    {
        processKeyboardEvent(evt);
        processMouseEvent(evt);
    }
}

void Input::setCursorCaptured(bool captured)
{
    SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
}

void Input::prepareMouseState()
{
    mouseDeltaX = mouseDeltaY = 0;
    releasedMouseButtons.clear();
    if (hasMouseFocus)
    {
        for (const auto &pair : pressedMouseButtons)
            pressedMouseButtons[pair.first] = false;
    }
    else
    {
        for (const auto &pair : pressedMouseButtons)
            releasedMouseButtons.insert(pair.first);
        pressedMouseButtons.clear();
    }
}

void Input::prepareKeyboardState()
{
    releasedKeys.clear();
    if (hasKeyboardFocus)
    {
        for (auto &pair : pressedKeys)
            pair.second = false; // not "pressed for the first time" anymore
    }
    else
    {
        for (const auto &pair : pressedKeys)
            releasedKeys.insert(pair.first);
        pressedKeys.clear();
    }
}

void Input::readWindowState(SDL_Window *window)
{
    auto flags = SDL_GetWindowFlags(window);
    hasKeyboardFocus = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    hasMouseFocus = (flags & SDL_WINDOW_MOUSE_FOCUS) != 0;
}

void Input::processKeyboardEvent(const SDL_Event &evt)
{
    if (!hasKeyboardFocus)
        return;

    switch (evt.type)
    {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
        {
            auto code = evt.key.keysym.sym;
            if (evt.type == SDL_KEYUP)
            {
                releasedKeys.insert(code);
                pressedKeys.erase(code);
            }
            else
            {
                pressedKeys[code] = pressedKeys.find(code) == pressedKeys.end(); // first time?
                releasedKeys.erase(code);
            }
            break;
        }
        default:
            break;
    }
}

void Input::processMouseEvent(const SDL_Event &evt)
{
    if (!hasMouseFocus)
        return;

    switch (evt.type)
    {
        case SDL_MOUSEMOTION:
            mouseDeltaX += evt.motion.xrel;
            mouseDeltaY += evt.motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN:
        {
            auto btn = evt.button.button;
            pressedMouseButtons[btn] = true; // pressed for the first time
            releasedMouseButtons.erase(btn);
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            auto btn = evt.button.button;
            if (pressedMouseButtons.find(btn) != pressedMouseButtons.end())
            {
                releasedMouseButtons.insert(btn);
                pressedMouseButtons.erase(btn);
            }
            break;
        }
        default:
            break;
    }
}

void Input::beginUpdate(SDL_Window *window)
{
    readWindowState(window);
    prepareMouseState();
    prepareKeyboardState();
    firstUpdate = false;
}

bool Input::isKeyPressed(SDL_Keycode code, bool firstTime) const
{
    auto where = pressedKeys.find(code);
    return where != pressedKeys.end() && (!firstTime || where->second);
}

bool Input::isKeyReleased(SDL_Keycode code) const
{
    return releasedKeys.find(code) != releasedKeys.end();
}

auto Input::getMouseMotion() const -> glm::vec2
{
    return {static_cast<float>(mouseDeltaX), static_cast<float>(mouseDeltaY)};
}

bool Input::isMouseButtonDown(uint8_t button, bool firstTime) const
{
    auto where = pressedMouseButtons.find(button);
    return where != pressedMouseButtons.end() && (!firstTime || where->second);
}

bool Input::isMouseButtonReleased(uint8_t button) const
{
    return releasedMouseButtons.find(button) != releasedMouseButtons.end();
}