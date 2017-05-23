#pragma once

class Input;
class Transform;

void applySpectator(Transform &transform, Input &input, float dt, float mouseSensitivity = 0.08f, float movementSpeed = 10);