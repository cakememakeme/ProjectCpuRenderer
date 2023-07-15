#include "CpuRasterizer.h"

#include <algorithm>
#include <array>

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

void CpuRasterizer::DrawIndexedTriangle(const size_t startIndex)
{
    // backface culling
    const size_t i0 = g_indexBuffer[startIndex];
    const size_t i1 = g_indexBuffer[startIndex + 1];
    const size_t i2 = g_indexBuffer[startIndex + 2];

    const Vector3 rootV0 = worldToClip(g_vertexBuffer[i0]);
    const Vector3 rootV1 = worldToClip(g_vertexBuffer[i1]);
    const Vector3 rootV2 = worldToClip(g_vertexBuffer[i2]);

    const Vector2 rootV0_screen = clipToScreen(rootV0);
    const Vector2 rootV1_screen = clipToScreen(rootV1);
    const Vector2 rootV2_screen = clipToScreen(rootV2);

    // 삼각형 전체 넓이의 두 배, 음수일 수도 있음
    const float area = edgeFunction(rootV0_screen, rootV1_screen, rootV2_screen);

    // 뒷면일 경우
    if (g_cullBackface && area < 0.0f)
    {
        return;
    }

    // clipping
    
    // stl 리스트는 노드구조라서 마음에 들지 않지만
    // sparse array를 쓰자니 이건 만들어야 할 것 같아서 그냥 리스트를 사용했다
    std::list<struct Triangle> triangles;
    triangles.push_back({ rootV0, rootV1, rootV2 });
    //clipTriangle_recursive(triangles);

    /*const auto& c0 = g_colorBuffer[i0];
    const auto& c1 = g_colorBuffer[i1];
    const auto& c2 = g_colorBuffer[i2];*/

    const auto& uv0 = g_uvBuffer[i0];
    const auto& uv1 = g_uvBuffer[i1];
    const auto& uv2 = g_uvBuffer[i2];

    // draw internal
    for (const auto& triangle : triangles)
    {
        const Vector2 clipV0_Screen = clipToScreen(triangle.v0);
        const Vector2 clipV1_Screen = clipToScreen(triangle.v1);
        const Vector2 clipV2_Screen = clipToScreen(triangle.v2);

        const Vector2 bMin = Vector2::Min(Vector2::Min(clipV0_Screen, clipV1_Screen), clipV2_Screen);
        const Vector2 bMax = Vector2::Max(Vector2::Max(clipV0_Screen, clipV1_Screen), clipV2_Screen);

        const auto xMin = size_t(std::clamp(std::floor(bMin.x), 0.0f, float(g_width - 1)));
        const auto yMin = size_t(std::clamp(std::floor(bMin.y), 0.0f, float(g_height - 1)));
        const auto xMax = size_t(std::clamp(std::ceil(bMax.x), 0.0f, float(g_width - 1)));
        const auto yMax = size_t(std::clamp(std::ceil(bMax.y), 0.0f, float(g_height - 1)));

        // Primitive 보간 후 Pixel(Fragment) Shader로 넘긴다
        for (size_t y = yMin; y <= yMax; y++) 
        {
            for (size_t x = xMin; x <= xMax; x++) 
            {
                const Vector2 point = Vector2(float(x), float(y));

                // 위에서 계산한 삼각형 전체 넓이 area를 재사용
                float w0 = edgeFunction(rootV1_screen, rootV2_screen, point) / area;
                float w1 = edgeFunction(rootV2_screen, rootV0_screen, point) / area;
                float w2 = edgeFunction(rootV0_screen, rootV1_screen, point) / area;

                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) 
                {
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

                    const float depth = w0 * z0 + w1 * z1 + w2 * z2;
                    // const Vector3 color = w0 * c0 + w1 * c1 + w2 * c2;
                    const Vector2 uv = w0 * uv0 + w1 * uv1 + w2 * uv2;

                    if (depth < g_depthBuffer[x + g_width * y])
                    {
                        g_depthBuffer[x + g_width * y] = depth;

                        PsInput psInput;
                        psInput.Position = w0 * p0 + w1 * p1 + w2 * p2;
                        psInput.normal = w0 * n0 + w1 * n1 + w2 * n2;
                        // psInput.color = color;
                        psInput.uv = uv;

                        vector<Vector4>& buffer = *g_displayBuffer.get();
                        buffer[x + g_width * y] = CpuPixelShader(psInput);
                    }
                }
            }
        }
    }
}

