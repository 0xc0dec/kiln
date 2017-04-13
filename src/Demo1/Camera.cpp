/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>


auto Camera::getProjectionMatrix() const -> const glm::mat4
{
    return ortho
        ? glm::ortho(orthoSize.x, orthoSize.y, nearClip, farClip)
        : glm::perspective(fov, aspectRatio, nearClip, farClip);
}