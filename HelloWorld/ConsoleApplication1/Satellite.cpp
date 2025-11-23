#include "Satellite.h"
#include "Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

Satellite::Satellite(const SatelliteParams& p)
    : params(p),
    spinAngleDeg(0.0f)
{
    // 위성별 trail 최대 길이 설정
    if (params.trailMaxPoints > 0)
        maxTrailPoints = params.trailMaxPoints;
    else
        maxTrailPoints = 2000;
}

glm::vec3 Satellite::positionRelativeToPlanet(float tYears) const
{
    // 케플러 궤도 계산
    return params.orbit.positionAtTime(tYears);
}

void Satellite::recordTrail(const glm::vec3& relPos)
{
    trail.push_back(relPos);
    if ((int)trail.size() > maxTrailPoints)
        trail.erase(trail.begin());
}

void Satellite::drawTrail(const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::mat4& model) const
{
    if (trail.size() < 2) return;

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("proj", proj);
    shader.setMat4("model", model);
    shader.setVec3("color", glm::vec3(0.8f, 0.8f, 0.8f));

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

void Satellite::drawAtWorld(const Shader& shader,
    float dtSec,
    float scale,
    const glm::vec3& worldPos) const
{
    spinAngleDeg += params.spinDegPerSec * dtSec;

    shader.use();

    glm::mat4 model(1.0f);
    model = glm::translate(model, worldPos);

    model = glm::rotate(model,
        glm::radians(spinAngleDeg),
        glm::vec3(0, 1, 0));

    model = glm::scale(model,
        glm::vec3(params.radiusRender * scale));

    shader.setMat4("model", model);

    // 텍스처가 planetShader에서 glBindTexture(GL_TEXTURE_2D, textureID)로 먼저 바인드됨
    shader.setVec3("objectColor", params.color);
}
