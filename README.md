# ProjectCpuRender

![CPU 렌더러](https://github.com/cakememakeme/ProjectCpuRender/assets/73391410/52c763f8-e754-4c39-bde1-a86f3487d3c7)

개괄적인 클래스 구성도는 uml.jpg에 포함되어 있습니다

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
