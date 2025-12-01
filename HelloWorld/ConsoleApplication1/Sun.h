#ifndef SUN_H
#define SUN_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Planet.h"

class Sun
{
public:
    Sun();

    void addPlanet(const Planet& p);

    std::vector<Planet>& getPlanets() { return planets; }
    const std::vector<Planet>& getPlanets() const { return planets; }

    float getMass() const { return mass; }

    // ⭐ 자전 업데이트
    void advanceSpin(float dtSec) const;

    // ⭐ 태양 렌더링용 model matrix 생성
    glm::mat4 buildModelMatrix(float scale) const;

    // ⭐ 태양 자전 속도 설정
    void setSpinSpeed(float degPerSec) { spinDegPerSec = degPerSec; }

private:
    float mass;                    // 태양 질량
    std::vector<Planet> planets;

    mutable float spinAngleDeg;    // ⭐ 누적 자전 각도
    float spinDegPerSec;           // ⭐ 초당 회전 속도 (deg/sec)
};

#endif
