#ifndef PHYSICS_H
#define PHYSICS_H

#include <glm/glm.hpp>

namespace Phys
{
    // 단위: "AU 비슷한 것" / "년 비슷한 것"… 실제 값보단 안정성 위주
    const float G = 1.0f; // 게임용 중력 상수 (너무 크지 않게 설정)

    inline glm::vec3 gravAccel(const glm::vec3& pos_i,
        const glm::vec3& pos_j,
        float mass_j)
    {
        glm::vec3 r = pos_j - pos_i;
        float dist2 = glm::dot(r, r) + 1e-4f;
        float invDist = 1.0f / sqrtf(dist2);
        float invDist3 = invDist * invDist * invDist;
        return (G * mass_j) * r * invDist3;
    }
}

#endif
