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

// 위성의 행성 주위 위치 계산
glm::vec3 Satellite::positionRelativeToPlanet(float tYears) const
{
	// 궤도 요소로부터 위치 계산
    return params.orbit.positionAtTime(tYears);
}

// 위성의 궤도 진행도 계산 (0.0 ~ 1.0)
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

// 주어진 위치에 가장 가까운 궤도 점의 인덱스 찾기
static int findClosestPointIndex(const std::vector<glm::vec3>& path,
    const glm::vec3& pos)
{
	int idx = 0; // 가장 가까운 점의 인덱스
	float minDist = std::numeric_limits<float>::max(); // 매우 큰 값으로 초기화

    for (int i = 0; i < (int)path.size(); i++)
    {
		float d = glm::distance(path[i], pos); // 거리 계산
		if (d < minDist) // 최소 거리 갱신
        {
            minDist = d;
            idx = i;
        }
    }
	return idx; // 가장 가까운 점의 인덱스 반환
}

// 위성의 궤도 궤적 그리기
void Satellite::drawTrail(const Shader& shader,
	const glm::mat4& view, // 뷰 매트릭스
	const glm::mat4& proj,  // 투영 매트릭스
	const glm::mat4& planetModel, // 행성 모델 매트릭스
    float tYears) const 
{
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("proj", proj);

	// 행성 위치 추출
    glm::vec3 parentPos = glm::vec3(planetModel[3]);

	// 행성 위치로 이동
    glm::mat4 model(1.0f);
    model = glm::translate(model, parentPos);
    shader.setMat4("model", model);

    // 1) orbitPath 캐싱
    if (!generatedOrbit)
    {
		// 궤도 경로 생성
        std::vector<glm::vec3> basePath = buildOrbitPath(params.orbit);

		// XY 평면 → XZ 평면 변환 행렬
        glm::mat4 orbitToXZ =
            glm::rotate(glm::mat4(1.0f),
                -glm::half_pi<float>(),
                glm::vec3(1, 0, 0));

		// 2) 변환 적용
        orbitPath.clear();
        orbitPath.reserve(basePath.size());

        for (auto& p : basePath)
        {
			glm::vec4 v = orbitToXZ * glm::vec4(p, 1.0f); // XZ 평면으로 변환
			orbitPath.push_back(glm::vec3(v)); // vec3로 변환하여 저장
        }

		generatedOrbit = true; // 궤도 경로 생성 완료 플래그 설정
    }

    // 현재 위성 위치
    glm::mat4 orbitToXZ =
        glm::rotate(glm::mat4(1.0f),
            -glm::half_pi<float>(),
            glm::vec3(1, 0, 0));

	// 현재 위성의 행성 상대 위치 계산
    glm::vec3 relPos =
        glm::vec3(orbitToXZ * glm::vec4(positionRelativeToPlanet(tYears), 1.0f));

	// 가장 가까운 궤도 점 인덱스 찾기
    int idx = findClosestPointIndex(orbitPath, relPos);

    glDisable(GL_DEPTH_TEST);

    // 전체 궤도 = 흰색
    shader.setVec3("color", glm::vec3(1, 1, 1));
	drawLineStrip(orbitPath); // 전체 궤도 그리기

    // gap 제거 초록색 trail
    if (idx > 1)
    {
        std::vector<glm::vec3> part;
        part.reserve(idx);

        for (int i = 0; i < idx; i++)
            part.push_back(orbitPath[i]);

        part.push_back(relPos);   // ★ gap 완전 제거 핵심

        shader.setVec3("color", glm::vec3(0, 1, 0));
        drawLineStrip(part);
    }

    glEnable(GL_DEPTH_TEST);
}

// 위성을 월드 위치에 그리기
void Satellite::drawAtWorld(const Shader& shader,
	float dtSec,   // 시뮬레이션 시간 델타 (초 단위, 자전용)
	float scale,   // 세계 스케일링 팩터
	const glm::vec3& worldPos, // 이미 스케일링 및 행성 위치가 적용된 월드 위치
	unsigned int sphereVAO, // 구 메쉬 VAO
    unsigned int indexCount) const 
{
    // 자전 업데이트
    const_cast<Satellite*>(this)->spinAngleDeg +=
        params.spinDegPerSec * dtSec;

    shader.use();

    glm::mat4 m(1.0f);

    // 1) 월드 위치 이동
    m = glm::translate(m, worldPos);

    // 2) 자전축 기울기 적용 (행성과 동일) 보통 Z축 기준으로 기울인다 (지구/위성 모두 동일)
    m = glm::rotate(m,
        glm::radians(params.axialTiltDeg),
        glm::vec3(0, 0, 1));

    // 3) 자전 적용
    m = glm::rotate(m,
        glm::radians(spinAngleDeg),
        glm::vec3(0, 1, 0));

    // 4) 스케일 적용
    m = glm::scale(m,
        glm::vec3(params.radiusRender * scale));

    shader.setMat4("model", m);
    shader.setVec3("objectColor", params.color);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, indexCount,
        GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
