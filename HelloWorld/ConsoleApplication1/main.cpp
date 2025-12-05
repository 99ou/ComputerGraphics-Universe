#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <vector>

#include <iomanip>
#include <sstream>
#include <string>

#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Sun.h"
#include "Planet.h"
#include "Satellite.h"
#include "Orbit.h"
#include "planetRing.h"

unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;

float simSpeedMultiplier = 1.0f;   // 시뮬레이션 배속 (0.5x ~ 10x) ?

Camera* gCamera = nullptr;
const float SCALE_UNITS = 1.0f;

// 현재 추적 중인 행성의 인덱스: -1 (NONE)
int trackingIndex = -1;
// 게임 일시 정지 플래그
bool isPaused = false;

float gTimeYears = 0.0f; // 시뮬레이션 경과 시간 (년 단위)

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

// 키보드 입력 콜백
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (gCamera)
		gCamera->processMouseMovement((float)xpos, (float)ypos);
}

// 마우스 휠 콜백
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (gCamera)
		gCamera->processMouseScroll((float)yoffset);
}

// 구 + UV 
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

// 화면 전체를 덮는 풀스크린 Quad 생성 (블룸/합성용)
void createQuad(unsigned int& vao, unsigned int& vbo)
{
	//   위치 (x,y)   //   텍스처 좌표 (u,v)
	float quadVertices[] = {
		// 1번 삼각형
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		// 2번 삼각형
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(quadVertices),
		quadVertices,
		GL_STATIC_DRAW);

	// layout(location=0) vec2 aPos;
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// layout(location=1) vec2 aTex;
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

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
	std::string texPath,
	float ax = 0.0f)
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
	p.axialTiltDeg = ax;

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

// 자전축 방향 계산 함수 추가-------------------------------------------------------------
glm::vec3 computeAxisDir(float tiltDeg)
{
	glm::mat4 R = glm::rotate(glm::mat4(1.0f),
		glm::radians(tiltDeg),
		glm::vec3(0, 0, 1));
	return glm::normalize(glm::vec3(R * glm::vec4(0, 1, 0, 0)));
}

// 선(line) 그리는 함수 추가 -------------------------------------------------------------
void drawAxisLine(const glm::vec3& center,
	const glm::vec3& axisDir,
	const Shader& axisShader,
	const glm::mat4& view,
	const glm::mat4& proj)
{
	float length = 5.0f;
	glm::vec3 p1 = center + axisDir * length;
	glm::vec3 p2 = center - axisDir * length;

	float verts[6] = { p1.x, p1.y, p1.z, p2.x, p2.y, p2.z };

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	axisShader.use();
	axisShader.setMat4("view", view);
	axisShader.setMat4("proj", proj);

	glLineWidth(1.0f);
	glDrawArrays(GL_LINES, 0, 2);

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}


