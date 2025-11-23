#ifndef ORBIT_H
#define ORBIT_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

// 케플러 궤도 요소
struct OrbitalElements
{
    float semiMajorAxis;          // a
    float eccentricity;           // e
    float inclinationDeg;         // i (deg)
    float ascNodeDeg;             // Ω (deg)
    float argPeriDeg;             // ω (deg)
    float periodYears;            // 공전 주기
    float meanAnomalyAtEpochDeg;  // M0 (deg)

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

        // 태양계가 눕지 않도록 XY → XZ
        return glm::vec3(pos.x, 0.0f, pos.y);
    }
};

#endif
