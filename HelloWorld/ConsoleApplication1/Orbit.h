#ifndef ORBIT_H
#define ORBIT_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>

// =============================
// 케플러 궤도 요소 정의
// =============================
struct OrbitalElements
{
    float semiMajorAxis;          // 긴반지름 (a)
    float eccentricity;           // 이심률 (e)
    float inclinationDeg;         // 경사각 (i, degree)
    float ascNodeDeg;             // 승교점 경도 Ω (degree, at epoch)
    float argPeriDeg;             // 근일점 인수 ω (degree, at epoch)
    float periodYears;            // 공전 주기 (년)
    float meanAnomalyAtEpochDeg;  // 평균근점이각 M0 (degree)
    float perihelionPrecessionDegPerYear = 0.0f; // 근일점 인수 ω 의 연간 변화량 (deg/year)
    float ascNodePrecessionDegPerYear = 0.0f;    // 승교점 경도 Ω 의 연간 변화량 (deg/year)

    // ------------------------------------
    // tYears: Epoch 기준 경과 시간(년 단위)
    // ------------------------------------
    glm::vec3 positionAtTime(float tYears) const
    {
        using std::sin;
        using std::cos;

        const float TWO_PI = glm::two_pi<float>();

        // -----------------------------
        // 1) 이심률 클램프
        // -----------------------------
        float e = eccentricity;
        if (e < 0.0f)  e = 0.0f;
        if (e >= 1.0f) e = 0.99f;

        // -----------------------------
        // 2) 평균 근점 이각 M(t)
        // -----------------------------
        float n = TWO_PI / periodYears;                    // 평균운동 (rad/year)
        float M0 = glm::radians(meanAnomalyAtEpochDeg);     // 초기 M0 (rad)
        float M = M0 + n * tYears;                         // 시간 t 에서의 M

        // -π ~ +π 범위로 정규화
        M = fmod(M, TWO_PI);
        if (M < -glm::pi<float>()) M += TWO_PI;
        if (M > glm::pi<float>()) M -= TWO_PI;

        // -----------------------------
        // 3) 케플러 방정식 (Newton-Raphson) → 편심이각 E
        // -----------------------------
		float E = M; // 초기 추정값
        for (int i = 0; i < 6; ++i)
        {
			// Newton-Raphson 반복
            float f = E - e * sin(E) - M;
            float fp = 1.0f - e * cos(E);
            E -= f / fp;
        }

        float cosE = cos(E);
        float sinE = sin(E);
		float sqrtOneMinusEsq = std::sqrt(1.0f - e * e); // √(1 - e²)

        // -----------------------------
        // 4) 편심이각 → 진근점이각 v, 거리 r
        // -----------------------------
		float cosV = (cosE - e) / (1.0f - e * cosE);               // 진근점이각 코사인
		float sinV = (sqrtOneMinusEsq * sinE) / (1.0f - e * cosE); // 진근점이각 사인

        float v = std::atan2(sinV, cosV);            // 진근점이각
        float r = semiMajorAxis * (1.0f - e * cosE); // 거리

        // -----------------------------
        // 5) 세차를 포함한 궤도 요소 시간 진화
        //    Ω(t) = Ω0 + Ω̇ * t
        //    ω(t) = ω0 + ω̇ * t
        // -----------------------------
		float iDeg = inclinationDeg; // 경사각은 일정하다고 가정
		float ODeg = ascNodeDeg + ascNodePrecessionDegPerYear * tYears; // 승교점 경도
		float wDeg = argPeriDeg + perihelionPrecessionDegPerYear * tYears; // 근일점 인수

		// 각도를 라디안으로 변환
        float i = glm::radians(iDeg);
        float O = glm::radians(ODeg);
        float w = glm::radians(wDeg);

		// 진근점이각 + 근일점 인수
        float theta = w + v;

        float cosO = cos(O), sinO = sin(O);
        float cosI = cos(i), sinI = sin(i);
        float cosT = cos(theta), sinT = sin(theta);

        // -----------------------------
        // 6) 궤도면 좌표 → 관성 좌표계 변환
        // -----------------------------
        float x = r * (cosO * cosT - sinO * sinT * cosI);
        float y = r * (sinO * cosT + cosO * sinT * cosI);
        float z = r * (sinT * sinI);

        return glm::vec3(x, y, z);
    }
};

// =============================
// 궤도 전체 경로 생성 유틸
//  - 행성/위성 공통으로 사용
// =============================
inline std::vector<glm::vec3> buildOrbitPath(const OrbitalElements& orbit)
{
    const int SAMPLES = 3600;   // 0.1 단위 샘플링
    std::vector<glm::vec3> pts;
    pts.reserve(SAMPLES);

    for (int i = 0; i < SAMPLES; i++)
    {
        float ratio = (float)i / (float)SAMPLES;
        float t = orbit.periodYears * ratio;  // 한 주기(=periodYears) 동안의 위치
        pts.push_back(orbit.positionAtTime(t));
    }
    return pts;
}

#endif
