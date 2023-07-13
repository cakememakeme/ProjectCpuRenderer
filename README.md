# ProjectCpuRender

![CPU 렌더러](https://github.com/cakememakeme/ProjectCpuRender/assets/73391410/52c763f8-e754-4c39-bde1-a86f3487d3c7)
라이트 컬러를 *직접* 조절했습니다. 기본적인 라이트 구성이며, 위 GUI에 구성되어 있는 기능 이외의 추가적인 구현은 하지 않았습니다

개괄적인 클래스 구성도는 다음과 같습니다
![클래스 다이어그램](https://github.com/cakememakeme/ProjectCpuRender/assets/73391410/ed219aa8-ef3d-436d-a30c-cc0d993fb919)
주요 기능은 CpuRenderPipeline.h/cpp, CpuRasterizer.h/cpp 클래스에서 수행됩니다

필요 프로그램&라이브러리
1. Visual Studio 2022

2. C++17

3. Direct3D 11

4. vcpkg설치
https://vcpkg.io/en/getting-started.html
https://github.com/microsoft/vcpkg

4.1
vcpkg { Assimp, Imgui } 설치
      
4.1.a
vcpkg install assimp:x64-windows

4.1.b

vcpkg install imgui[win32-binding,dx11-binding]:x64-windows

vcpkg integrate install
