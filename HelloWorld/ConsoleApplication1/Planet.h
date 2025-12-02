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

    float mass;
    float radiusRender;
    glm::vec3 color;

    OrbitalElements orbit;
    float spinDegPerSec;

    std::string texturePath;
    float axialTiltDeg = 0.0f; // 자전축 기울기
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

    glm::vec3 positionAroundSun(float tYears) const;

    // 진행도 (0~1)
    float orbitProgress(float tYears) const;

    // 흰색 전체 궤도 + 초록색 진행 궤도
    void drawTrail(const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& proj,
        float tYears) const;

    void advanceSpin(float dtSec) const;

    glm::mat4 buildModelMatrix(float scale,
        const glm::vec3& worldPos) const;

private:
    PlanetParams params;
    std::vector<Satellite> sats;

    mutable float spinAngleDeg;

    // orbitPath 캐시
    mutable bool generatedOrbit = false;
    mutable std::vector<glm::vec3> orbitPath;
};

#endif
