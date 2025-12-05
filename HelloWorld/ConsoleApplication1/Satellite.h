#ifndef SATELLITE_H
#define SATELLITE_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Orbit.h"

class Shader;

struct SatelliteParams
{
	std::string name; // 위성 이름

	float mass; // 질량
	float radiusRender; // 렌더링 반지름
	glm::vec3 color; // 위성 색상

	OrbitalElements orbit; // 궤도 요소
	float spinDegPerSec; // 자전 속도 (도/초)
	int   trailMaxPoints; // 궤적 최대 점 개수

	std::string texturePath; // 텍스처 파일 경로

    float axialTiltDeg = 0.0f;    // 자전축 기울기
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
		float dtSec,    // 시뮬레이션 시간 델타 (초 단위, 자전용)
		float scale,    // 세계 스케일링 팩터
		const glm::vec3& worldPos, // 이미 스케일링 및 행성 위치가 적용된 월드 위치
		unsigned int sphereVAO, // 구 메쉬 VAO
		unsigned int indexCount) const; // 구 메쉬 인덱스 개수

    float getMass() const { return params.mass; }

private:
	SatelliteParams params; // 위성 파라미터
	mutable float spinAngleDeg; // 자전 각도

	mutable bool generatedOrbit = false; // 궤도 경로 생성 플래그
	mutable std::vector<glm::vec3> orbitPath; // 궤도 경로 점들
};

#endif
