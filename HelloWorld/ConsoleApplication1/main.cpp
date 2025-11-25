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
// 태양계 셋업 -------------------------------------------------------
// 클래스명 객체명 (이름, 렌더크기, 태양거리, 이심률, 공전주기, 자전속도, 텍스처)
void setupSolarSystem(Sun& sun)
{
	// 행성 ----------------------------------------------------------
	// 1) 수성 (Mercury)
	// radius: 0.25, distance: 12, ecc: 0, orbSpeed: 1.2, spinSpeed: 0.1
	PlanetParams mercuryP = createPlanetParams(
		"Mercury", 0.25f, 12.0f, 0.0f, 1.2f, 0.1f, "textures/mercury.jpg"
	);
	mercuryP.orbit.argPeriDeg = 29.124f; // 수성의 근일점 편각
	Planet mercury(mercuryP);
	sun.addPlanet(mercury); // [0] 등록
	// 2) 금성 (Venus) - 역자전(-0.05)
	// radius: 0.60, distance: 18, ecc: 0, orbSpeed: 0.9, spinSpeed: -0.05
	PlanetParams venusP = createPlanetParams(
		"Venus", 0.60f, 18.0f, 0.0f, 0.9f, -0.05f, "textures/venus.jpg"
	);
	Planet venus(venusP);
	sun.addPlanet(venus); // [1] 등록
	// 3) 지구 (Earth)
	// radius: 0.65, distance: 24, ecc: 0, orbSpeed: 0.6, spinSpeed: 1.0
	PlanetParams earthP = createPlanetParams(
		"Earth", 0.65f, 24.0f, 0.0f, 0.6f, 1.0f, "textures/earth_day.jpg"
	);
	// *세부 튜닝이 필요한 경우 여기서 수정
	earthP.orbit.argPeriDeg = 102.9f;
	Planet earth(earthP);

	// 위성 등록은 해당 Sun에 추가하기 전에 해당 행성에 추가하기 
	// [2] Earth 위성 등록 ====================================================================
	SatelliteParams moonP = createSatelliteParams(
		"Moon", 0.18f, 3.0f, 0.0549f, 27.32f / 365.25f, 10.0f, "textures/2k_moon.jpg"
	);
	// *달은 궤도 경사각이 중요하므로 추가 설정
	moonP.orbit.inclinationDeg = 5.145f;
	moonP.orbit.ascNodeDeg = 125.0f;
	moonP.orbit.argPeriDeg = 318.0f;
	Satellite moon(moonP);
	earth.addSatellite(moon);

	// ------------------------------------------------------------------------------------------

	sun.addPlanet(earth); // [2] 등록
	// 4) 화성 (Mars)
	// radius: 0.34, distance: 30, ecc: 0, orbSpeed: 0.5, spinSpeed: 0.97
	PlanetParams marsP = createPlanetParams(
		"Mars", 0.34f, 30.0f, 0.0f, 0.5f, 0.97f, "textures/mars.jpg"
	);
	Planet mars(marsP);
	sun.addPlanet(mars); // [3] 등록
	// 5) 목성 (Jupiter) - 가장 빠른 자전(2.4)
	// radius: 7.0, distance: 40, ecc: 0, orbSpeed: 0.3, spinSpeed: 2.4
	PlanetParams jupiterP = createPlanetParams(
		"Jupiter", 7.0f, 40.0f, 0.0f, 0.3f, 2.4f, "textures/jupiter.jpg"
	);
	Planet jupiter(jupiterP);
	sun.addPlanet(jupiter); // [4] 등록
	// 6) 토성 (Saturn)
	// radius: 5.8, distance: 50, ecc: 0, orbSpeed: 0.25, spinSpeed: 2.2
	PlanetParams saturnP = createPlanetParams(
		"Saturn", 5.8f, 50.0f, 0.0f, 0.25f, 2.2f, "textures/saturn.jpg"
	);
	Planet saturn(saturnP);
	sun.addPlanet(saturn); // [5] 등록

	// 7) 천왕성 (Uranus) - 역자전(-1.5)
	// radius: 2.5, distance: 60, ecc: 0, orbSpeed: 0.18, spinSpeed: -1.5f
	PlanetParams uranusP = createPlanetParams(
		"Uranus", 2.5f, 60.0f, 0.0f, 0.18f, -1.5f, "textures/uranus.jpg"
	);
	Planet uranus(uranusP);
	sun.addPlanet(uranus); // [6] 등록

	// 8) 해왕성 (Neptune)
	// radius: 2.4, distance: 70, ecc: 0, orbSpeed: 0.12, spinSpeed: 1.1
	PlanetParams neptuneP = createPlanetParams(
		"Neptune", 2.4f, 70.0f, 0.0f, 0.12f, 1.1f, "textures/neptune.jpg"
	);
	Planet neptune(neptuneP);
	sun.addPlanet(neptune); // [7] 등록
}

// render helper: 한 행성(Planet)을 렌더링
void renderPlanet(Planet& planet,
	unsigned int textureID,
	Shader& shader,
	unsigned int sphereIndexCount,
	float dtSeconds,
	float worldScale,
	const glm::vec3& worldPos,
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
	glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}
