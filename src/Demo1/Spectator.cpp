#include "Spectator.h"
#include "Transform.h"
#include "Input.h"
#include <glm/gtx/vector_angle.hpp>


void updateAsSpectator(Transform &transform, Input &input, float dt, float mouseSensitivity, float movementSpeed)
{
    auto mouseMotion = input.getMouseMotion();

    if (input.isMouseButtonDown(SDL_BUTTON_RIGHT, true))
        input.setCursorCaptured(true);
    if (input.isMouseButtonReleased(SDL_BUTTON_RIGHT))
        input.setCursorCaptured(false);

    if (input.isMouseButtonDown(SDL_BUTTON_RIGHT, false))
    {
        if (mouseMotion.x != 0)
            transform.rotateByAxisAngle({0, 1, 0}, mouseSensitivity * dt * -mouseMotion.x, TransformSpace::World);

        if (mouseMotion.y != 0)
        {
            auto angleToUp = glm::angle(glm::normalize(transform.getLocalForward()), glm::vec3(0, 1, 0));
            auto delta = mouseSensitivity * dt * -mouseMotion.y;
            if (delta > 0)
            {
                if (angleToUp - delta <= 0.1f)
                    delta = angleToUp - 0.1f;
            }
            else
            {
                if (angleToUp - delta >= 3.04f)
                    delta = angleToUp - 3.04f;
            }

            transform.rotateByAxisAngle({1, 0, 0}, delta, TransformSpace::Self);
        }
    }

    auto movement = glm::vec3(0);
    if (input.isKeyPressed(SDLK_w, false))
        movement += transform.getLocalForward();
    if (input.isKeyPressed(SDLK_s, false))
        movement += transform.getLocalBack();
    if (input.isKeyPressed(SDLK_a, false))
        movement += transform.getLocalLeft();
    if (input.isKeyPressed(SDLK_d, false))
        movement += transform.getLocalRight();
    if (input.isKeyPressed(SDLK_q, false))
        movement += transform.getLocalDown();
    if (input.isKeyPressed(SDLK_e, false))
        movement += transform.getLocalUp();
    movement = dt * movementSpeed * glm::normalize(movement);

    if (!glm::any(glm::isnan(movement)))
        transform.translateLocal(movement);
}