// ================================================================
// 태양계 Setup
// ================================================================
void setupSolarSystem(Sun& sun)
{
	// ================================================================
	// Mercury (수성)
	// ================================================================
	PlanetParams mercuryP = createPlanetParams(
		"Mercury",                  // 행성 이름
		0.25f,                      // 렌더링 반지름
		16.0f,                      // 시뮬레이션 스케일 거리
		0.2056f,                    // 이심률
		0.240846f,                  // 공전 주기(년)
		360.0f / 58.646f,           // 자전 속도 (deg/day)
		"textures/2k_mercury.jpg"   // 텍스처 경로
	);

	// 수성 궤도 요소 (NASA JPL Elements)
	mercuryP.orbit.inclinationDeg = 7.005f;            // 궤도 경사각
	mercuryP.orbit.ascNodeDeg = 48.33167f;			   // 승교점 경도
	mercuryP.orbit.argPeriDeg = 29.124f;               // 근일점 인수
	mercuryP.orbit.meanAnomalyAtEpochDeg = 174.796f;   // 평균근점이각
	mercuryP.orbit.periodYears = 0.240846f;			   // 공전 주기(년)
	mercuryP.orbit.perihelionPrecessionDegPerYear = 0.015555f;  // 근일점 세차
	mercuryP.orbit.ascNodePrecessionDegPerYear = -0.005000f;    // 승교점 세차
	mercuryP.axialTiltDeg = 0.034f; 					// 자전축 기울기

	Planet mercury(mercuryP);      // 수성 Planet 객체 생성
	sun.addPlanet(mercury);        // 태양에 수성 등록

	// ================================================================
	// Venus (금성)
	// ================================================================
	PlanetParams venusP = createPlanetParams(
		"Venus",                    // 행성 이름
		0.60f,                      // 렌더링 반지름
		24.0f,                      // 시뮬레이션 스케일 거리
		0.0067f,                    // 이심률
		0.615197f,                  // 공전 주기(년, 224.701일)
		-360.0f / 243.0185f,        // 자전 속도 (역자전 → 음수, deg/day)
		"textures/2k_venus.jpg"     // 텍스처 경로
	);

	// 금성 궤도 요소 (NASA JPL Elements)
	venusP.orbit.inclinationDeg = 3.39471f;          // 궤도 경사각
	venusP.orbit.ascNodeDeg = 76.68069f;			 // 승교점 경도
	venusP.orbit.argPeriDeg = 54.884f;				 // 근일점 인수
	venusP.orbit.meanAnomalyAtEpochDeg = 50.115f;    // 평균근점이각
	venusP.orbit.periodYears = 0.615197f;            // 공전 주기(년)
	venusP.orbit.perihelionPrecessionDegPerYear = 0.007777f;   // 근일점 세차
	venusP.orbit.ascNodePrecessionDegPerYear = -0.003900f;     // 승교점 세차
	venusP.axialTiltDeg = 177.36f; 					 // 자전축 기울기

	Planet venus(venusP);          // 금성 Planet 객체 생성
	sun.addPlanet(venus);          // 태양에 금성 등록

	// ================================================================
	// Earth (지구)
	// ================================================================
	PlanetParams earthP = createPlanetParams(
		"Earth",                    // 행성 이름
		0.65f,                      // 렌더링 반지름
		32.0f,                      // 시뮬레이션 스케일 거리
		0.01671022f,                // 이심률
		1.000000f,                  // 공전 주기(년)
		360.0f / 0.99726968f,       // 자전 속도(항성일 0.99726968일)
		"textures/2k_earth_day.jpg" // 텍스처 경로
	);

	// 지구 궤도 요소 (NASA JPL Elements)
	earthP.mass = 5.9722e24; 						 // 질량
	earthP.orbit.inclinationDeg = 0.00005f;          // 궤도 경사각
	earthP.orbit.ascNodeDeg = -11.26064f;			 // 승교점 경도 
	earthP.orbit.argPeriDeg = 114.20783f;			 // 근일점 인수 
	earthP.orbit.meanAnomalyAtEpochDeg = 357.51716f; // 평균근점이각
	earthP.orbit.periodYears = 1.000000f;            // 공전 주기(년)
	earthP.orbit.ascNodePrecessionDegPerYear = -0.01397f;
	earthP.orbit.perihelionPrecessionDegPerYear = 0.01397f;
	earthP.axialTiltDeg = 23.439f;					 // 자전축 기울기

	Planet earth(earthP);   // 지구 생성

	// ================================================================
	// + Moon (달)
	// ================================================================
	SatelliteParams moonP{
		"Moon",                         // 이름
		0.0123f,                        // 질량 (사용 안 해도 무관)
		0.18f,                          // 렌더링 반지름
		glm::vec3(1.0f),                // 기본 색상 (텍스처 사용)
		{
			3.844f,                     // 거리
			0.0549f,                    // 이심률
			5.145f,                     // 궤도 경사각
			125.08f,                    // 승교점 경도
			318.15f,                    // 근일점 인수
			27.321661f / 365.25f,       // 공전 주기(년) — 27.321661일/365.25
			115.3654f                   // 평균근점이각
		},
		360.0f / 27.321661f,            // 자전 속도 — 조석고정
		2000,                           // trail 포인트
		"textures/2k_moon.jpg"          // 달 텍스처
	};

	moonP.mass = 7.34767309e22; 						     // 질량
	moonP.orbit.perihelionPrecessionDegPerYear = 0.1114f;    // 근지점 세차(약 3232.6일 주기)
	moonP.orbit.ascNodePrecessionDegPerYear = -0.053f;       // 승교점 세차(18.6년 주기)
	moonP.axialTiltDeg = 6.687f; 						     // 자전축 기울기

	Satellite moon(moonP);        // 달 생성
	earth.addSatellite(moon);     // 지구 위성으로 달 등록
	sun.addPlanet(earth);		  // 태양에 지구 추가

	// ================================================================
	// Mars (화성)
	// ================================================================
	PlanetParams marsP = createPlanetParams(
		"Mars",                     // 행성 이름
		0.45f,                      // 렌더링 반지름
		48.0f,                      // 시뮬레이션 스케일 거리
		0.0934f,                    // 이심률
		1.8808476f,                 // 공전 주기(년, 686.980일)
		360.0f / 1.025957f,         // 자전 속도(화성 하루 1.025957일)
		"textures/2k_mars.jpg"      // 텍스처 경로
	);

	// 화성 궤도 요소 (NASA JPL Elements)
	marsP.orbit.inclinationDeg = 1.85061f;           // 궤도 경사각
	marsP.orbit.ascNodeDeg = 49.57854f;				 // 승교점 경도 
	marsP.orbit.argPeriDeg = 286.502f;				 // 근일점 인수 
	marsP.orbit.meanAnomalyAtEpochDeg = 19.412f;     // 평균근점이각
	marsP.orbit.periodYears = 1.8808476f;            // 공전 주기(년)
	marsP.orbit.perihelionPrecessionDegPerYear = 0.004166f;  // 근일점 세차
	marsP.orbit.ascNodePrecessionDegPerYear = -0.002000f;    // 승교점 세차

	Planet mars(marsP);            // 화성 Planet 객체 생성
	sun.addPlanet(mars);           // 태양에 화성 등록


	// ================================================================
	// Jupiter (목성)
	// ================================================================
	PlanetParams jupiterP = createPlanetParams(
		"Jupiter",                  // 행성 이름
		2.8f,                       // 렌더링 반지름
		94.0f,                     // 시뮬레이션 스케일 거리
		0.0489f,                    // 이심률
		11.862615f,                 // 공전 주기(년)
		360.0f / 0.41354f,          // 자전 속도 (목성 하루 0.41354일)
		"textures/2k_jupiter.jpg"   // 텍스처 경로
	);

	// 목성 궤도 요소 (NASA JPL Elements)
	jupiterP.mass = 1.8985e27f; 					  // 질량
	jupiterP.orbit.inclinationDeg = 1.303f;           // 궤도 경사각
	jupiterP.orbit.ascNodeDeg = 100.55615f;			  // 승교점 경도
	jupiterP.orbit.argPeriDeg = 273.867f;			  // 근일점 인수 
	jupiterP.orbit.meanAnomalyAtEpochDeg = 20.020f;   // 평균근점이각 
	jupiterP.orbit.periodYears = 11.862615f;          // 공전 주기(년)
	jupiterP.orbit.perihelionPrecessionDegPerYear = 0.000194f;  // 근일점 세차
	jupiterP.orbit.ascNodePrecessionDegPerYear = -0.000150f;	// 승교점 세차
	jupiterP.axialTiltDeg = 3.13f; 					  // 자전축 기울기

	jupiterP.ring.enabled = true;
	jupiterP.ring.innerRadius = 2.0f;   // 목성 고리 내경 
	jupiterP.ring.outerRadius = 2.03f;  // 목성 고리 외경
	jupiterP.ring.alpha = 0.005f;		// 목성 고리는 아주 희미함
	jupiterP.ring.texturePath = "textures/2k_jupiter_ring_alpha.png";  // 텍스쳐 경로

	Planet jupiter(jupiterP);			// 목성 생성

	// ================================================================
	// Europa (유로파)
	// ================================================================
	SatelliteParams europaP{
		"Europa",                      // 위성 이름
		0.0625f,                       // 위성 질량
		0.20f,                         // 렌더링 반지름
		glm::vec3(1.0f),               // 색상 (텍스처 사용하므로 의미 X)
		{
			7.5f,                      // 거리 스케일(목성 중심)
			0.0094f,                   // 이심률
			0.470f,                    // 궤도 경사각
			219.106f,                  // 승교점 경도
			88.970f,                   // 근지점 인수
			3.551181f / 365.25f,       // 공전 주기(년) — 3.551181일
			128.0f                     // 평균근점이각
		},
		360.0f / 3.551181f,            // 자전 속도 — 조석고정
		2000,                          // trail 포인트 개수
		"textures/2k_europa.jpg"       // 유로파 텍스처
	};

	europaP.mass = 4.80e22f; 		   // 질량
	europaP.orbit.perihelionPrecessionDegPerYear = 0.0101f;   // 근지점 세차
	europaP.orbit.ascNodePrecessionDegPerYear = -0.0111f;	  // 승교점 세차
	europaP.axialTiltDeg = 0.1f; 							  // 자전축 기울기

	Satellite europa(europaP);      // 유로파 생성
	jupiter.addSatellite(europa);	// 목성에 유로파 위성 등록
	sun.addPlanet(jupiter);         // 태양에 목성 등록

	// ================================================================
	// Saturn (토성)
	// ================================================================
	PlanetParams saturnP = createPlanetParams(
		"Saturn",                   // 행성 이름
		2.5f,                       // 렌더링 반지름
		140.0f,                     // 시뮬레이션 스케일 거리
		0.0565f,                    // 이심률
		29.4571f,                   // 공전 주기(년, 10759일)
		360.0f / 0.44401f,          // 자전 속도(토성 하루 0.44401일)
		"textures/2k_saturn.jpg"    // 텍스처 경로
	);

	// 토성 궤도 요소 (NASA JPL Elements)
	saturnP.mass = 5.683e26f; 					     // 질량
	saturnP.orbit.inclinationDeg = 2.485f;           // 궤도 경사각
	saturnP.orbit.ascNodeDeg = 113.662f;			 // 승교점 경도
	saturnP.orbit.argPeriDeg = 339.392f;			 // 근일점 인수
	saturnP.orbit.meanAnomalyAtEpochDeg = 317.020f;  // 평균근점이각
	saturnP.orbit.periodYears = 29.4571f;			 // 공전 주기(년)
	saturnP.orbit.perihelionPrecessionDegPerYear = 0.000100f;  // 근일점 세차
	saturnP.orbit.ascNodePrecessionDegPerYear = -0.000070f;	   // 승교점 세차
	saturnP.axialTiltDeg = 26.73f; 					 // 자전축 기울기

	saturnP.ring.enabled = true;	// 고리 활성화
	saturnP.ring.innerRadius = 1.2f;// 토성 고리 내경
	saturnP.ring.outerRadius = 2.4f;// 토성 고리 외경
	saturnP.ring.alpha = 0.9f;		// 토성 고리 투명도
	saturnP.ring.texturePath = "textures/2k_saturn_ring_alpha.png";

	Planet saturn(saturnP);         // 토성 생성

	// ================================================================
	// Titan (타이탄)
	// ================================================================
	SatelliteParams titanP{
		"Titan",                        // 위성 이름
		0.2225f,                        // 질량
		0.1105f,                        // 렌더링 반지름
		glm::vec3(1.0f),                // 색상 (텍스처 사용)
		{
			10.0f,                      // 거리 스케일(토성 중심)
			0.0288f,                    // 이심률
			0.34854f,                   // 궤도 경사각
			168.77f,                    // 승교점 경도
			186.585f,                   // 근지점 인수
			15.945f / 365.25f,          // 공전 주기(년) — 15.945일
			17.0f                       // 평균근점이각
		},
		360.0f / 15.945f,               // 자전 속도 — 조석고정
		2000,                           // trail 포인트 개수
		"textures/2k_titan.jpg"         // 타이탄 텍스처
	};

	titanP.mass = 1.3452e23f; 		    // 질량
	titanP.orbit.perihelionPrecessionDegPerYear = 0.0038f;    // 근지점 세차
	titanP.orbit.ascNodePrecessionDegPerYear = -0.0046f;	  // 승교점 세차
	titanP.axialTiltDeg = 0.3f; 							  // 자전축 기울기

	Satellite titan(titanP);         // 타이탄 생성
	saturn.addSatellite(titan);		 // 토성에 타이탄 추가
	sun.addPlanet(saturn);           // 태양에 토성 등록

	// ================================================================
	// Uranus (천왕성)
	// ================================================================
	PlanetParams uranusP = createPlanetParams(
		"Uranus",                    // 행성 이름
		1.80f,                       // 렌더링 반지름
		190.0f,                      // 시뮬레이션 스케일 거리
		0.04726f,                    // 이심률
		84.016846f,                  // 공전 주기(년)
		-(360.0f / 0.71833f),        // 자전 속도 (17.24h = 0.71833일, 역행 = 음수)
		"textures/2k_uranus.jpg",    // 텍스처 경로
		97.77f                       // 자전축 기울기
	);

	// 천왕성 궤도 요소 (NASA JPL Elements)
	uranusP.mass = 8.6810e25f; 					      // 질량
	uranusP.orbit.inclinationDeg = 0.773f;            // 궤도 경사각
	uranusP.orbit.ascNodeDeg = 74.006f;               // 승교점 경도
	uranusP.orbit.argPeriDeg = 96.998857f;            // 근일점 인수
	uranusP.orbit.meanAnomalyAtEpochDeg = 142.2386f;  // 평균근점이각
	uranusP.orbit.periodYears = 84.016846f;           // 공전 주기(년, 동일하게 유지)
	uranusP.orbit.perihelionPrecessionDegPerYear = 0.000095f;   // 근일점 세차
	uranusP.orbit.ascNodePrecessionDegPerYear = -0.000095f;		// 승교점 세차
	uranusP.axialTiltDeg = 97.77f; 					  // 자전축 기울기

	uranusP.ring.enabled = true;	 // 고리 활성화
	uranusP.ring.innerRadius = 1.52f;// 천왕성 고리 내경
	uranusP.ring.outerRadius = 1.55f;// 천왕성 고리 외경
	uranusP.ring.alpha = 0.005f;	 // 천왕성 고리 투명도
	uranusP.ring.texturePath = "textures/2k_uranus_ring_alpha.png";

	Planet uranus(uranusP);        // 천왕성 객체 생성

	SatelliteParams oberonP{
		"Oberon",				   // 위성 이름
		0.225f,					   // 질량
		0.054f,                    // 렌더링 반지름
		glm::vec3(1.0f),		   // 색상 (텍스처 사용)
		{
			8.0f,                  // 거리 스케일 (천왕성 중심)
			0.0014f,               // 이심률
			0.34854f,              // 궤도 경사각
			168.77f,               // 승교점 경도
			186.585f,              // 근지점 인수
			13.463234f / 365.25f,  // 공전 주기(년)
			17.0f                  // 평균근점이각
		},
		360.0f / 13.463234f,       // 조석 고정
		2000,
		"textures/2k_oberon.jpg"
	};

	oberonP.mass = 3.014e21f; 	   // 질량
	oberonP.orbit.perihelionPrecessionDegPerYear = 0.0035f;   // 근지점 세차 (조금 작게)
	oberonP.orbit.ascNodePrecessionDegPerYear = -0.0042f;  // 승교점 세차
	oberonP.axialTiltDeg = 0.0f;   // 자전축 기울기

	Satellite oberon(oberonP);     // 오베론 생성
	uranus.addSatellite(oberon);   // 천왕성에 타이탄 추가
	sun.addPlanet(uranus);         // 태양에 천왕성 등록

	// ================================================================
	// Neptune (해왕성)
	// ================================================================
	PlanetParams neptuneP = createPlanetParams(
		"Neptune",                   // 행성 이름
		1.70f,                       // 렌더링 반지름
		230.0f,                      // 시뮬레이션 스케일 거리
		0.008678f,                   // 이심률
		164.8922113f,                // 공전 주기(년)
		360.0f / 0.67125f,           // 자전 속도 (자전주기 약 16.11h = 0.67125 day)
		"textures/2k_neptune.jpg"    // 텍스처 경로
	);

	// 해왕성 궤도 요소 (NASA JPL Elements)
	neptuneP.mass = 1.02413e26f;
	neptuneP.orbit.inclinationDeg = 1.769f;       // 궤도 경사각
	neptuneP.orbit.ascNodeDeg = 131.784f;		  // 승교점 경도
	neptuneP.orbit.argPeriDeg = 273.187f;		  // 근일점 인수
	neptuneP.orbit.meanAnomalyAtEpochDeg = 259.908f;		   // 평균근점이각
	neptuneP.orbit.periodYears = 164.8922113f;	  // 공전 주기(년)
	neptuneP.orbit.perihelionPrecessionDegPerYear = 0.000032f; // 근일점 세차
	neptuneP.orbit.ascNodePrecessionDegPerYear = -0.000032f;   // 승교점 세차
	neptuneP.axialTiltDeg = 28.32f; 			  // 자전축 기울기

	neptuneP.ring.enabled = true;	  // 고리 활성화
	neptuneP.ring.innerRadius = 1.6f; // 해왕성 고리 내경
	neptuneP.ring.outerRadius = 1.63f;// 해왕성 고리 외경
	neptuneP.ring.alpha = 0.005f;	  // 해왕성 고리 투명도
	neptuneP.ring.texturePath = "textures/2k_neptune_ring_alpha.png";

	Planet neptune(neptuneP);   // 해왕성 객체 생성

	// ================================================================
	// Triton (트리톤)
	// ================================================================
	SatelliteParams tritonP{
		"Triton",                       // 위성 이름
		2.139e22f,                      // 질량
		0.0934f,                        // 렌더링 반지름
		glm::vec3(1.0f),                // 색상 (텍스처 사용)
		{
			6.0f,                       // 거리 스케일 (해왕성 중심)
			0.000016f,                  // 이심률
			157.0f,                     // 궤도 경사각 (역행)
			200.0f,                     // 승교점 경도
			39.48f,                     // 근지점 인수
			5.876854f / 365.25f,        // 실제 공전 주기(년)
			358.0f                      // 평균근점이각
		},
		360.0f / 5.876854f,             // 조석고정
		2000,                           // trail 포인트 개수
		"textures/2k_triton.jpg"		// 텍스처 경로
	};

	tritonP.mass = 2.139e22f;								  // 질량
	tritonP.orbit.perihelionPrecessionDegPerYear = 0.0050f;   // 근지점 세차
	tritonP.orbit.ascNodePrecessionDegPerYear = -0.0050f; 	  // 승교점 세차
	tritonP.axialTiltDeg = 0.3f; 							  // 자전축 기울기

	Satellite triton(tritonP);           // 트리톤 생성
	neptune.addSatellite(triton);		 // 해왕성에 트리톤 추가
	sun.addPlanet(neptune);				 // 태양에 해왕성 등록

	// ---------------------------------------------------------
	// 9. 아스가르드 (Asgard) - 수직 궤도 행성
	// ---------------------------------------------------------
	PlanetParams asgardP = createPlanetParams(
		"Asgard",
		1.5f,   // 크기: 지구보다 좀 큼
		45.0f,  // 거리: 목성(40)과 토성(50) 사이를 가로지름
		0.0f,   // 이심률
		2.0f,   // 공전 속도: 꽤 빠름
		5.0f,   // 자전 속도
		"textures/2k_asgard.jpg" // 텍스처 (없으면 달 텍스처 재활용)
	);

	// 궤도 경사각을 90도로 설정 (수직 공전)
	asgardP.orbit.inclinationDeg = 90.0f;

	// 궤도 교차 지점 설정 (0도면 X축에서 교차, 90도면 Z축에서 교차)
	asgardP.orbit.ascNodeDeg = 45.0f;

	Planet asgard(asgardP);
	sun.addPlanet(asgard); // 태양계 등록
}

