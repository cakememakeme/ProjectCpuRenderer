#include "CpuRasterizer.h"

#include <algorithm>

#include "CpuShader.h"

// GPU에서 내부적으로 사용하는 메모리라고 생각합시다.

// 쉐이더 상수
ShaderConstants g_constants;
std::vector<size_t> g_indexBuffer;
Light g_light;
std::shared_ptr<std::vector<DirectX::SimpleMath::Vector4>> g_displayBuffer;
std::vector<float> g_depthBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_vertexBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_normalBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_colorBuffer;
std::vector<DirectX::SimpleMath::Vector2> g_uvBuffer;
// Clockwise가 앞면
bool g_cullBackface;
// 정투영(ortho) vs 원근(perspective)투영
bool g_bUsePerspectiveProjection;
// 눈과 화면의 거리 (조절 가능)
float g_distEyeToScreen;
// 현재 사용하는 조명 (0: directional, 1: point, 2: spot)
int g_lightType;
int g_width;
int g_height;

// ~GPU에서 내부적으로 사용하는 메모리라고 생각합시다.

using namespace DirectX::SimpleMath;
using namespace std;

shared_ptr<CpuRenderPipeline> CpuRasterizer::gpu = nullptr;

void CpuRasterizer::BindSharedMemory(shared_ptr<CpuRenderPipeline> renderPipeline)
{
    CpuRasterizer::gpu = renderPipeline;
}

void CpuRasterizer::DrawIndexedTriangle(const size_t& startIndex)
{
    const size_t i0 = g_indexBuffer[startIndex];
    const size_t i1 = g_indexBuffer[startIndex + 1];
    const size_t i2 = g_indexBuffer[startIndex + 2];
    
    const Vector2 v0 = ProjectWorldToRaster(g_vertexBuffer[i0]);
    const Vector2 v1 = ProjectWorldToRaster(g_vertexBuffer[i1]);
    const Vector2 v2 = ProjectWorldToRaster(g_vertexBuffer[i2]);

    // 삼각형 전체 넓이의 두 배, 음수일 수도 있음
    const float area = EdgeFunction(v0, v1, v2);

    // 뒷면일 경우 그리지 않음
    if (g_cullBackface && area < 0.0f)
    {
        return;
    }

    /*const auto& c0 = g_colorBuffer[i0];
    const auto& c1 = g_colorBuffer[i1];
    const auto& c2 = g_colorBuffer[i2];*/

    const auto& uv0 = g_uvBuffer[i0];
    const auto& uv1 = g_uvBuffer[i1];
    const auto& uv2 = g_uvBuffer[i2];

    const Vector2 bMin = Vector2::Min(Vector2::Min(v0, v1), v2);
    const Vector2 bMax = Vector2::Max(Vector2::Max(v0, v1), v2);

    const auto xMin = size_t(std::clamp(std::floor(bMin.x), 0.0f, float(g_width - 1)));
    const auto yMin = size_t(std::clamp(std::floor(bMin.y), 0.0f, float(g_height - 1)));
    const auto xMax = size_t(std::clamp(std::ceil(bMax.x), 0.0f, float(g_width - 1)));
    const auto yMax = size_t(std::clamp(std::ceil(bMax.y), 0.0f, float(g_height - 1)));

    // GPU 안에서는 멀티쓰레딩으로 여러 픽셀들을 한꺼번에 처리합니다.
    // 엄밀히 얘기하면 픽셀이 아니라 될 수 있는 "후보"들이기 때문에
    // Fragment라는 다른 용어를 사용하기도 합니다.
    // OpenGL, Vulkan에서는 Fragment Shader, DX에서는 Pixel Shader
    for (size_t j = yMin; j <= yMax; j++) {
        for (size_t i = xMin; i <= xMax; i++) {

            const Vector2 point = Vector2(float(i), float(j));

            // 위에서 계산한 삼각형 전체 넓이 area를 재사용
            // area가 음수라면 alpha0, alpha1, alpha2 모두 음수여야
            // 삼각형 안에 포함되는 픽셀로 판단할 수 있습니다.
            float w0 = EdgeFunction(v1, v2, point) / area;
            float w1 = EdgeFunction(v2, v0, point) / area;
            float w2 = EdgeFunction(v0, v1, point) / area;

            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {

                // Perspective-Correct Interpolation
                // OpenGL 구현
                // https://stackoverflow.com/questions/24441631/how-exactly-does-opengl-do-perspectively-correct-linear-interpolation

                const float z0 = g_vertexBuffer[i0].z + g_distEyeToScreen;
                const float z1 = g_vertexBuffer[i1].z + g_distEyeToScreen;
                const float z2 = g_vertexBuffer[i2].z + g_distEyeToScreen;

                const Vector3 p0 = g_vertexBuffer[i0];
                const Vector3 p1 = g_vertexBuffer[i1];
                const Vector3 p2 = g_vertexBuffer[i2];

                // 뒷면일 경우에도 쉐이딩이 가능하도록 normal을 반대로
                // 뒤집어줍니다.
                const Vector3 n0 = area < 0.0f ? -g_normalBuffer[i0] : g_normalBuffer[i0];
                const Vector3 n1 = area < 0.0f ? -g_normalBuffer[i1] : g_normalBuffer[i1];
                const Vector3 n2 = area < 0.0f ? -g_normalBuffer[i2] : g_normalBuffer[i2];

                if (g_bUsePerspectiveProjection)
                {
                    w0 /= z0;
                    w1 /= z1;
                    w2 /= z2;

                    const float wSum = w0 + w1 + w2;

                    w0 /= wSum;
                    w1 /= wSum;
                    w2 /= wSum;
                }

                // 이하 동일
                const float depth = w0 * z0 + w1 * z1 + w2 * z2;
                // const Vector3 color = w0 * c0 + w1 * c1 + w2 * c2;
                const Vector2 uv = w0 * uv0 + w1 * uv1 + w2 * uv2;

                if (depth < g_depthBuffer[i + g_width * j])
                {
                    g_depthBuffer[i + g_width * j] = depth;

                    PsInput psInput;
                    psInput.Position = w0 * p0 + w1 * p1 + w2 * p2;
                    psInput.normal = w0 * n0 + w1 * n1 + w2 * n2;
                    // psInput.color = color;
                    psInput.uv = uv;

                    vector<Vector4>& buffer = *g_displayBuffer.get();
                    buffer[i + g_width * j] = CpuPixelShader(psInput);
                }
            }
        }
    }
}

