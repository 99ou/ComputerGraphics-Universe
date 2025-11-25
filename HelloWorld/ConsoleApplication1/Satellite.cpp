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

// 헤더(.h) 파일 선언도 맞춰서 수정해야 합니다: 
// void drawAtWorld(..., unsigned int sphereVAO, unsigned int indexCount) const;

void Satellite::drawAtWorld(const Shader& shader,
    float dtSec,
    float scale,
    const glm::vec3& worldPos,
    unsigned int sphereVAO,      // [추가] 어떤 모양을 그릴지 알아야 함
    unsigned int indexCount)     // [추가] 점이 몇 개인지 알아야 함
    const // 주의: 멤버변수 spinAngleDeg를 수정하려면 mutable 키워드가 필요하거나 const를 빼야 함
{
    // const 함수에서 멤버 변수 수정 불가하므로, 
    // 헤더에서 float spinAngleDeg; 를 mutable float spinAngleDeg; 로 바꾸거나
    // 이 함수의 const를 제거하세요.
    const_cast<Satellite*>(this)->spinAngleDeg += params.spinDegPerSec * dtSec;

    shader.use();

    glm::mat4 model(1.0f);
    model = glm::translate(model, worldPos);
    model = glm::rotate(model, glm::radians(spinAngleDeg), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(params.radiusRender * scale));

    shader.setMat4("model", model);
    shader.setVec3("objectColor", params.color); // 쉐이더 변수명 확인 필요 (diffuseMap 쓰면 이건 무시될 수도)

    // [핵심 추가] 실제 그리기 명령
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