// render helper: 한 행성(Planet)을 렌더링
void renderPlanet(Planet& planet,
	unsigned int textureID,
	Shader& shader,
	unsigned int sphereIndexCount,
	float dtSeconds,
	float worldScale,
	const glm::vec3& worldPos,
	unsigned int sphereVAO,
	bool isSun = false,
	float emissionStrength = 1.0f)
{
	// 시뮬레이션 시간 기준 자전 업데이트 (배속 + 일시정지 반영)
	float dtSimDays = (isPaused ? 0.0f : dtSeconds * simSpeedMultiplier);
	planet.advanceSpin(dtSimDays);

	// 텍스처 + 쉐이더 설정
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	shader.setInt("diffuseMap", 0);
	shader.setInt("isSun", isSun ? 1 : 0);
	shader.setFloat("emissionStrength", emissionStrength);

	glm::mat4 model = planet.buildModelMatrix(worldScale, worldPos);
	shader.setMat4("model", model);

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void renderSatellites(Planet& planet,
	Shader& shader,
	float dt,
	float simTime,
	float scale,
	unsigned int sphereVAO,
	unsigned int indexCount,
	glm::vec3 planetWorldPos, // 이미 회전(XZ) + 스케일 + 질량중심 보정이 적용된 행성 위치
	unsigned int texMoon,
	unsigned int texEuropa,
	unsigned int texTitan)
{
	// 위성이 없으면 바로 리턴
	if (planet.satellites().empty()) return;

	// 모든 위성 순회
	for (auto& sat : planet.satellites())
	{
		std::string sName = sat.getParams().name;
		unsigned int currentTex = texMoon; // 기본값: 달 텍스처

		// 1. 이름에 따라 텍스처 자동 선택
		if (sName == "Moon")        currentTex = texMoon;
		else if (sName == "Europa") currentTex = texEuropa;
		else if (sName == "Titan")  currentTex = texTitan;
		// else currentTex = texGenericRock; // 필요 시 일반 위성 텍스처

		// 2. 텍스처 바인딩
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, currentTex);
		shader.setInt("diffuseMap", 0);
		shader.setInt("isSun", 0);
		shader.setFloat("emissionStrength", 1.0f);

		// 3. 상대 위치 계산 (모든 궤도는 XY → XZ로 회전해서 사용)
		glm::vec3 rel = sat.positionRelativeToPlanet(simTime);                 // XY 기준
		glm::vec3 relXZ = glm::vec3(orbitToXZ * glm::vec4(rel, 1.0f));         // XZ 기준으로 회전

		// 4. 최종 world 위치
		// planetWorldPos는 이미 (physPos + Re) * scale 에 orbitToXZ를 적용한 값
		// relXZ는 시뮬레이션 단위이므로 scale을 곱해서 같은 단위로 맞춘다.
		glm::vec3 satWorldPos = planetWorldPos + relXZ * scale;

		// 5. 렌더링 (자전도 시뮬레이션 배속 반영)
		float dtSimDays = (isPaused ? 0.0f : dt * simSpeedMultiplier);
		sat.drawAtWorld(shader, dtSimDays, scale, satWorldPos, sphereVAO, indexCount);
	}
}

