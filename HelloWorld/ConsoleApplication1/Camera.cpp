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
    // [추가] 추적 중이라면, 회전한 각도(front)에 맞춰 위치를 다시 잡아줘야 함 (궤도 회전)
    if (isTracking)
    {
        position = targetPos - (front * trackDistance);
    }
}

void Camera::processMouseScroll(float yoffset)
{
    if (isTracking)
    {
        // 추적 중: 거리 조절 (Zoom In/Out)
        trackDistance -= yoffset * 2.0f; // 감도 조절
        if (trackDistance < 2.0f) trackDistance = 2.0f; // 최소 거리
        if (trackDistance > 100.0f) trackDistance = 100.0f; // 최대 거리

        // 거리 바꿨으니 위치 즉시 갱신
        position = targetPos - (front * trackDistance);
    }
    else {
        fov -= yoffset;
        if (fov < 10.0f)
            fov = 10.0f;
        if (fov > 90.0f)
            fov = 90.0f;
    }
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

// [추가] 추적 시작
void Camera::startTracking(glm::vec3 target)
{
    isTracking = true;
    targetPos = target;

    // 현재 카메라와 대상 사이의 거리를 유지하면서 시작
    trackDistance = glm::distance(position, targetPos);

    // 너무 멀거나 가까우면 적당히 조정 (선택사항)
    if (trackDistance < 5.0f) trackDistance = 10.0f;
}

// [추가] 추적 종료 (자유 시점으로 복귀)
void Camera::stopTracking()
{
    isTracking = false;
}

// [추가] 매 프레임 호출: 대상이 움직이면 카메라도 따라감
void Camera::updateTargetPosition(glm::vec3 newPos)
{
    if (!isTracking) return;

    targetPos = newPos;

    // [핵심 로직]
    // 추적 모드일 때는 '바라보는 방향(front)'의 반대쪽으로 '거리(distance)'만큼 떨어져 있어야 함
    // 즉, 대상(target)을 중심으로 공전하는 위치 계산
    position = targetPos - (front * trackDistance);
}
