#include "Sun.h"

Sun::Sun()
    : mass(1.0f),
    spinAngleDeg(0.0f),
    spinDegPerSec(5.0f)   // ⭐ 기본 자전 속도 (원하는 값으로 조절)
{
}

void Sun::addPlanet(const Planet& p)
{
    planets.push_back(p);
}

void Sun::advanceSpin(float dtSec) const
{
    spinAngleDeg += spinDegPerSec * dtSec;
}

glm::mat4 Sun::buildModelMatrix(float scale) const
{
    glm::mat4 model(1.0f);

    // 태양은 (0,0,0)에 고정되어 있음
    model = glm::translate(model, glm::vec3(0.0f));

    // ⭐ 자전 적용
    model = glm::rotate(model,
        glm::radians(spinAngleDeg),
        glm::vec3(0, 1, 0));

    // ⭐ 크기 조절
    model = glm::scale(model, glm::vec3(scale));

    return model;
}