Vector3 CpuRasterizer::worldToClip(const Vector3& pointWorld)
{
    // @todo. 카메라 기능 개선이 들어가면 VertexShader로 이동
    Vector3 pointView = worldToView(pointWorld);
    Vector3 pointClip = viewToClip(pointView);
    return pointClip;
}

DirectX::SimpleMath::Vector2 CpuRasterizer::clipToScreen(const DirectX::SimpleMath::Vector3& pointClip)
{
    // 레스터 좌표의 범위 [-0.5, width - 1 + 0.5] x [-0.5, height - 1 + 0.5]
    const float xScale = 2.0f / g_width;
    const float yScale = 2.0f / g_height;

    // NDC -> 레스터 화면 좌표계(screen space)
    // 주의: y좌표 상하반전
    return Vector2((pointClip.x + 1.0f) / xScale - 0.5f, (1.0f - pointClip.y) / yScale - 0.5f);
}

float CpuRasterizer::edgeFunction(const Vector2& v0, const Vector2& v1, const Vector2& point)
{
    const Vector2 a = v1 - v0;
    const Vector2 b = point - v0;
    return a.x * b.y - a.y * b.x;
}

bool CpuRasterizer::intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle)
{
    const struct Triangle& tri = triangle;

    // 용책 64p, 평면 구축 참고
    const float i0 = plane.x * tri.v0.x + plane.y * tri.v0.y + plane.z * tri.v0.z + plane.w;
    const float i1 = plane.x * tri.v0.x + plane.y * tri.v0.y + plane.z * tri.v0.z + plane.w;
    const float i2 = plane.x * tri.v0.x + plane.y * tri.v0.y + plane.z * tri.v0.z + plane.w;

    // inside
    if (i0 >= 0.0f && i1 >= 0.0f && i0 >= 0.0f)
    {
        return false;
    }
    // outside
    if (i0 < 0.0f && i1 < 0.0f && i0 < 0.0f)
    {
        return false;
    }

    // split
    return true;
}

bool CpuRasterizer::intersectPlaneAndLine(Vector3& outIntersectPoint, 
    const Vector4& plane, const Vector3& pointA, const Vector3& pointB)
{
    const Vector3 n(plane.x, plane.y, plane.z);
    const Vector3 t(pointB - pointA);
    const float dist = n.Dot(t);
    const EVertexPlace place = findVertexPlace(dist);
    if (place == EVertexPlace::Middle)
    {
        // 두 벡터가 수직하는 경우 (dot(n, t) == 0)
        return false;
    }
    if (std::fabs(dist) < std::numeric_limits<float>::epsilon())
    {
        // 미세하게 split
        return false;
    }

    const float aDot = pointA.Dot(n);
    const float bDot = pointB.Dot(n);
    const float scale = -(plane.w) - aDot / (bDot - aDot);

    if (scale < 0.0f)
    {
        return false;
    }
    if (scale > 1.0f)
    {
        return false;
    }

    outIntersectPoint = pointA + (scale * (pointB - pointA));
    return true;
}

