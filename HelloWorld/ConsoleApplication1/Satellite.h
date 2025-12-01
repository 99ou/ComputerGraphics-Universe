#ifndef SATELLITE_H
#define SATELLITE_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Orbit.h"

class Shader;

struct SatelliteParams
{
    std::string name;

    float mass;
    float radiusRender;
    glm::vec3 color;

    OrbitalElements orbit;
    float spinDegPerSec;
    int   trailMaxPoints;

    std::string texturePath;
};

class Satellite
{
public:
    Satellite(const SatelliteParams& p);

    const SatelliteParams& getParams() const { return params; }

    glm::vec3 positionRelativeToPlanet(float tYears) const;

    float orbitProgress(float tYears) const;

    void drawTrail(const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::mat4& planetModel,
        float tYears) const;

    void drawAtWorld(const Shader& shader,
        float dtSec,
        float scale,
        const glm::vec3& worldPos,
        unsigned int sphereVAO,
        unsigned int indexCount) const;

    float getMass() const { return params.mass; }

private:
    SatelliteParams params;
    mutable float spinAngleDeg;

    mutable bool generatedOrbit = false;
    mutable std::vector<glm::vec3> orbitPath;
};

#endif
