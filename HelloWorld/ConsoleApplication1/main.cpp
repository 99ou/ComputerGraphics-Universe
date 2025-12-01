#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <vector>

#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Sun.h"
#include "Planet.h"
#include "Satellite.h"
#include "Orbit.h"

unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;

Camera* gCamera = nullptr;
const float SCALE_UNITS = 1.0f;


// ========================
// XZ 평면 정렬 행렬 정의
// ========================
static glm::mat4 orbitToXZ =
glm::rotate(glm::mat4(1.0f),
	glm::radians(-90.0f),
	glm::vec3(1.0f, 0.0f, 0.0f));

// 콜백 -------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	SCR_WIDTH = w;
	SCR_HEIGHT = h;
	glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (gCamera)
		gCamera->processMouseMovement((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (gCamera)
		gCamera->processMouseScroll((float)yoffset);
}

// 구 + UV ----------------------------------------------------------
void createSphere(unsigned int& vao, unsigned int& indexCount,
	int stacks = 40, int slices = 40)
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

	for (int i = 0; i <= stacks; ++i)
	{
		float v = (float)i / stacks;
		float phi = v * glm::pi<float>();

		for (int j = 0; j <= slices; ++j)
		{
			float u = (float)j / slices;
			float theta = u * glm::two_pi<float>();

			float x = sin(phi) * cos(theta);
			float y = cos(phi);
			float z = sin(phi) * sin(theta);

			// pos
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);
			// normal
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);
			// uv
			vertices.push_back(u);
			vertices.push_back(v);
		}
	}

	for (int i = 0; i < stacks; ++i)
	{
		for (int j = 0; j < slices; ++j)
		{
			int row1 = i * (slices + 1);
			int row2 = (i + 1) * (slices + 1);

			indices.push_back(row1 + j);
			indices.push_back(row2 + j);
			indices.push_back(row1 + j + 1);

			indices.push_back(row1 + j + 1);
			indices.push_back(row2 + j);
			indices.push_back(row2 + j + 1);
		}
	}

	indexCount = (unsigned int)indices.size();

	unsigned int vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
		vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
		indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

// 텍스처 로드 및 에러 체크 헬퍼 함수
unsigned int loadTextureWithCheck(const char* path)
{
	unsigned int textureID = loadTexture(path);
	if (textureID == 0) // loadTexture가 실패하면 보통 0을 반환함
	{
		std::cerr << "[ERROR] Texture load failed: " << path << std::endl;
		// 필요하다면 여기서 프로그램을 강제 종료하거나, 
		// 핑크색 '에러 텍스처'를 대신 반환하는 로직을 넣을 수도 있음
	}
	return textureID;
}

