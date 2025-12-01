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
    float inclinationDeg;         // 경사각 (도)
    float ascNodeDeg;             // 승교점 경도 Ω (도)
    float argPeriDeg;             // 근일점 인수 ω (도)
    float periodYears;            // 공전 주기 (년)
    float meanAnomalyAtEpochDeg;  // 평균근점이각 M0 (도)

    glm::vec3 positionAtTime(float tYears) const
    {
        using std::sin;
        using std::cos;

        const float TWO_PI = glm::two_pi<float>();

        float e = eccentricity;
        if (e < 0.0f)  e = 0.0f;
        if (e >= 1.0f) e = 0.99f;

        // 평균 근점 이각
        float n = TWO_PI / periodYears;
        float M0 = glm::radians(meanAnomalyAtEpochDeg);
        float M = M0 + n * tYears;

        M = fmod(M, TWO_PI);
        if (M < -glm::pi<float>()) M += TWO_PI;
        if (M > glm::pi<float>()) M -= TWO_PI;

        // 케플러 방정식 Newton 방법
        float E = M;
        for (int i = 0; i < 6; ++i)
        {
            float f = E - e * sin(E) - M;
            float fp = 1.0f - e * cos(E);
            E -= f / fp;
        }

        float cosE = cos(E);
        float sinE = sin(E);
        float sqrtOneMinusEsq = sqrt(1.0f - e * e);

        float cosV = (cosE - e) / (1.0f - e * cosE);
        float sinV = (sqrtOneMinusEsq * sinE) / (1.0f - e * cosE);

        float v = atan2(sinV, cosV);
        float r = semiMajorAxis * (1.0f - e * cosE);

        float i = glm::radians(inclinationDeg);
        float O = glm::radians(ascNodeDeg);
        float w = glm::radians(argPeriDeg);
        float theta = w + v;

        float cosO = cos(O), sinO = sin(O);
        float cosI = cos(i), sinI = sin(i);
        float cosT = cos(theta), sinT = sin(theta);

        float x = r * (cosO * cosT - sinO * sinT * cosI);
        float y = r * (sinO * cosT + cosO * sinT * cosI);
        float z = r * (sinT * sinI);

        glm::vec3 pos(x, y, z);
        return pos;
    }
};

// =============================
// 궤도 전체 경로 생성 유틸
//  - 행성/위성 공통으로 사용
// =============================
inline std::vector<glm::vec3> buildOrbitPath(const OrbitalElements& orbit)
{
    const int SAMPLES = 3600;   // 0.1 단위
    std::vector<glm::vec3> pts;
    pts.reserve(SAMPLES);

    for (int i = 0; i < SAMPLES; i++)
    {
        float ratio = (float)i / (float)SAMPLES;
        float t = orbit.periodYears * ratio;
        pts.push_back(orbit.positionAtTime(t));
    }
    return pts;
}

#endif
