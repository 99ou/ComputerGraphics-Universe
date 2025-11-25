#ifndef SATELLITE_H
#define SATELLITE_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Orbit.h"

class Shader;

// 위성(달 등) 파라미터
struct SatelliteParams
{
    std::string name;

    float mass;              // 모행성 대비 질량 비율
    float radiusRender;      // 렌더링 시 크기
    glm::vec3 color;         // 예비 색상(텍스처 없을 때 사용)

    OrbitalElements orbit;   // 모행성 기준 공전 궤도

    float spinDegPerSec;     // 자전 속도

    int trailMaxPoints;      // 궤도 trail 최대 길이

    std::string texturePath; // 🔥 텍스처 경로 (jpg)
};


// 위성 클래스
class Satellite
{
public:
    Satellite(const SatelliteParams& p);

    const SatelliteParams& getParams() const { return params; }

    // 모행성 기준 상대 위치 (케플러 궤도)
    glm::vec3 positionRelativeToPlanet(float tYears) const;

    // trail 기록
    void recordTrail(const glm::vec3& relPos);

    // trail 렌더링
    void drawTrail(const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::mat4& model) const;

    // 위성 렌더
    void drawAtWorld(const Shader& shader,
        float dtSec,
        float scale,
        const glm::vec3& worldPos,
        unsigned int sphereVAO,      // [추가] 어떤 모양을 그릴지 알아야 함
        unsigned int indexCount)     // [추가] 점이 몇 개인지 알아야 함
        const;

    float getMass() const { return params.mass; }

private:
    SatelliteParams params;

    mutable float spinAngleDeg;

    std::vector<glm::vec3> trail;
    int maxTrailPoints;
};

#endif
