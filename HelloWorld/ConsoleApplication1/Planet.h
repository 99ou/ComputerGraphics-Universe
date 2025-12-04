#ifndef PLANET_H
#define PLANET_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Orbit.h"
#include "Satellite.h"
#include "planetRing.h"

class Shader;

// =====================================================
// ⭐ RingParams — PlanetParams보다 먼저 선언
// =====================================================
struct RingParams {
	bool enabled = false;    // 고리 활성화 여부
	float innerRadius = 1.0f;// 고리 내경
	float outerRadius = 1.5f;// 고리 외경
	float alpha = 1.0f;      // 고리 투명도

    unsigned int textureID = 0;   // 실제 OpenGL 텍스처 ID
    std::string texturePath;      // main.cpp에서 경로만 세팅
};

// =====================================================
// ⭐ PlanetParams
// =====================================================
struct PlanetParams
{
	std::string name;          // 행성 이름
	float mass;                // 질량
	float radiusRender;        // 렌더링 반지름
	glm::vec3 color;           // 색상
	OrbitalElements orbit;     // 궤도 요소
	float spinDegPerSec;       // 자전 속도 (도/초)
	std::string texturePath;   // 텍스처 경로
	float axialTiltDeg = 0.0f; // 자전축 기울기 (도)

    RingParams ring;           // 고리 정보
};

class Planet
{
public:
    Planet(const PlanetParams& p);

    const PlanetParams& getParams() const { return params; }

    void addSatellite(const Satellite& s);
    std::vector<Satellite>& satellites() { return sats; }
    const std::vector<Satellite>& satellites() const { return sats; }

	// 질량 반환 헬퍼
    float getMass() const { return params.mass; }

	// 행성의 태양 주위 위치 계산
    glm::vec3 positionAroundSun(float tYears) const;
	// 행성의 자전축 방향 계산
    float orbitProgress(float tYears) const;

	// 행성 궤도 그리기
    void drawTrail(const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& proj,
        float tYears) const;

	// 자전 업데이트
    void advanceSpin(float dtSec) const;

	// 행성 모델 매트릭스 생성
    glm::mat4 buildModelMatrix(float scale,
        const glm::vec3& worldPos) const;

    // 고리 객체 포인터 (main에서 바로 접근 가능하도록)
    PlanetRing* ring = nullptr;

private:
    PlanetParams params;
    std::vector<Satellite> sats;

	mutable float spinAngleDeg;               // 자전 각도 (도)
	mutable bool generatedOrbit = false;      // 궤도 경로 생성 여부
	mutable std::vector<glm::vec3> orbitPath; // 궤도 경로 점들
};

#endif