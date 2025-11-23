#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 startPos)
    : position(startPos),
    worldUp(0.0f, 1.0f, 0.0f),
    yaw(-90.0f),
    pitch(0.0f),
    lastX(0.0f),
    lastY(0.0f),
    firstMouse(true),
    moveSpeed(40.0f),
    mouseSensitivity(0.1f),
    fov(45.0f)
{
    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(bool w, bool s, bool a, bool d, float dt)
{
    float velocity = moveSpeed * dt;

    if (w)
        position += front * velocity;
    if (s)
        position -= front * velocity;
    if (a)
        position -= right * velocity;
    if (d)
        position += right * velocity;
}

void Camera::processMouseMovement(float xpos, float ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Y는 거꾸로
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateVectors();
}

void Camera::processMouseScroll(float yoffset)
{
    fov -= yoffset;
    if (fov < 10.0f)
        fov = 10.0f;
    if (fov > 90.0f)
        fov = 90.0f;
}

void Camera::updateVectors()
{
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
