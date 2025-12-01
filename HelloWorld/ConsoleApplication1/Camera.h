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

    // ------- [ADD] 카메라 추적 ----------
    void startTracking(glm::vec3 target); // 추적 시작
    void stopTracking();    // 추적 종료

    void updateTargetPosition(glm::vec3 newPos); // 매 프레임 대상의 새로운 위치를 업데이트
    bool getIsTracking() const { return isTracking; } // 추적 상태 확인

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

    // [추가] 추적 관련 변수
    bool isTracking = false;    // 현재 추적 중인가?
    glm::vec3 targetPos;        // 추적 대상의 위치
    float trackDistance = 15.0f; // 대상과의 거리 (줌으로 조절 가능하게 할 예정)

    void updateVectors();
};

#endif
