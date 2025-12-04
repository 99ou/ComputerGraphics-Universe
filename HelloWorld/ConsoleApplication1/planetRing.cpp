#include "planetRing.h"
#include "Shader.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <cmath>

PlanetRing::PlanetRing()
    : vao(0), vbo(0), textureID(0),
    innerRadius(1.0f), outerRadius(1.5f),
    alpha(1.0f), shader(nullptr)
{
}

PlanetRing::~PlanetRing()
{
}

void PlanetRing::setShader(Shader* shaderPtr)
{
    shader = shaderPtr;
}

void PlanetRing::init(unsigned int texID,
    float innerR,
    float outerR,
    float alphaVal)
{
    textureID = texID;       // main.cpp에서 전달됨
    innerRadius = innerR;
    outerRadius = outerR;
    alpha = alphaVal;

    createMesh(512);
}

void PlanetRing::createMesh(int segments)
{
    std::vector<float> verts;
    verts.reserve((segments + 1) * 2 * 5);

    for (int i = 0; i <= segments; i++)
    {
        float t = float(i) / segments;              // 0~1, 각도 비율
        float ang = t * glm::two_pi<float>();

        float c = cos(ang);
        float s = sin(ang);

        // inner (반지름 0쪽)
        verts.push_back(c * innerRadius);
        verts.push_back(0.0f);
        verts.push_back(s * innerRadius);
        verts.push_back(0.0f);   // u = 반지름 방향 (안쪽)
        verts.push_back(t);      // v = 각도 방향 (둘레)

        // outer (반지름 1쪽)
        verts.push_back(c * outerRadius);
        verts.push_back(0.0f);
        verts.push_back(s * outerRadius);
        verts.push_back(1.0f);   // u = 반지름 방향 (바깥쪽)
        verts.push_back(t);      // v = 각도 방향 (둘레)
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void PlanetRing::render(const glm::mat4& planetModel,
    const glm::mat4& view,
    const glm::mat4& proj)
{
    if (!shader) return;

    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("proj", proj);

    // 행성의 modelMatrix를 그대로 사용 (추가 회전 X)
    glm::mat4 model = planetModel;

    shader->setMat4("model", model);
    shader->setFloat("alpha", alpha);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    shader->setInt("ringTex", 0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (512 + 1) * 2);
}