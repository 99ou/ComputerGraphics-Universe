#ifndef PLANET_H
#define PLANET_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Orbit.h"
#include "Satellite.h"

class Shader;

struct PlanetParams
{
    std::string name;

    float mass;            // 태양 대비 질량 비율
    float radiusRender;    // 렌더링시 표시할 실제 반지름
    glm::vec3 color;       // 필요 시 보조 색상

    OrbitalElements orbit; // 태양 기준 공전 궤도

    float spinDegPerSec;   // 자전 속도

    std::string texturePath; // 🔥 행성 텍스처 경로 (jpg)
};

class Planet
{
public:
    Planet(const PlanetParams& p);

    const PlanetParams& getParams() const { return params; }

    void addSatellite(const Satellite& s);

    std::vector<Satellite>& satellites() { return sats; }
    const std::vector<Satellite>& satellites() const { return sats; }

    float getMass() const { return params.mass; }

    // 태양 기준 위치 (tYears: 시뮬레이션 시간)
    glm::vec3 positionAroundSun(float tYears) const;

    // 행성 trail 기록
    void recordTrail(const glm::vec3& heliocentricPos);

    // trail 렌더
    void drawTrail(const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& proj) const;

    // 자전
    void advanceSpin(float dtSec) const;

    // 행성 렌더링을 위한 model matrix 생성
    glm::mat4 buildModelMatrix(float scale,
        const glm::vec3& worldPos) const;

private:
    PlanetParams params;
    std::vector<Satellite> sats;

    mutable float spinAngleDeg;

    std::vector<glm::vec3> trail;  // 태양 기준 궤적
    int maxTrailPoints;
};

#endif
