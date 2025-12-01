#include "Planet.h"
#include "Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <GL/glew.h>
#include <limits>

Planet::Planet(const PlanetParams& p)
    : params(p),
    spinAngleDeg(0.0f),
    generatedOrbit(false)
{
}

void Planet::addSatellite(const Satellite& s)
{
    sats.push_back(s);
}

glm::vec3 Planet::positionAroundSun(float tYears) const
{
    return params.orbit.positionAtTime(tYears);
}

static void drawLineStrip(const std::vector<glm::vec3>& pts)
{
    if (pts.size() < 2) return;

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        pts.size() * sizeof(glm::vec3),
        pts.data(),
        GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)pts.size());

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

static int findClosestPointIndex(const std::vector<glm::vec3>& path,
    const glm::vec3& pos)
{
    int idx = 0;
    float minDist = std::numeric_limits<float>::max();

    for (int i = 0; i < (int)path.size(); i++)
    {
        float d = glm::distance(path[i], pos);
        if (d < minDist)
        {
            minDist = d;
            idx = i;
        }
    }
    return idx;
}

void Planet::drawTrail(const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& proj,
    float tYears) const
{
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("proj", proj);

    glm::mat4 model(1.0f);
    shader.setMat4("model", model);

    // ------------------------------
    // 1) orbitPath 캐싱
    // ------------------------------
    if (!generatedOrbit)
    {
        std::vector<glm::vec3> basePath = buildOrbitPath(params.orbit);

        glm::mat4 orbitToXZ =
            glm::rotate(glm::mat4(1.0f),
                -glm::half_pi<float>(),
                glm::vec3(1, 0, 0));

        orbitPath.clear();
        orbitPath.reserve(basePath.size());

        for (auto& p : basePath)
        {
            glm::vec4 v = orbitToXZ * glm::vec4(p, 1.0f);
            orbitPath.push_back(glm::vec3(v));
        }

        generatedOrbit = true;
    }

    // ------------------------------
    // 2) 현재 행성의 실제 위치
    // ------------------------------
    glm::mat4 orbitToXZ =
        glm::rotate(glm::mat4(1.0f),
            -glm::half_pi<float>(),
            glm::vec3(1, 0, 0));

    glm::vec3 currPos =
        glm::vec3(orbitToXZ * glm::vec4(positionAroundSun(tYears), 1.0f));

    // ------------------------------
    // 3) 현재 위치와 가장 가까운 orbitPath index
    // ------------------------------
    int idx = findClosestPointIndex(orbitPath, currPos);

    glDisable(GL_DEPTH_TEST);

    // 전체 궤도 = 흰색
    shader.setVec3("color", glm::vec3(1, 1, 1));
    drawLineStrip(orbitPath);

    // ------------------------------
    // 4) 초록색 (gap 완전 제거 버전)
    // ------------------------------
    if (idx > 1)
    {
        std::vector<glm::vec3> part;
        part.reserve(idx);

        // 0 ~ idx-1
        for (int i = 0; i < idx; i++)
            part.push_back(orbitPath[i]);

        // 마지막 = currPos (★ gap 완전 제거 핵심)
        part.push_back(currPos);

        shader.setVec3("color", glm::vec3(0, 1, 0));
        drawLineStrip(part);
    }

    glEnable(GL_DEPTH_TEST);
}

void Planet::advanceSpin(float dtSec) const
{
    spinAngleDeg += params.spinDegPerSec * dtSec;
}

glm::mat4 Planet::buildModelMatrix(float scale,
    const glm::vec3& worldPos) const
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, worldPos);
    m = glm::rotate(m, glm::radians(spinAngleDeg), glm::vec3(0, 1, 0));
    m = glm::scale(m, glm::vec3(params.radiusRender * scale));
    return m;
}