void CpuRasterizer::clipTriangle_recursive(std::list<struct Triangle>& triangles)
{
    for (auto triangle = triangles.begin(); triangle != triangles.end(); ++triangle)
    {
        const struct Triangle& tri = *triangle;

        // far plane 제외하고 클리핑을 수행
        const Vector4 leftPlane = Vector4(-1.0f, 0.0f, 0.0f, 1.0f);
        if (intersectPlaneAndTriangle(leftPlane, tri))
        {
            triangles.splice(triangles.end(), splitTriangle(leftPlane, tri));
            triangles.erase(triangle);
            clipTriangle_recursive(triangles);
            return;
        }
        const Vector4 rightPlane = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
        if (intersectPlaneAndTriangle(rightPlane, tri))
        {
            triangles.splice(triangles.end(), splitTriangle(rightPlane, tri));
            triangles.erase(triangle);
            clipTriangle_recursive(triangles);
            return;
        }
        const Vector4 topPlane = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
        if (intersectPlaneAndTriangle(topPlane, tri))
        {
            triangles.splice(triangles.end(), splitTriangle(topPlane, tri));
            triangles.erase(triangle);
            clipTriangle_recursive(triangles);
            return;
        }
        const Vector4 downPlane = Vector4(0.0f, -1.0f, 0.0f, 1.0f);
        if (intersectPlaneAndTriangle(downPlane, tri))
        {
            triangles.splice(triangles.end(), splitTriangle(downPlane, tri));
            triangles.erase(triangle);
            clipTriangle_recursive(triangles);
            return;
        }
        const Vector4 nearPlane = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
        if (intersectPlaneAndTriangle(nearPlane, tri))
        {
            triangles.splice(triangles.end(), splitTriangle(nearPlane, tri));
            triangles.erase(triangle);
            clipTriangle_recursive(triangles);
            return;
        }
    }
}

list<struct Triangle> CpuRasterizer::splitTriangle(const DirectX::SimpleMath::Vector4& plane, const Triangle& triangle)
{
    const struct Triangle& tri = triangle;
    std::array<Vector3, 3> tris = { tri.v0, tri.v1, tri.v2 };
    list<Vector3> inside;
    list<Vector3> outside;

    for(size_t i = 1; i <= tris.size(); ++i)
    {
        const size_t curVert = i % 3;
        const Vector3& pointA = tris[i - 1];
        const Vector3& pointB = tris[curVert];

        const float distB = plane.x * pointB.x + plane.y * pointB.y + plane.z * pointB.z + plane.w;
        const EVertexPlace placeB = findVertexPlace(distB);
        if (placeB == EVertexPlace::Middle)
        {
            outside.push_back(pointB);
            inside.push_back(pointB);
        }
        else
        {
            Vector3 intersectPoint = Vector3::Zero;
            if (intersectPlaneAndLine(intersectPoint, plane, pointA, pointB))
            {
                //if(placeB == EVertexPlace::)
            }
        }

    }

    const float i1 = plane.x * tri.v0.x + plane.y * tri.v0.y + plane.z * tri.v0.z + plane.w;
    const float i2 = plane.x * tri.v0.x + plane.y * tri.v0.y + plane.z * tri.v0.z + plane.w;

    const EVertexPlace v1Loc = findVertexPlace(i1);
    const EVertexPlace v2Loc = findVertexPlace(i2);

    for (int i = 1; i <= 3; ++i)
    {

    }
    return list<struct Triangle>();
}

EVertexPlace CpuRasterizer::findVertexPlace(const float distance)
{
    if (std::fabs(distance) < std::numeric_limits<float>::epsilon())
    {
        return EVertexPlace::Middle;
    }
    else if (distance < 0.0f)
    {
        return EVertexPlace::Outside;
    }
    
    return EVertexPlace::Inside;
}

Vector3 CpuRasterizer::worldToView(const Vector3& pointWorld)
{
    // 월드 좌표계의 원점이 우리가 보는 화면의 중심이라고 가정(world->view transformation 생략, 추후 개선 예정)
    return pointWorld;
}

DirectX::SimpleMath::Vector3 CpuRasterizer::viewToClip(const Vector3& pointView)
{
    // 정투영(Orthographic projection)
    Vector3 pointProj = Vector3(pointView.x, pointView.y, 1.0f);

    // 원근투영(Perspective projection)
    // 원근투영도 행렬로 표현할 수 있습니다.
    if (g_bUsePerspectiveProjection)
    {
        const float scale = g_distEyeToScreen / (g_distEyeToScreen + pointView.z);
        pointProj = Vector3(pointView.x * scale, pointView.y * scale, 1.0f);
    }

    const float aspect = static_cast<float>(g_width) / g_height;
    const Vector3 pointNDC = Vector3(pointProj.x / aspect, pointProj.y, 1.0f);

    return pointNDC;
}