// 행성 위치 업데이트 및 렌더링 좌표 계산 함수
// 리턴값: 행성의 최종 렌더링 위치 (World Position)
glm::vec3 updatePlanetPhysics(Planet& planet, float simYears, float scaleUnits)
{
	// 1. 기본 공전 위치 계산
	glm::vec3 physPos = planet.positionAroundSun(simYears);

	// 2. 궤적 기록 (오타 방지!)
	planet.recordTrail(physPos);

	// 3. 렌더링용 월드 좌표 변환
	glm::vec3 worldPos = physPos * scaleUnits;

	// 4. 위성(달)이 있는 경우 무게중심(Barycenter) 보정
	//    (위성이 없으면 위에서 계산한 worldPos가 그대로 유지됨)
	auto& sats = planet.satellites();
	if (!sats.empty())
	{
		// 첫 번째 위성만 계산 (다중 위성일 경우 반복문 필요하지만, 여기선 달 하나라고 가정)
		Satellite& sat = sats[0];

		// 행성과 위성의 질량 가져오기
		float Mp = planet.getParams().mass;
		float Ms = sat.getParams().mass;

		// 위성의 상대 위치 계산
		glm::vec3 rel = sat.positionRelativeToPlanet(simYears);
		sat.recordTrail(rel); // 위성 궤적 기록

		// 무게중심 보정 벡터 계산
		glm::vec3 Re = -(Ms / (Mp + Ms)) * rel; // 행성이 밀려나는 위치
		// glm::vec3 Rm = Re + rel;             // 위성의 실제 위치 (필요하다면 리턴하거나 저장)

		// 행성의 최종 위치 수정 (무게중심 기준)
		worldPos = (physPos + Re) * scaleUnits;
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

	// 텍스처 -------------------------------------------------------
	unsigned int texSun = loadTexture("textures/2k_sun.jpg");
	// 행성
	unsigned int texMercury = loadTexture("textures/2k_mercury.jpg");
	unsigned int texVenus = loadTexture("textures/2k_venus_surface.jpg");
	unsigned int texEarth = loadTexture("textures/2k_earth_daymap.jpg");
	unsigned int texMars = loadTexture("textures/2k_mars.jpg");
	unsigned int texJupiter = loadTexture("textures/2k_jupiter.jpg");
	unsigned int texSaturn = loadTexture("textures/2k_saturn.jpg");
	unsigned int texUranus = loadTexture("textures/2k_uranus.jpg");
	unsigned int texNeptune = loadTexture("textures/2k_neptune.jpg");
	//위성
	unsigned int texMoon = loadTexture("textures/2k_moon.jpg");
	unsigned int skyTex = loadTexture("textures/2k_skybox.jpg");

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

		glm::mat4 sunModel(1.0f);
		sunModel = glm::scale(sunModel, glm::vec3(7.0f)); // 태양 크기
		sceneShader.setMat4("model", sunModel);
		glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

		// 1-2. 모든 행성 순회 및 그리기 -----------------------------
		// 태양이 관리하는 행성 리스트 가져오기
		auto& planets = sun.getPlanets();

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

			// C. 행성 렌더링
			renderPlanet(planet, currentTex, sceneShader, sphereIndexCount, dt, SCALE_UNITS, planetWorldPos);

			// D. 위성(달) 처리
			if (!planet.satellites().empty())
			{
				// 첫 번째 위성만 처리 (달)
				Satellite& moon = planet.satellites()[0];

				// 달의 월드 위치 계산 (행성 위치 + 상대 위치)
				glm::vec3 rel = moon.positionRelativeToPlanet(simYears);
				glm::vec3 moonWorldPos = planetWorldPos + (rel * SCALE_UNITS);

				// 달 렌더링 (텍스처: texMoon 고정)
				// 달은 스스로 빛나지 않으므로 isSun=0, emission=1.0(기본 밝기) 설정은 renderPlanet 내부 혹은 drawAtWorld 확인 필요
				// 여기서는 drawAtWorld가 텍스처 바인딩을 안 한다면 미리 해야 함.
				// 보통 drawAtWorld 내부구조에 따라 다르지만, 안전하게 여기서 바인딩:
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texMoon);
				sceneShader.setInt("diffuseMap", 0);
				sceneShader.setInt("isSun", 0);
				sceneShader.setFloat("emissionStrength", 1.0f);
				glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

				moon.drawAtWorld(sceneShader, dt, SCALE_UNITS, moonWorldPos, sphereVAO, sphereIndexCount);
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

		// 다시 행성 리스트를 순회하며 궤적 그리기
		for (auto& planet : planets)
		{
			// 행성 궤적 그리기
			planet.drawTrail(lineShader, view, proj);

			// 위성 궤적 그리기
			if (!planet.satellites().empty())
			{
				Satellite& moon = planet.satellites()[0];

				// 위성 궤적은 "행성 중심"을 기준으로 그려지므로
				// 현재 프레임의 행성 위치(World Space)로 Model Matrix를 만들어줘야 함
				glm::vec3 currentPlanetPos = planet.positionAroundSun(simYears) * SCALE_UNITS;
				glm::mat4 moonTrailModel = glm::translate(glm::mat4(1.0f), currentPlanetPos);

				moon.drawTrail(lineShader, view, proj, moonTrailModel);
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
