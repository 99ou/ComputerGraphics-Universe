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

// 태양계 셋업 -------------------------------------------------------
void setupSolarSystem(Sun& sun)
{
    // 지구
    PlanetParams earthP;
    earthP.name = "Earth";
    earthP.mass = 1.0f;
    earthP.radiusRender = 0.7f;
    earthP.color = glm::vec3(1.0f);

    earthP.orbit.semiMajorAxis = 25.0f;
    earthP.orbit.eccentricity = 0.0167f;
    earthP.orbit.inclinationDeg = 0.0f;
    earthP.orbit.ascNodeDeg = 0.0f;
    earthP.orbit.argPeriDeg = 102.9f;
    earthP.orbit.periodYears = 1.0f;
    earthP.orbit.meanAnomalyAtEpochDeg = 0.0f;

    earthP.spinDegPerSec = 30.0f;
    earthP.texturePath = "textures/earth_day.jpg";

    Planet earth(earthP);

    // 달
    SatelliteParams moonP;
    moonP.name = "Moon";
    moonP.mass = 0.05f;
    moonP.radiusRender = 0.18f;
    moonP.color = glm::vec3(1.0f);

    moonP.orbit.semiMajorAxis = 3.0f;
    moonP.orbit.eccentricity = 0.0549f;
    moonP.orbit.inclinationDeg = 5.145f;
    moonP.orbit.ascNodeDeg = 125.0f;
    moonP.orbit.argPeriDeg = 318.0f;
    moonP.orbit.periodYears = 27.32f / 365.25f;
    moonP.orbit.meanAnomalyAtEpochDeg = 0.0f;

    moonP.spinDegPerSec = 10.0f;
    moonP.trailMaxPoints = 230;
    moonP.texturePath = "textures/moon.jpg";

    Satellite moon(moonP);
    earth.addSatellite(moon);

    sun.addPlanet(earth);
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
    unsigned int texSun = loadTexture("textures/sun.jpg");
    unsigned int texEarth = loadTexture("textures/earth_day.jpg");
    unsigned int texMoon = loadTexture("textures/moon.jpg");
    unsigned int skyTex = loadTexture("textures/skybox.jpg");

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
        // 1) HDR FBO : 태양 / 지구 / 달 만
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

        // 태양 ------------------------------------------------------
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texSun);
        sceneShader.setInt("diffuseMap", 0);
        sceneShader.setInt("isSun", 1);
        sceneShader.setFloat("emissionStrength", 4.0f);

        glm::mat4 sunModel(1.0f);
        sunModel = glm::scale(sunModel, glm::vec3(7.0f));
        sceneShader.setMat4("model", sunModel);
        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

        // 지구 / 달 위치 계산 --------------------------------------
        Planet& earth = sun.getPlanets()[0];
        PlanetParams eP = earth.getParams();
        auto& sats = earth.satellites();

        glm::vec3 earthPhys = earth.positionAroundSun(simYears);
        earth.recordTrail(earthPhys);
        glm::vec3 earthWorld = earthPhys * SCALE_UNITS;
        glm::vec3 earthCM = earthWorld;
        glm::vec3 moonCM(0.0f);
        glm::mat4 moonTrailModel(1.0f);

        if (!sats.empty())
        {
            Satellite& moon = sats[0];
            const SatelliteParams& mP = moon.getParams();
            glm::vec3 rel = moon.positionRelativeToPlanet(simYears);

            float Mp = eP.mass;
            float Ms = mP.mass;
            glm::vec3 Re = -(Ms / (Mp + Ms)) * rel;
            glm::vec3 Rm = Re + rel;

            earthCM = (earthPhys + Re) * SCALE_UNITS;
            moonCM = (earthPhys + Rm) * SCALE_UNITS;

            moon.recordTrail(rel);
            moonTrailModel = glm::translate(glm::mat4(1.0f), earthCM);
        }

        // 지구 ------------------------------------------------------
        earth.advanceSpin(dt);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texEarth);
        sceneShader.setInt("diffuseMap", 0);
        sceneShader.setInt("isSun", 0);
        sceneShader.setFloat("emissionStrength", 1.0f);

        glm::mat4 earthModel = earth.buildModelMatrix(SCALE_UNITS, earthCM);
        sceneShader.setMat4("model", earthModel);
        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

        // 달 --------------------------------------------------------
        if (!sats.empty())
        {
            Satellite& moon = sats[0];
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texMoon);
            sceneShader.setInt("diffuseMap", 0);
            sceneShader.setInt("isSun", 0);
            sceneShader.setFloat("emissionStrength", 1.0f);

            moon.drawAtWorld(sceneShader, dt, SCALE_UNITS, moonCM);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ================================
        // 2) Bloom Blur
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
        // 3) 기본 프레임버퍼: Skybox → Composite
        // ================================
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // (1) Skybox 먼저 (Bloom X)
        glDisable(GL_DEPTH_TEST);
        skyShader.use();
        skyShader.setMat4("view", view);
        skyShader.setMat4("proj", proj);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTex);
        skyShader.setInt("skyTex", 0);

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

        // (2) 그 위에 Bloom 합성 결과를 Additive로 덮기
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
        glBlendFunc(GL_ONE, GL_ONE);   // 검정 배경은 skybox 안 가리고, 밝은 픽셀만 더해짐
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisable(GL_BLEND);

        // ================================
        // 4) 궤도 / 트레일 (Bloom X, Skybox 위)
        // ================================
        glEnable(GL_DEPTH_TEST); // drawTrail 안에서 depth 끄면 그대로 사용

        earth.drawTrail(lineShader, view, proj);
        if (!sats.empty())
        {
            Satellite& moon = sats[0];
            moon.drawTrail(lineShader, view, proj, moonTrailModel);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
