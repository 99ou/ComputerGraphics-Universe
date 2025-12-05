#include "Planet.h"
#include "Shader.h"
#include "Texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <GL/glew.h>
#include <limits>

Planet::Planet(const PlanetParams& p)
	: params(p),         // 행성 파라미터 복사
	spinAngleDeg(0.0f),  // 자전 각도 초기화
    generatedOrbit(false) 
{
    if (params.ring.enabled)
    {
        unsigned int texID = 0;

        if (!params.ring.texturePath.empty())
        {
            texID = loadTexture(params.ring.texturePath);
            params.ring.textureID = texID;   // 나중에 필요하면 참조
        }
        else
        {
            texID = params.ring.textureID;   // fallback (이미 ID가 세팅된 경우)
        }

		// 고리 객체 생성 및 초기화
        ring = new PlanetRing();
		// 외부에서 텍스처 ID를 전달받아 초기화
        ring->init(texID,
            params.ring.innerRadius,
            params.ring.outerRadius,
            params.ring.alpha);
    }
}

void Planet::addSatellite(const Satellite& s)
{
    sats.push_back(s);
}

// 행성의 태양 주위 위치 계산
glm::vec3 Planet::positionAroundSun(float tYears) const
{
	return params.orbit.positionAtTime(tYears); // 궤도 요소로부터 위치 계산
}

// 궤도 경로를 그리는 헬퍼 함수
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

// 주어진 위치에 가장 가까운 궤도 점의 인덱스 찾기
static int findClosestPointIndex(const std::vector<glm::vec3>& path,
    const glm::vec3& pos)
{
    int idx = 0;
    float minDist = std::numeric_limits<float>::max();

    for (int i = 0; i < (int)path.size(); i++)
    {
		// 거리 계산
        float d = glm::distance(path[i], pos);
		// 최소 거리 갱신
        if (d < minDist)
        {
            minDist = d;
            idx = i;
        }
    }
	return idx; // 가장 가까운 점의 인덱스 반환
}

void Planet::drawTrail(const Shader& shader,
	const glm::mat4& view, // 뷰 매트릭스
	const glm::mat4& proj, // 투영 매트릭스
	float tYears) const    // 경과 시간 (년 단위)
{
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("proj", proj);

    glm::mat4 model(1.0f);
	shader.setMat4("model", model); // 단위 행렬

	// 궤도 경로 생성
    if (!generatedOrbit)
    {
		// 기본 궤도 경로 (XY 평면)
        std::vector<glm::vec3> basePath = buildOrbitPath(params.orbit);

		// XY 평면에서 XZ 평면으로 변환 매트릭스
        glm::mat4 orbitToXZ =
            glm::rotate(glm::mat4(1.0f),
                -glm::half_pi<float>(),
                glm::vec3(1, 0, 0));

        orbitPath.clear();
        orbitPath.reserve(basePath.size());

		// 변환 적용
        for (auto& p : basePath)
        {
			glm::vec4 v = orbitToXZ * glm::vec4(p, 1.0f); // XZ 평면으로 변환
			orbitPath.push_back(glm::vec3(v));            // vec3로 변환하여 저장
        }

        generatedOrbit = true;
    }

	// 현재 위치에 가장 가까운 궤도 점 찾기
    glm::mat4 orbitToXZ =
        glm::rotate(glm::mat4(1.0f),
            -glm::half_pi<float>(),
            glm::vec3(1, 0, 0));

	// 현재 행성 위치 계산 (XZ 평면)
    glm::vec3 currPos =
        glm::vec3(orbitToXZ * glm::vec4(positionAroundSun(tYears), 1.0f));

	// 가장 가까운 궤도 점 인덱스 찾기
    int idx = findClosestPointIndex(orbitPath, currPos);

    glDisable(GL_DEPTH_TEST);

    shader.setVec3("color", glm::vec3(1, 1, 1));
    drawLineStrip(orbitPath);

	if (idx > 1) // 유효한 인덱스인 경우
    {
        std::vector<glm::vec3> part;
        part.reserve(idx);

        for (int i = 0; i < idx; i++)
            part.push_back(orbitPath[i]);

        part.push_back(currPos);

        shader.setVec3("color", glm::vec3(0, 1, 0));
        drawLineStrip(part);
    }

    glEnable(GL_DEPTH_TEST);
}

// 자전 업데이트
void Planet::advanceSpin(float dtSec) const
{
    spinAngleDeg += params.spinDegPerSec * dtSec;
}

// 행성 모델 매트릭스 생성
glm::mat4 Planet::buildModelMatrix(float scale, const glm::vec3& worldPos) const
{
    glm::mat4 m(1.0f);
	m = glm::translate(m, worldPos); // 위치 변환

    float precessionSpeed = glm::radians(360.0f / 25772.0f);  // 라디안/년

	extern float gTimeYears; // 전역 시뮬레이션 시간 (년 단위)
	float precessionAngle = precessionSpeed * gTimeYears; // 세차 각도 계산

	// 세차, 자전축 기울기, 자전 변환 적용
    m = glm::rotate(m, precessionAngle, glm::vec3(0, 1, 0));
    m = glm::rotate(m,
        glm::radians(params.axialTiltDeg),
        glm::vec3(0, 0, 1));
    m = glm::rotate(m,
        glm::radians(spinAngleDeg),
        glm::vec3(0, 1, 0));

	// 스케일 변환 적용
    m = glm::scale(m, glm::vec3(params.radiusRender * scale));

    return m;
}