// -------------------------------------------------------------
//  행성 + 위성 질량중심(barycenter) 보정 포함 updatePlanetPhysics()
//  - 모든 위성의 질량과 상대 위치를 합산하여 행성의 질량중심 보정 적용
//  - 달 / 유로파 / 타이탄 모두 서로 다른 barycenter 반영
// -------------------------------------------------------------
void updatePlanetPhysics(
	Planet& planet,
	float simYears,
	const glm::mat4& orbitToXZ,
	float scaleUnits,
	glm::vec3& outWorldPos)
{
	// -----------------------------
	// 1) 행성의 원래(순수) 궤도 위치
	// -----------------------------
	glm::vec3 physPos = planet.positionAroundSun(simYears);

	// -----------------------------
	// 2) 위성 목록
	// -----------------------------
	const auto& sats = planet.satellites();

	// 위성이 없다면 보정 없이 원래 위치 사용
	if (sats.empty())
	{
		outWorldPos = glm::vec3(
			orbitToXZ * glm::vec4(physPos * scaleUnits, 1.0f));
		return;
	}

	// -----------------------------
	// 3) 질량 정보
	// -----------------------------
	float Mp = planet.getParams().mass;  // 행성 질량

	float sumMs = 0.0f;                  // 위성 총 질량
	for (const auto& s : sats)
		sumMs += s.getParams().mass;

	// -----------------------------
	// 4) 모든 위성 기반 barycenter offset 계산
	// -----------------------------
	glm::vec3 baryOffset(0.0f);

	for (const auto& sat : sats)
	{
		// (XY 기준) 위성의 상대 위치 (행성 중심)
		glm::vec3 rel = sat.positionRelativeToPlanet(simYears);

		// barycenter 기여량
		baryOffset += -(sat.getParams().mass / (Mp + sumMs)) * rel;
	}

	// -----------------------------
	// 5) XY → XZ 평면 회전 적용
	// -----------------------------
	glm::vec3 baryOffsetXZ =
		glm::vec3(orbitToXZ * glm::vec4(baryOffset, 1.0f));

	// -----------------------------
	// 6) 최종적으로 보정된 행성의 worldPos
	// -----------------------------
	glm::vec3 planetWorldPos =
		glm::vec3(orbitToXZ * glm::vec4(physPos, 1.0f)) +
		baryOffsetXZ;

	planetWorldPos *= scaleUnits;

	outWorldPos = planetWorldPos;
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
		"uniform float ringAlpha;\n"
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
		"  FragColor = vec4(color, ringAlpha);\n"
		"  float brightness = dot(color, vec3(0.2126,0.7152,0.0722));\n"
		"  if(brightness > 1.0) BrightColor = vec4(color,1.0);\n"
		"  else BrightColor = vec4(0.0,0.0,0.0,1.0);\n"
		"}\n";

	Shader sceneShader(sceneVert, sceneFrag);

	sceneShader.use();
	sceneShader.setFloat("ringAlpha", 1.0f);   // 기본값: 불투명

	// ================================
	// Ring Shader (고리 전용)
	// ================================
	const char* ringVert =
		"#version 330 core\n"
		"layout(location=0) in vec3 aPos;\n"
		"layout(location=1) in vec2 aTex;\n"
		"out vec2 TexCoord;\n"
		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 proj;\n"
		"void main(){\n"
		"  TexCoord = aTex;\n"
		"  gl_Position = proj * view * model * vec4(aPos,1.0);\n"
		"}\n";

	const char* ringFrag =
		"#version 330 core\n"
		"in vec2 TexCoord;\n"
		"layout(location=0) out vec4 FragColor;\n"
		"layout(location=1) out vec4 BrightColor;\n"
		"uniform sampler2D ringTex;\n"
		"uniform float alpha;\n"
		"void main(){\n"
		"  vec4 c = texture(ringTex, TexCoord);\n"
		"  if(c.a < 0.05) discard;\n"
		"  vec4 col = vec4(c.rgb, c.a * alpha);\n"
		"  FragColor = col;\n"
		"  BrightColor = vec4(0.0, 0.0, 0.0, 1.0); // ★ 고리는 항상 Bloom 0\n"
		"}\n";

	Shader ringShader(ringVert, ringFrag);

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

	// =====================
	// Axis Line Shader
	// =====================
	const char* axisVert =
		"#version 330 core\n"
		"layout(location=0) in vec3 aPos;\n"
		"uniform mat4 view;\n"
		"uniform mat4 proj;\n"
		"void main(){\n"
		"   gl_Position = proj * view * vec4(aPos, 1.0);\n"
		"}\n";

	const char* axisFrag =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"void main(){ FragColor = vec4(1.0); }\n";

	Shader axisShader(axisVert, axisFrag);

	// 기하 생성 ----------------------------------------------------
	unsigned int sphereVAO, sphereIndexCount;
	createSphere(sphereVAO, sphereIndexCount);

	// 화면 전체 Quad (블룸/합성용)
	unsigned int quadVAO, quadVBO;
	createQuad(quadVAO, quadVBO);

	// 행성 -------------------------------------------------------
	unsigned int texSun = loadTextureWithCheck("textures/2k_sun.jpg");
	unsigned int texMercury = loadTextureWithCheck("textures/2k_mercury.jpg");
	unsigned int texVenus = loadTextureWithCheck("textures/2k_venus_surface.jpg");
	unsigned int texEarth = loadTextureWithCheck("textures/2k_earth_daymap.jpg");
	unsigned int texMars = loadTextureWithCheck("textures/2k_mars.jpg");
	unsigned int texJupiter = loadTextureWithCheck("textures/2k_jupiter.jpg");
	unsigned int texSaturn = loadTextureWithCheck("textures/2k_saturn.jpg");
	unsigned int texUranus = loadTextureWithCheck("textures/2k_uranus.jpg");
	unsigned int texNeptune = loadTextureWithCheck("textures/2k_neptune.jpg");
	unsigned int texAsgard = loadTextureWithCheck("textures/2k_asgard.jpg");

	// 위성 ---------------------------------------------------
	unsigned int texMoon = loadTextureWithCheck("textures/2k_moon.jpg");
	unsigned int texEuropa = loadTextureWithCheck("textures/2k_europa.jpg");
	unsigned int texTitan = loadTextureWithCheck("textures/2k_titan.jpg");
	unsigned int texOberon = loadTextureWithCheck("textures/2k_oberon.jpg");
	unsigned int texTriton = loadTextureWithCheck("textures/2k_triton.jpg");

	// 고리 -----------------------------------------------------
	unsigned int texJupiterRing = loadTextureWithCheck("textures/2k_jupiter_ring_alpha.png");
	unsigned int texSaturnRing = loadTextureWithCheck("textures/2k_saturn_ring_alpha.png");
	unsigned int texUranusRing = loadTextureWithCheck("textures/2k_uranus_ring_alpha.png");
	unsigned int texNeptuneRing = loadTextureWithCheck("textures/2k_neptune_ring_alpha.png");

	// Skybox
	unsigned int skyTex = loadTextureWithCheck("textures/2k_stars_milky_way.jpg");

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

	// 현실 시간 1초당 시뮬레이션에서 1일이 지나도록 설정
	// simYears는 "년" 단위이므로 1일 = 1/365년
	const float SIM_SPEED = 1.0f / 365.0f;

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

		// ===========================
		// Simulation Speed Control
		// Shift = speed up
		// Ctrl  = slow down
		// + 100x / - 100x
		// ===========================
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
			glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
		{
			simSpeedMultiplier += dt * 2.0f;
			if (simSpeedMultiplier > 100.0f)
				simSpeedMultiplier = 100.0f;
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
			glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
		{
			simSpeedMultiplier -= dt * 2.0f;
			if (simSpeedMultiplier < 0.1f)
				simSpeedMultiplier = 0.1f;
		}

		// 행성 추적 ------------------------------------------------
		// [추가] 추적 대상 선택 (숫자키 1 ~ 8)
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) trackingIndex = 0; // 수성
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) trackingIndex = 1; // 금성
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) trackingIndex = 2; // 지구
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) trackingIndex = 3; // 화성
		if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) trackingIndex = 4; // 목성
		if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) trackingIndex = 5; // 토성
		if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) trackingIndex = 6; // 천왕성
		if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) trackingIndex = 7; // 해왕성
		if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) trackingIndex = 8; // 아스가르드

		// [추가] 추적 해제 (ESC)
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			trackingIndex = -1;
			gCamera->stopTracking();
		}
		static bool zeroKeyPressed = false; // 이전 프레임 키 상태 저장
		if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
		{
			if (!zeroKeyPressed) // 방금 눌리기 시작했다면
			{
				isPaused = !isPaused; // 상태 반전 (On <-> Off)
				std::cout << "Simulation " << (isPaused ? "PAUSED" : "RESUMED") << std::endl;
				zeroKeyPressed = true;
			}
		}
		else
		{
			zeroKeyPressed = false; // 키를 떼면 리셋
		}

		if (!isPaused) {
			simYears += dt * SIM_SPEED * simSpeedMultiplier;
		}

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

		// 태양 자전 업데이트 (배속 + 일시정지 반영)
		float dtSimDaysForSun = (isPaused ? 0.0f : dt * simSpeedMultiplier);
		sun.advanceSpin(dtSimDaysForSun);

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

		int pIdx = 0; // 행성 인덱스 카운터

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
			else if (pName == "Asgard") currentTex = texAsgard;

			// B. 물리 업데이트 및 위치 계산 (Helper 함수 사용)
			// 이 함수 내부에서 recordTrail()도 호출됨
			glm::vec3 planetWorldPos;
			updatePlanetPhysics(planet, simYears, orbitToXZ, SCALE_UNITS, planetWorldPos);

			// 행성 world 좌표를 저장
			planetWorldPositions.push_back(planetWorldPos);

			// ==========================================
			// [추가] 카메라 추적 로직 (핵심!)
			// ==========================================
			if (trackingIndex == pIdx)
			{
				// 1. 현재 추적 모드가 꺼져있다면 -> 켜기
				if (!gCamera->getIsTracking()) {
					gCamera->startTracking(planetWorldPos);
				}
				// 2. 매 프레임 행성의 새로운 위치를 카메라에 전달
				gCamera->updateTargetPosition(planetWorldPos);
			}
			// ==========================================

			// C. 행성 렌더링
			renderPlanet(planet, currentTex, sceneShader, sphereIndexCount, dt, SCALE_UNITS, planetWorldPos, sphereVAO);

			// 고리 렌더 (Saturn / Jupiter 등)
			if (planet.getParams().ring.enabled && planet.ring)
			{
				glm::mat4 planetModel =
					planet.buildModelMatrix(SCALE_UNITS, planetWorldPos);

				planet.ring->setShader(&ringShader);

				// ★ 알파 블렌딩 켜기
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				planet.ring->render(planetModel, view, proj);

				// ★ 다시 끄기 (다른 렌더에 영향 X)
				glDisable(GL_BLEND);
			}

			// 행성별 위성 렌더링
			renderSatellites(planet, sceneShader,
				dt, simYears, SCALE_UNITS,
				sphereVAO, sphereIndexCount,
				planetWorldPos,
				texMoon,
				texEuropa,
				texTitan);

			// [추가] 루프 끝날 때 인덱스 증가
			pIdx++;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// ================================
		// 2) Bloom Blur
		// ================================
		bool horizontal = true;
		bool first = true;
		int passes = 10;

		blurShader.use();
		glDisable(GL_DEPTH_TEST);

		// ★ 풀스크린 Quad VAO 바인딩
		glBindVertexArray(quadVAO);

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

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// 풀스크린 Quad VAO 바인딩
		glBindVertexArray(quadVAO);
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

		// ==============================
		// ★ 선택된 행성의 자전축 렌더링
		// ==============================
		if (trackingIndex >= 0)
		{
			auto& plist = sun.getPlanets();
			if (trackingIndex < plist.size())
			{
				Planet& P = plist[trackingIndex];

				// 1) 행성 위치 구하기 (이미 업데이트된 worldPos 저장되어 있어야 함)
				glm::vec3 pos = planetWorldPositions[trackingIndex];

				// 2) 행성의 자전축 방향 계산
				float tilt = P.getParams().axialTiltDeg;
				glm::vec3 axisDir = computeAxisDir(tilt);

				// 3) 선 그리기
				drawAxisLine(pos, axisDir,
					axisShader,
					cam.getViewMatrix(),
					proj);
			}
		}

		// ===========================
		// Update Window Title (Show Speed)
		// ===========================
		{
			std::stringstream ss;
			ss << "Speed : x " << std::fixed << std::setprecision(1) << simSpeedMultiplier;

			glfwSetWindowTitle(window, ss.str().c_str());
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
