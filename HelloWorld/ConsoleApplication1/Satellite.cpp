#include "Satellite.h"
#include "Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <GL/glew.h>
#include <limits>

Satellite::Satellite(const SatelliteParams& p)
    : params(p),
    spinAngleDeg(0.0f),
    generatedOrbit(false)
{
}

glm::vec3 Satellite::positionRelativeToPlanet(float tYears) const
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

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
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

void Satellite::drawTrail(const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::mat4& planetModel,
    float tYears) const
{
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("proj", proj);

    glm::vec3 parentPos = glm::vec3(planetModel[3]);

    glm::mat4 model(1.0f);
    model = glm::translate(model, parentPos);
    shader.setMat4("model", model);

    // 1) orbitPath 캐싱
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

    // 현재 위성의 실제 위치(모행성 기준)
    glm::mat4 orbitToXZ =
        glm::rotate(glm::mat4(1.0f),
            -glm::half_pi<float>(),
            glm::vec3(1, 0, 0));

    glm::vec3 relPos =
        glm::vec3(orbitToXZ * glm::vec4(positionRelativeToPlanet(tYears), 1.0f));

    int idx = findClosestPointIndex(orbitPath, relPos);

    glDisable(GL_DEPTH_TEST);

    // 전체 궤도 = 흰색
    shader.setVec3("color", glm::vec3(1, 1, 1));
    drawLineStrip(orbitPath);

    // gap 없애는 초록색
    if (idx > 1)
    {
        std::vector<glm::vec3> part;
        part.reserve(idx);

        for (int i = 0; i < idx; i++)
            part.push_back(orbitPath[i]);

        part.push_back(relPos);   // ★ 현재 위성 위치로 연결(틈 제거 핵심)

        shader.setVec3("color", glm::vec3(0, 1, 0));
        drawLineStrip(part);
    }

    glEnable(GL_DEPTH_TEST);
}

void Satellite::drawAtWorld(const Shader& shader,
    float dtSec,
    float scale,
    const glm::vec3& worldPos,
    unsigned int sphereVAO,
    unsigned int indexCount) const
{
    // const method 내부에서 회전 업데이트
    const_cast<Satellite*>(this)->spinAngleDeg +=
        params.spinDegPerSec * dtSec;

    shader.use();

    glm::mat4 m(1.0f);
    m = glm::translate(m, worldPos);
    m = glm::rotate(m, glm::radians(spinAngleDeg),
        glm::vec3(0, 1, 0));
    m = glm::scale(m, glm::vec3(params.radiusRender * scale));

    shader.setMat4("model", m);
    shader.setVec3("objectColor", params.color);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, indexCount,
        GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
