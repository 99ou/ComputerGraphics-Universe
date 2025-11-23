#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(glm::vec3 startPos);

    glm::mat4 getViewMatrix() const;

    float getFOV() const { return fov; }
    glm::vec3 getPosition() const { return position; }

    // WASD 이동
    void processKeyboard(bool w, bool s, bool a, bool d, float dt);

    // 마우스 이동
    void processMouseMovement(float xpos, float ypos);

    // 마우스 휠
    void processMouseScroll(float yoffset);

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;

    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float lastX;
    float lastY;
    bool firstMouse;

    float moveSpeed;
    float mouseSensitivity;
    float fov;

    void updateVectors();
};

#endif