// 화면 Quad --------------------------------------------------------
void createQuad(unsigned int& vao, unsigned int& vbo)
{
	float quadVertices[] = {
		// pos   // uv
		-1.f,  1.f, 0.f, 1.f,
		-1.f, -1.f, 0.f, 0.f,
		 1.f, -1.f, 1.f, 0.f,

		-1.f,  1.f, 0.f, 1.f,
		 1.f, -1.f, 1.f, 0.f,
		 1.f,  1.f, 1.f, 1.f
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}
// [추가] 토성 고리용 Mesh (3D Scene용) - vec3 pos, vec3 normal, vec2 uv
void createRingMesh(unsigned int& vao, unsigned int& vbo)
{
	float vertices[] = {
		// Pos (3)             // Normal (3)       // UV (2)
		-1.0f,  1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,

		-1.0f,  1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
		 1.0f, -1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// layout 0: vec3 (Pos)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// layout 1: vec3 (Normal)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// layout 2: vec2 (UV)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

// -------------------------------------------------------------
// [Helper] 행성 파라미터 생성기
// 필수적인 값만 인자로 받고, 나머지는 기본값(0 등)으로 채웁니다.
// -------------------------------------------------------------
PlanetParams createPlanetParams(std::string name,
	float radius,
	float distance,   // SemiMajorAxis
	float eccentricity,
	float period,
	float spinSpeed,
	std::string texPath)
{
	PlanetParams p;
	p.name = name;
	p.mass = 1.0f; // 필요하면 인자로 추가
	p.radiusRender = radius;
	p.color = glm::vec3(1.0f);

	// 궤도 기본 설정
	p.orbit.semiMajorAxis = distance;
	p.orbit.eccentricity = eccentricity;
	p.orbit.periodYears = period;

	// 나머지 궤도 각도들은 0.0f로 초기화 (필요시 반환 후 수정)
	p.orbit.inclinationDeg = 0.0f;
	p.orbit.ascNodeDeg = 0.0f;
	p.orbit.argPeriDeg = 0.0f;
	p.orbit.meanAnomalyAtEpochDeg = 0.0f;

	p.spinDegPerSec = spinSpeed;
	p.texturePath = texPath;

	return p;
}

// -------------------------------------------------------------
// [Helper] 위성 파라미터 생성기
// -------------------------------------------------------------
SatelliteParams createSatelliteParams(std::string name,
	float radius,
	float distance,
	float eccentricity,
	float period,
	float spinSpeed,
	std::string texPath)
{
	SatelliteParams p;
	p.name = name;
	p.mass = 0.05f; // 기본값
	p.radiusRender = radius;
	p.color = glm::vec3(1.0f);

	p.orbit.semiMajorAxis = distance;
	p.orbit.eccentricity = eccentricity;
	p.orbit.periodYears = period;

	// 위성은 궤도 경사각이 있는 경우가 많으므로 일단 0으로 두고 밖에서 수정 권장
	p.orbit.inclinationDeg = 0.0f;
	p.orbit.ascNodeDeg = 0.0f;
	p.orbit.argPeriDeg = 0.0f;
	p.orbit.meanAnomalyAtEpochDeg = 0.0f;

	p.spinDegPerSec = spinSpeed;
	p.trailMaxPoints = 200; // 궤적 길이 기본값
	p.texturePath = texPath;

	return p;
}
// 태양계 Setup -------------------------------------------------------
void setupSolarSystem(Sun& sun)
{
	// [ 행성 ] ----------------------------------------------------------
	// 1) 수성 (Mercury)
	PlanetParams mercuryP = createPlanetParams(
		"Mercury",
		0.25f,			  // 렌더링 반지름
		16.0f,            // 태양 거리(시뮬레이션 스케일)
		0.2056f,          // 이심률 (실제 값)
		0.0f,             // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		0.1f,             // 자전 속도
		"textures/2k_mercury.jpg"
	);

	// 수성 실제 궤도 요소 추가 (NASA JPL Elements)
	mercuryP.orbit.inclinationDeg = 7.005f;     // 궤도 경사각 i
	mercuryP.orbit.ascNodeDeg = 48.33167f;		// 승교점 경도 Ω
	mercuryP.orbit.argPeriDeg = 29.124f;		// 근일점 인수 ω
	mercuryP.orbit.meanAnomalyAtEpochDeg = 174.796f;	// 평균근점이각 M₀
	mercuryP.orbit.periodYears = 0.240846f;		// 실제 공전주기

	// 객체 생성 + 태양에 추가
	Planet mercury(mercuryP);
	sun.addPlanet(mercury); // [0] 등록

	// --------------------------------------------------------------------
	// 2) 금성 (Venus)
	PlanetParams venusP = createPlanetParams(
		"Venus",
		0.60f,              // 렌더링 반지름
		25.0f,              // 태양 거리 (시뮬레이션 스케일)
		0.006773f,          // 이심률 (실제 값)
		0.0f,               // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		-0.05f,             // 역자전 (금성 자전은 시계 방향)
		"textures/2k_venus_atmosphere.jpg"
	);

	// 금성 실제 궤도 요소 추가 (NASA JPL Elements)
	venusP.orbit.inclinationDeg = 3.39458f;    // 궤도 경사각 i
	venusP.orbit.ascNodeDeg = 76.67984f;	   // 승교점 경도 Ω
	venusP.orbit.argPeriDeg = 54.884f;		   // 근일점 인수 ω
	venusP.orbit.meanAnomalyAtEpochDeg = 50.416f;	// 평균근점이각 M₀
	venusP.orbit.periodYears = 0.615197f;	   // 실제 공전주기

	// 객체 생성 + 태양에 추가
	Planet venus(venusP);
	sun.addPlanet(venus);   // [1] 등록

	// --------------------------------------------------------------------
	// 3) 지구 (Earth)
	PlanetParams earthP = createPlanetParams(
		"Earth",
		0.65f,				// 렌더링 반지름
		34.0f,				// 태양 거리 (시뮬레이션 스케일)
		0.0167086f,         // 이심률 (실제 값)
		0.0f,               // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		1.0f,               // 자전 속도
		"textures/2k_earth_daymap.jpg"
	);

	// 지구 실제 궤도 요소 추가 (NASA JPL Elements)
	earthP.orbit.inclinationDeg = 0.00005f;    // 궤도 경사각 i
	earthP.orbit.ascNodeDeg = -11.26064f;	   // 승교점 경도 Ω
	earthP.orbit.argPeriDeg = 102.94719f;	   // 근일점 인수 ω
	earthP.orbit.meanAnomalyAtEpochDeg = 357.51716f;	// 평균근점이각 M₀
	earthP.orbit.periodYears = 1.000000f;      // 실제 공전주기

	Planet earth(earthP);

	// 위성 등록은 해당 Sun에 추가하기 전에 해당 행성에 추가하기 
	// 달 (Moon)
	SatelliteParams moonP = createSatelliteParams(
		"Moon",
		0.18f,                  // 렌더링 반지름
		3.0f,                   // 지구 기준 거리 (시각적 스케일)
		0.0549f,                // 이심률 (실제 값)
		0.0f,					// 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		0.0f,                   // 자전 속도 (시각적 속도, 실제 자전주기를 적용하니 0.0f로 한다.)
		"textures/2k_moon.jpg"
	);

	// 달 실제 궤도 요소 (지구 기준)
	moonP.orbit.inclinationDeg = 5.145f;			// 궤도 경사각 i
	moonP.orbit.ascNodeDeg = 125.08f;				// 승교점 경도 Ω
	moonP.orbit.argPeriDeg = 318.15f;			    // 근일점 인수 ω
	moonP.orbit.meanAnomalyAtEpochDeg = 135.27f;    // 평균근점이각 M₀
	moonP.orbit.periodYears = 27.321661f / 365.25f; // 실제 공전주기
	moonP.spinDegPerSec = 360.0f / (moonP.orbit.periodYears * 365.25f * 24.0f * 3600.0f); // 조석 고정

	// 위성 객체 생성 후 지구에 추가
	Satellite moon(moonP);
	earth.addSatellite(moon);
	sun.addPlanet(earth); // [2] 등록

	// --------------------------------------------------------------------
	// 4) 화성 (Mars)
	PlanetParams marsP = createPlanetParams(
		"Mars",
		0.34f,               // 렌더링 반지름
		49.0f,               // 태양 거리 (시뮬레이션 스케일)
		0.0934f,             // 이심률 (실제 값)
		0.0f,                // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		0.97f,               // 자전 속도
		"textures/2k_mars.jpg"
	);

	// 화성 실제 궤도 요소 (NASA JPL Elements)
	marsP.orbit.inclinationDeg = 1.850f;     // 궤도 경사각 i
	marsP.orbit.ascNodeDeg = 49.558f;		 // 승교점 경도 Ω
	marsP.orbit.argPeriDeg = 286.502f;       // 근일점 인수 ω
	marsP.orbit.meanAnomalyAtEpochDeg = 19.412f;	// 평균근점이각 M₀
	marsP.orbit.periodYears = 1.8808476f;	 // 실제 공전주기

	// 객체 생성 + 태양에 등록
	Planet mars(marsP);
	sun.addPlanet(mars);   // [3] 등록

	// --------------------------------------------------------------------
	// 5) 목성 (Jupiter)
	PlanetParams jupiterP = createPlanetParams(
		"Jupiter",
		2.8f,                // 렌더링 반지름
		94.0f,               // 태양 거리 (시뮬레이션 스케일)
		0.0489f,             // 이심률 (실제 값)
		0.0f,                // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		2.4f,                // 자전 속도
		"textures/2k_jupiter.jpg"
	);

	// 목성 실제 궤도 요소 (NASA JPL Elements)
	jupiterP.orbit.inclinationDeg = 1.303f;       // 궤도 경사각 i
	jupiterP.orbit.ascNodeDeg = 100.556f;		  // 승교점 경도 Ω
	jupiterP.orbit.argPeriDeg = 14.75385f;		  // 근일점 인수 ω
	jupiterP.orbit.meanAnomalyAtEpochDeg = 20.0202f;	// 평균근점이각 M₀
	jupiterP.orbit.periodYears = 11.862615f;	  // 실제 공전주기

	// 객체 생성 + 태양에 등록
	Planet jupiter(jupiterP);
	sun.addPlanet(jupiter);   // [4] 등록

	// --------------------------------------------------------------------
	// 6) 토성 (Saturn)
	PlanetParams saturnP = createPlanetParams(
		"Saturn",
		2.5f,               // 렌더링 반지름
		140.0f,             // 태양과 거리 (시뮬레이션 스케일)
		0.0565f,            // 이심률 (실제 값)
		0.25f,              // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		2.0f,               // 자전 속도
		"textures/2k_saturn.jpg"
	);

	// 토성의 실제 궤도 요소 (NASA JPL Elements)
	saturnP.orbit.inclinationDeg = 2.485f;      // 궤도 경사각 i
	saturnP.orbit.ascNodeDeg = 113.715f;		// 승교점 경도 Ω
	saturnP.orbit.argPeriDeg = 92.43194f;		// 근일점 인수 ω
	saturnP.orbit.meanAnomalyAtEpochDeg = 317.0207f;   // 평균근점이각 M₀
	saturnP.orbit.periodYears = 29.447498f;		// 실제 공전주기

	// 객체 생성 + 태양에 추가
	Planet saturn(saturnP);
	sun.addPlanet(saturn);   // [5] 등록

	// --------------------------------------------------------------------
	// 7) 천왕성 (Uranus)
	PlanetParams uranusP = createPlanetParams(
		"Uranus",
		1.8f,                // 렌더링 반지름
		190.0f,              // 태양 거리 (시뮬레이션 스케일)
		0.0472f,             // 이심률 (실제 값)
		0.15f,               // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		-1.4f,               // 자전속도 (천왕성은 '거의 90도 눕는' 극단적인 역자전 → 음수로 표현)
		"textures/2k_uranus.jpg"
	);

	// 천왕성 실제 궤도 요소 (NASA JPL Elements)
	uranusP.orbit.inclinationDeg = 0.773f;      // 궤도 경사각 i
	uranusP.orbit.ascNodeDeg = 74.006f;			// 승교점 경도 Ω
	uranusP.orbit.argPeriDeg = 96.998857f;		// 근일점 인수 ω
	uranusP.orbit.meanAnomalyAtEpochDeg = 142.2386f;   // 평균근점이각 M₀
	uranusP.orbit.periodYears = 84.016846f;		// 실제 공전주기

	// 객체 생성 + 태양 등록
	Planet uranus(uranusP);
	sun.addPlanet(uranus);   // [6] 등록
	
	// --------------------------------------------------------------------
	// 8) 해왕성 (Neptune)
	PlanetParams neptuneP = createPlanetParams(
		"Neptune",
		1.7f,                // 렌더링 반지름
		230.0f,              // 태양 거리 (시뮬레이션 스케일)
		0.008678f,           // 이심률 (실제 값)
		0.0f,                // 공전 속도 스케일 (시각적 속도, 실제 공전주기를 적용하니 0.0f로 한다.)
		1.5f,                // 자전 속도 (Neptune은 빠른 자전)
		"textures/2k_neptune.jpg"
	);

	// 해왕성 실제 궤도 요소 (NASA JPL Elements)
	neptuneP.orbit.inclinationDeg = 1.769f;       // 궤도 경사각 i
	neptuneP.orbit.ascNodeDeg = 131.784f;		  // 승교점 경도 Ω
	neptuneP.orbit.argPeriDeg = 273.187f;		  // 근일점 인수 ω
	neptuneP.orbit.meanAnomalyAtEpochDeg = 259.908f;	// 평균근점이각 M₀
	neptuneP.orbit.periodYears = 164.79132f;      // 실제 공전주기

	// 객체 생성 + 태양 등록
	Planet neptune(neptuneP);
	sun.addPlanet(neptune);   // [7] 등록
}

// render helper: 한 행성(Planet)을 렌더링
void renderPlanet(Planet& planet,
	unsigned int textureID,
	Shader& shader,
	unsigned int sphereIndexCount,
	float dtSeconds,
	float worldScale,
	const glm::vec3& worldPos,
	unsigned int sphereVAO,      // VAO 바인딩
	bool isSun = false,
	float emissionStrength = 1.0f)
{
	// 자전 업데이트
	planet.advanceSpin(dtSeconds);

	// 텍스처 + 쉐이더 유니폼 설정
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	shader.setInt("diffuseMap", 0);
	shader.setInt("isSun", isSun ? 1 : 0);
	shader.setFloat("emissionStrength", emissionStrength);

	// 모델 매트릭스 생성 및 전송
	glm::mat4 model = planet.buildModelMatrix(worldScale, worldPos);
	shader.setMat4("model", model);

	// 실제 그리기
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

// 행성 위치 업데이트 및 렌더링 좌표 계산 함수
// 리턴값: 행성의 최종 렌더링 위치 (World Position)
glm::vec3 updatePlanetPhysics(Planet& planet, float simYears, float scaleUnits)
{
	// ------------------------------------------------------------
	// 1. 행성 위치 (XY 기반)
	// ------------------------------------------------------------
	glm::vec3 physPos = planet.positionAroundSun(simYears);

	// 회전 적용된 행성 trail 기록
	glm::vec3 physPosXZ =
		glm::vec3(orbitToXZ * glm::vec4(physPos, 1.0f));
	// planet.recordTrail(physPosXZ);

	// 행성 worldPos
	glm::vec3 worldPos =
		glm::vec3(orbitToXZ * glm::vec4(physPos * scaleUnits, 1.0f));

	// ------------------------------------------------------------
	// 2. 위성(달) 처리
	// ------------------------------------------------------------
	auto& sats = planet.satellites();
	if (!sats.empty())
	{
		Satellite& moon = sats[0];

		float Mp = planet.getParams().mass;
		float Ms = moon.getParams().mass;

		// (A) 달의 상대 궤도 좌표 (XY 기반)
		glm::vec3 rel = moon.positionRelativeToPlanet(simYears);

		// (B) 달의 상대 궤도를 XZ 평면으로 회전 (⭐ 매우 중요)
		glm::vec3 relXZ =
			glm::vec3(orbitToXZ * glm::vec4(rel, 1.0f));

		// (C) barycenter 보정도 회전 전 값이 아닌 XY 기준 rel로 계산
		glm::vec3 Re = -(Ms / (Mp + Ms)) * rel;

		// (D) 행성 최종 worldPos (회전 포함)
		worldPos =
			glm::vec3(orbitToXZ *
				glm::vec4((physPos + Re) * scaleUnits, 1.0f));

		// (E) moon trail은 main loop에서 relXZ 기반 worldPos로 기록
		//     updatePlanetPhysics에서는 trail 기록하지 않음.
	}

	return worldPos;
}

// main -------------------------------------------------------------
int main()
{
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window =
		glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Full", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "GLEW init error\n";
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Camera cam(glm::vec3(0.0f, 80.0f, 230.0f));
	gCamera = &cam;

	// Skybox shader -------------------------------------------------
	const char* skyVert =
		"#version 330 core\n"
		"layout(location=0) in vec3 aPos;\n"
		"layout(location=1) in vec3 aNormal;\n"
		"layout(location=2) in vec2 aTex;\n"
		"out vec2 TexCoord;\n"
		"uniform mat4 view;\n"
		"uniform mat4 proj;\n"
		"void main(){\n"
		"  mat4 rotView = mat4(mat3(view));\n"
		"  TexCoord = aTex;\n"
		"  gl_Position = proj * rotView * vec4(aPos * 1500.0, 1.0);\n"
		"}\n";

	const char* skyFrag =
		"#version 330 core\n"
		"in vec2 TexCoord;\n"
		"out vec4 FragColor;\n"
		"uniform sampler2D skyTex;\n"
		"void main(){ FragColor = vec4(texture(skyTex, TexCoord).rgb, 1.0); }\n";

	Shader skyShader(skyVert, skyFrag);

	// Scene + bright pass shader -----------------------------------
	const char* sceneVert =
		"#version 330 core\n"
		"layout(location=0) in vec3 aPos;\n"
		"layout(location=1) in vec3 aNormal;\n"
		"layout(location=2) in vec2 aTex;\n"
		"out vec3 FragPos;\n"
		"out vec3 Normal;\n"
		"out vec2 TexCoord;\n"
		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 proj;\n"
		"void main(){\n"
		"  FragPos = vec3(model * vec4(aPos,1.0));\n"
		"  Normal  = mat3(transpose(inverse(model))) * aNormal;\n"
		"  TexCoord= aTex;\n"
		"  gl_Position = proj * view * vec4(FragPos,1.0);\n"
		"}\n";

	const char* sceneFrag =
		"#version 330 core\n"
		"in vec3 FragPos;\n"
		"in vec3 Normal;\n"
		"in vec2 TexCoord;\n"
		"layout(location=0) out vec4 FragColor;\n"
		"layout(location=1) out vec4 BrightColor;\n"
		"uniform sampler2D diffuseMap;\n"
		"uniform vec3 lightPos;\n"
		"uniform vec3 lightColor;\n"
		"uniform vec3 viewPos;\n"
		"uniform int  isSun;\n"
		"uniform float emissionStrength;\n"
		"void main(){\n"
		"  vec3 texColor = texture(diffuseMap, TexCoord).rgb;\n"
		"  vec3 color;\n"
		"  if(isSun == 1){\n"
		"    color = texColor * emissionStrength;\n"
		"  } else {\n"
		"    vec3 norm = normalize(Normal);\n"
		"    vec3 lightDir = normalize(lightPos - FragPos);\n"
		"    float diff = max(dot(norm, lightDir), 0.0);\n"
		"    vec3 diffuse = diff * lightColor * texColor;\n"
		"    vec3 ambient = 0.1 * texColor;\n"
		"    color = ambient + diffuse;\n"
		"  }\n"
		"  FragColor = vec4(color,1.0);\n"
		"  float brightness = dot(color, vec3(0.2126,0.7152,0.0722));\n"
		"  if(brightness > 1.0) BrightColor = vec4(color,1.0);\n"
		"  else BrightColor = vec4(0.0,0.0,0.0,1.0);\n"
		"}\n";

	Shader sceneShader(sceneVert, sceneFrag);

	// Orbit / trail line shader ------------------------------------
	const char* lineVert =
		"#version 330 core\n"
		"layout(location=0) in vec3 aPos;\n"
		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 proj;\n"
		"void main(){ gl_Position = proj * view * model * vec4(aPos,1.0); }\n";

	const char* lineFrag =
		"#version 330 core\n"
		"layout(location=0) out vec4 FragColor;\n"
		"uniform vec3 color;\n"
		"void main(){ FragColor = vec4(color,1.0); }\n";

	Shader lineShader(lineVert, lineFrag);

	// Blur shader ---------------------------------------------------
	const char* quadVert =
		"#version 330 core\n"
		"layout(location=0) in vec2 aPos;\n"
		"layout(location=1) in vec2 aTex;\n"
		"out vec2 TexCoord;\n"
		"void main(){ TexCoord = aTex; gl_Position = vec4(aPos,0.0,1.0); }\n";

	const char* blurFrag =
		"#version 330 core\n"
		"in vec2 TexCoord;\n"
		"out vec4 FragColor;\n"
		"uniform sampler2D image;\n"
		"uniform bool horizontal;\n"
		"const float weight[5] = float[](0.227027,0.1945946,0.1216216,0.054054,0.016216);\n"
		"void main(){\n"
		"  vec2 texel = 1.0 / vec2(textureSize(image,0));\n"
		"  vec3 result = texture(image, TexCoord).rgb * weight[0];\n"
		"  for(int i=1;i<5;i++){\n"
		"    if(horizontal){\n"
		"      result += texture(image, TexCoord + vec2(texel.x*i,0)).rgb * weight[i];\n"
		"      result += texture(image, TexCoord - vec2(texel.x*i,0)).rgb * weight[i];\n"
		"    } else {\n"
		"      result += texture(image, TexCoord + vec2(0,texel.y*i)).rgb * weight[i];\n"
		"      result += texture(image, TexCoord - vec2(0,texel.y*i)).rgb * weight[i];\n"
		"    }\n"
		"  }\n"
		"  FragColor = vec4(result,1.0);\n"
		"}\n";

	Shader blurShader(quadVert, blurFrag);

	// Final composite shader ---------------------------------------
	const char* finalFrag =
		"#version 330 core\n"
		"in vec2 TexCoord;\n"
		"out vec4 FragColor;\n"
		"uniform sampler2D sceneTex;\n"
		"uniform sampler2D bloomTex;\n"
		"uniform float exposure;\n"
		"void main(){\n"
		"  vec3 hdr   = texture(sceneTex, TexCoord).rgb;\n"
		"  vec3 bloom = texture(bloomTex, TexCoord).rgb;\n"
		"  vec3 col   = hdr + bloom;\n"
		"  vec3 mapped = vec3(1.0) - exp(-col * exposure);\n"
		"  mapped = pow(mapped, vec3(1.0/2.2));\n"
		"  FragColor = vec4(mapped,1.0);\n"
		"}\n";

	Shader finalShader(quadVert, finalFrag);

	// 기하 생성 ----------------------------------------------------
	unsigned int sphereVAO, sphereIndexCount;
	createSphere(sphereVAO, sphereIndexCount);

	unsigned int quadVAO, quadVBO;
	createQuad(quadVAO, quadVBO);

	unsigned int ringVAO, ringVBO;
	createRingMesh(ringVAO, ringVBO); // 토성 전용 메시 생성

	// 텍스처 -------------------------------------------------------
	// loadTexture -> loadTextureWithCheck로 변경

	unsigned int texSun = loadTextureWithCheck("textures/2k_sun.jpg");

	// 행성
	unsigned int texMercury = loadTextureWithCheck("textures/2k_mercury.jpg");
	unsigned int texVenus = loadTextureWithCheck("textures/2k_venus_surface.jpg");
	unsigned int texEarth = loadTextureWithCheck("textures/2k_earth_daymap.jpg");
	unsigned int texMars = loadTextureWithCheck("textures/2k_mars.jpg");
	unsigned int texJupiter = loadTextureWithCheck("textures/2k_jupiter.jpg");
	unsigned int texSaturn = loadTextureWithCheck("textures/2k_saturn.jpg");
	unsigned int texUranus = loadTextureWithCheck("textures/2k_uranus.jpg");
	unsigned int texNeptune = loadTextureWithCheck("textures/2k_neptune.jpg");

	// 위성
	unsigned int texMoon = loadTextureWithCheck("textures/2k_moon.jpg");

	// Skybox
	unsigned int skyTex = loadTextureWithCheck("textures/2k_stars_milky_way.jpg");

	// 부가 요소
	unsigned int texSaturnRing = loadTextureWithCheck("textures/Saturn_ring-removebg.png"); // 토성 고리

	// 태양계 -------------------------------------------------------
	Sun sun;
	setupSolarSystem(sun);

	// HDR FBO ------------------------------------------------------
	unsigned int hdrFBO;
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	

	unsigned int colorBuffers[2];
	glGenTextures(2, colorBuffers);
	for (int i = 0; i < 2; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
			SCR_WIDTH, SCR_HEIGHT, 0,
			GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + i,
			GL_TEXTURE_2D,
			colorBuffers[i], 0);
	}

	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
		SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, rboDepth);

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "HDR framebuffer not complete!\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ping-pong FBO ------------------------------------------------
	unsigned int pingFBO[2];
	unsigned int pingColor[2];
	glGenFramebuffers(2, pingFBO);
	glGenTextures(2, pingColor);
	for (int i = 0; i < 2; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingColor[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
			SCR_WIDTH, SCR_HEIGHT, 0,
			GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, pingColor[i], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float lastTime = (float)glfwGetTime();
	float simYears = 0.0f;
	const float SIM_SPEED = 0.05f;

	// 루프 ---------------------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		float now = (float)glfwGetTime();
		float dt = now - lastTime;
		lastTime = now;
		std::vector<glm::vec3> planetWorldPositions;

		bool w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
		bool s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
		bool a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
		bool d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
		cam.processKeyboard(w, s, a, d, dt);

		simYears += dt * SIM_SPEED;

		glm::mat4 view = cam.getViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(cam.getFOV()),
			(float)SCR_WIDTH / (float)SCR_HEIGHT,
			0.1f, 3000.0f);

		// ================================
		// 1) HDR FBO : 태양 / 지구 / 달 등 모든 천체
		// ================================
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		sceneShader.use();
		sceneShader.setMat4("view", view);
		sceneShader.setMat4("proj", proj);
		sceneShader.setVec3("lightPos", glm::vec3(0.0f));
		sceneShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f));
		sceneShader.setVec3("viewPos", cam.getPosition());

		glBindVertexArray(sphereVAO);

		// 1-1. 태양 그리기 -------------------------------------------
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texSun);
		sceneShader.setInt("diffuseMap", 0);
		sceneShader.setInt("isSun", 1);
		sceneShader.setFloat("emissionStrength", 4.0f);

		// 매 프레임 자전 업데이트
		sun.advanceSpin(dt);

		// 회전 포함된 태양 모델 행렬 생성
		glm::mat4 sunModel = sun.buildModelMatrix(7.0f);

		// 태양 텍스처 + 파라미터
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texSun);
		sceneShader.setInt("diffuseMap", 0);
		sceneShader.setInt("isSun", 1);
		sceneShader.setFloat("emissionStrength", 4.0f);

		// 렌더링
		sceneShader.setMat4("model", sunModel);
		glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

		// 1-2. 모든 행성 순회 및 그리기 -----------------------------
		// 태양이 관리하는 행성 리스트 가져오기
		auto& planets = sun.getPlanets();

		planetWorldPositions.clear();                 
		planetWorldPositions.reserve(planets.size());

		for (auto& planet : planets)
		{
			// A. 텍스처 자동 선택 (이름 기반)
			std::string pName = planet.getParams().name;
			unsigned int currentTex = texMercury; // 기본값

			if (pName == "Mercury") currentTex = texMercury;
			else if (pName == "Venus")   currentTex = texVenus;
			else if (pName == "Earth")   currentTex = texEarth;
			else if (pName == "Mars")    currentTex = texMars;
			else if (pName == "Jupiter") currentTex = texJupiter;
			else if (pName == "Saturn")  currentTex = texSaturn;
			else if (pName == "Uranus")  currentTex = texUranus;
			else if (pName == "Neptune") currentTex = texNeptune;

			// B. 물리 업데이트 및 위치 계산 (Helper 함수 사용)
			// 이 함수 내부에서 recordTrail()도 호출됨
			glm::vec3 planetWorldPos = updatePlanetPhysics(planet, simYears, SCALE_UNITS);

			// 행성 world 좌표를 저장
			planetWorldPositions.push_back(planetWorldPos);

			// C. 행성 렌더링
			renderPlanet(planet, currentTex, sceneShader, sphereIndexCount, dt, SCALE_UNITS, planetWorldPos, sphereVAO);
			// 해성 이름이 "Saturn" 일시 고리 렌더링
			if (pName == "Saturn") {
				// 1. 투명도(Alpha Blending) 켜기
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				// 2. 텍스처 바인딩 (고리)
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texSaturnRing);
				sceneShader.setInt("diffuseMap", 0);
				sceneShader.setInt("isSun", 0); // 고리는 빛나지 않음
				sceneShader.setFloat("emissionStrength", 1.0f);

				// 3. 모델 행렬 계산 (위치 -> 회전 -> 크기)
				glm::mat4 ringModel = glm::mat4(1.0f);

				// (1) 위치: 토성 위치로 이동
				ringModel = glm::translate(ringModel, planetWorldPos);

				// (2) 회전: 고리를 눕히기 위해 X축 90도 회전
				//     + 자전축 기울기(Axial Tilt)가 있다면 여기서 같이 적용
				ringModel = glm::rotate(ringModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

				// (3) 크기: 토성 본체보다 커야 함 (예: 1.5배 ~ 2.5배)
				// 토성의 반지름을 가져와서 비례식으로 키움
				float ringScale = planet.getParams().radiusRender * 2.2f;
				ringModel = glm::scale(ringModel, glm::vec3(SCALE_UNITS * ringScale));

				sceneShader.setMat4("model", ringModel);

				// 4. 사각형(Quad) 그리기
				// createQuad로 만든 quadVAO를 사용합니다.
				glBindVertexArray(ringVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);

				// 5. 투명도 끄기 (다른 물체에 영향 안 주게)
				glDisable(GL_BLEND);
			}
			// ============================
			// D. 위성 (달) 렌더링
			// ============================
			if (!planet.satellites().empty())
			{
				Satellite& moon = planet.satellites()[0];

				// 상대 위치 (XY 기준)
				glm::vec3 rel = moon.positionRelativeToPlanet(simYears);

				// XZ 평면 정렬 적용
				glm::vec3 relXZ =
					glm::vec3(orbitToXZ * glm::vec4(rel, 1.0f));

				// 달 월드 위치
				glm::vec3 moonWorldPos =
					planetWorldPos + relXZ * SCALE_UNITS;

				// trail 기록 (월드좌표로만!)
				//moon.recordTrail(relXZ);

				// 달 렌더링
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texMoon);
				sceneShader.setInt("diffuseMap", 0);
				sceneShader.setInt("isSun", 0);
				sceneShader.setFloat("emissionStrength", 1.0f);

				moon.drawAtWorld(sceneShader, dt, SCALE_UNITS,
					moonWorldPos,
					sphereVAO, sphereIndexCount);
			}
			else
			{
				// [문제 확인] 만약 지구(Earth)인데 위성이 없다면 로그 출력!
				if (pName == "Earth") {
					std::cout << "[ERROR] Earth has NO satellites! Check setupSolarSystem()." << std::endl;
				}
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// ================================
		// 2) Bloom Blur (기존 코드 유지)
		// ================================
		bool horizontal = true;
		bool first = true;
		int passes = 10;

		blurShader.use();
		glBindVertexArray(quadVAO);
		glDisable(GL_DEPTH_TEST);

		for (int i = 0; i < passes; ++i)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[horizontal]);
			blurShader.setBool("horizontal", horizontal);

			glActiveTexture(GL_TEXTURE0);
			if (first)
				glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);
			else
				glBindTexture(GL_TEXTURE_2D, pingColor[!horizontal]);
			blurShader.setInt("image", 0);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			horizontal = !horizontal;
			if (first) first = false;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// ================================
		// 3) 기본 프레임버퍼: Skybox → Composite (기존 코드 유지)
		// ================================
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// (1) Skybox
		glDisable(GL_DEPTH_TEST);
		skyShader.use();
		skyShader.setMat4("view", view);
		skyShader.setMat4("proj", proj);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skyTex);
		skyShader.setInt("skyTex", 0);

		glBindVertexArray(sphereVAO);
		glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

		// (2) Composite
		finalShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
		finalShader.setInt("sceneTex", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingColor[!horizontal]);
		finalShader.setInt("bloomTex", 1);

		finalShader.setFloat("exposure", 1.2f);

		glBindVertexArray(quadVAO);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisable(GL_BLEND);

		// ================================
		// 4) 궤도 / 트레일 (모든 행성 루프 처리)
		// ================================
		glEnable(GL_DEPTH_TEST);

		int planetIdx = 0;

		// 다시 행성 리스트를 순회하며 궤적 그리기
		for (auto& planet : planets)
		{
			// 행성 궤적 (white + green)
			planet.drawTrail(lineShader, view, proj, simYears);
			// -----------------------------------------------------
			// 위성 궤도 트레일 렌더링
			// -----------------------------------------------------
			for (auto& sat : planet.satellites())
			{
				glm::mat4 satTrailModel =
					glm::translate(glm::mat4(1.0f),
						planetWorldPositions[planetIdx]);

				// 위성 궤적 (white + green)
				sat.drawTrail(lineShader, view, proj, satTrailModel, simYears);
			}

			planetIdx++;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
