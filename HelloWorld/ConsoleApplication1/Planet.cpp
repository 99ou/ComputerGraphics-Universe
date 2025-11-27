#include "Planet.h"
#include "Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

Planet::Planet(const PlanetParams& p)
    : params(p),
    spinAngleDeg(0.0f),
    maxTrailPoints(4000) // 행성 trail 길이 (원하면 PlanetParams로 빼도 됨)
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

void Planet::recordTrail(const glm::vec3& heliocentricPos)
{
    trail.push_back(heliocentricPos);
    if ((int)trail.size() > maxTrailPoints)
        trail.erase(trail.begin());
}

void Planet::drawTrail(const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& proj) const
{
    if (trail.size() < 2) return;

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("proj", proj);
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setVec3("color", glm::vec3(0.4f, 0.6f, 1.0f)); // 파란 계열 궤도색

    glDisable(GL_DEPTH_TEST);

    std::vector<float> verts;
    verts.reserve(trail.size() * 3);

    for (const auto& p : trail)
    {
        verts.push_back(p.x);
        verts.push_back(p.y);
        verts.push_back(p.z);
    }

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)trail.size());

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glEnable(GL_DEPTH_TEST);
}

void Planet::advanceSpin(float dtSec) const
{
    spinAngleDeg += params.spinDegPerSec * dtSec;
}

glm::mat4 Planet::buildModelMatrix(float scale,
    const glm::vec3& worldPos) const
{
    glm::mat4 model(1.0f);

    model = glm::translate(model, worldPos);

    // 2. [추가] 자전축 기울기 적용 (Z축 기준 회전)
    // 자전(Spin)을 하기 전에 축을 먼저 기울여야, 기울어진 채로 돕니다.
    if (params.axialTiltDeg != 0.0f)
    {
        model = glm::rotate(model, glm::radians(params.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    model = glm::rotate(model,
        glm::radians(spinAngleDeg),
        glm::vec3(0, 1, 0));

    model = glm::scale(model,
        glm::vec3(params.radiusRender * scale));

    return model;
}