Vector2 CpuRasterizer::ProjectWorldToRaster(Vector3 pointWorld)
{
    // 월드 좌표계의 원점이 우리가 보는 화면의 중심이라고 가정

    // 정투영(Orthographic projection)
    Vector2 pointProj = Vector2(pointWorld.x, pointWorld.y);

    // 원근투영(Perspective projection)
    // 원근투영도 행렬로 표현할 수 있습니다.
    if (g_bUsePerspectiveProjection)
    {
        const float scale = g_distEyeToScreen / (g_distEyeToScreen + pointWorld.z);
        pointProj = Vector2(pointWorld.x * scale, pointWorld.y * scale);
    }

    const float aspect = static_cast<float>(g_width) / g_height;
    const Vector2 pointNDC = Vector2(pointProj.x / aspect, pointProj.y);

    // 레스터 좌표의 범위 [-0.5, width - 1 + 0.5] x [-0.5, height - 1 + 0.5]
    const float xScale = 2.0f / g_width;
    const float yScale = 2.0f / g_height;

    // NDC -> 레스터 화면 좌표계
    // 주의: y좌표 상하반전
    return Vector2((pointNDC.x + 1.0f) / xScale - 0.5f, (1.0f - pointNDC.y) / yScale - 0.5f);
}

float CpuRasterizer::EdgeFunction(const Vector2& v0, const Vector2& v1, const Vector2& point)
{
    const Vector2 a = v1 - v0;
    const Vector2 b = point - v0;
    return a.x * b.y - a.y * b.x;
}
