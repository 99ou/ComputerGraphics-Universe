# ComputerGraphics-Universe
안양대학교 3학년 컴퓨터그래픽스 수업 은하계 만들기 과제

---

전체 코드는 OpenGL을 사용하여 태양계를 시뮬레이션하고, HDR(High Dynamic Range)과 Bloom(빛 번짐) 효과를 적용하여 렌더링하는 프로그램입니다.

크게 설정(Setup), 초기화(Init), 렌더링 루프(Loop) 세 부분으로 나뉩니다. 각 메서드와 주요 블록의 역할을 간단히 요약해 드립니다.

1. 콜백 함수 (입력 및 창 관리)
framebuffer_size_callback: 창 크기가 조절될 때 뷰포트(그림 그려지는 영역) 크기를 맞춰줍니다.

mouse_callback: 마우스 움직임을 감지하여 카메라의 시점(Yaw, Pitch)을 회전시킵니다.

scroll_callback: 마우스 휠을 굴려 카메라 줌(Zoom)을 조절합니다.

2. 도형 생성 (Geometry)
createSphere: 행성을 그리기 위한 구(Sphere) 모델의 점(Vertex), 법선(Normal), 텍스처 좌표(UV)를 생성하고 GPU에 올립니다.

createQuad: 화면 전체를 덮는 **사각형(Quad)**을 만듭니다. 나중에 블러(Blur)나 최종 합성을 할 때 도화지 용도로 씁니다.

3. 시뮬레이션 설정
setupSolarSystem: 태양, 지구(Earth), 수성(Mercury), 달(Moon) 객체를 생성합니다. 질량, 크기, 궤도 데이터(공전 속도, 기울기 등)를 입력하여 Sun 객체에 등록합니다.

4. 렌더링 헬퍼
renderPlanet: 행성 하나를 그리는 과정을 함수로 묶은 것입니다.

자전 업데이트 → 쉐이더 값 설정(텍스처, 빛) → 위치 행렬 계산 → 그리기(glDrawElements)를 수행합니다.

5. 메인 함수 (main) - 초기화 및 준비
GLFW/GLEW 초기화: 윈도우 창을 만들고 OpenGL 기능을 활성화합니다.

쉐이더(Shader) 컴파일:

skyShader: 우주 배경(Skybox)을 그립니다.

sceneShader: 행성을 그립니다. (밝은 부분은 따로 추출)

lineShader: 궤도 선(Trail)을 그립니다.

blurShader: 추출된 빛을 흐리게 만듭니다 (Bloom 효과).

finalShader: 원본 장면과 흐린 빛을 합치고 톤매핑(Tone mapping)하여 최종 색상을 만듭니다.

FBO (프레임버퍼) 설정:

hdrFBO: 화면에 바로 그리지 않고, '일반 색상'과 '눈부신 색상'을 따로 저장하는 메모리 공간입니다.

pingFBO: 블러 효과를 주기 위해 이미지를 번갈아 가며 처리할 메모리 공간입니다.

6. 메인 함수 (main) - 무한 루프 (Game Loop)
매 프레임마다 다음 순서로 작동합니다.

입력 및 시간 계산: dt(델타 타임)를 구하고 키보드 입력을 받아 카메라를 이동합니다.

물리 연산: simYears를 증가시키고, 행성과 위성의 새로운 3D 위치를 계산합니다.

Pass 1 (장면 렌더링): 태양, 지구, 수성, 달을 hdrFBO에 그립니다. 이때 밝은 빛(태양 등)은 별도의 텍스처로 빠집니다.

Pass 2 (블러 처리): 밝은 빛만 담긴 텍스처를 가로/세로로 여러 번 문질러서 흐리게 만듭니다 (Bloom).

Pass 3 (최종 합성):

검은 화면에 우주 배경(skyShader)을 먼저 그립니다.

그 위에 아까 그린 행성들과 블러 처리된 빛을 합쳐서(finalShader) 그립니다.

Pass 4 (궤적 그리기): 행성이 지나간 길(Trail)을 선으로 그립니다.

화면 갱신: glfwSwapBuffers로 완성된 그림을 모니터에 보여줍니다.
