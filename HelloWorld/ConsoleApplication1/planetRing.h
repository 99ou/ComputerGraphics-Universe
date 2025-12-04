#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader;

// ====================================================
// PlanetRing
//  - 고리 메쉬와 렌더링 담당
//  - 텍스처 로드는 외부(main.cpp/Planet.cpp)에서 수행
// ====================================================
class PlanetRing
{
public:
    PlanetRing();
    ~PlanetRing();

    // 외부에서 textureID를 전달받는 초기화
    void init(unsigned int texID,
        float innerR,
        float outerR,
        float alpha = 1.0f);

    // ringShader 지정
    void setShader(Shader* shaderPtr);

    // 렌더링
    void render(const glm::mat4& planetModel,
        const glm::mat4& view,
        const glm::mat4& proj);

private:
    unsigned int vao, vbo;       // 고리 메쉬
    unsigned int textureID;      // 메인에서 전달된 텍스처 ID

    float innerRadius;
    float outerRadius;
    float alpha;

    Shader* shader;

    void createMesh(int segments);
};
