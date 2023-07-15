#pragma once

#include <memory>
#include <list>

#include <directxtk/SimpleMath.h>

class CpuRenderPipeline;

struct Triangle
{
    // 클리핑 용도로 만들어진 간단한 폴리곤 구조체
    DirectX::SimpleMath::Vector3 v0;
    DirectX::SimpleMath::Vector3 v1;
    DirectX::SimpleMath::Vector3 v2;
};

enum class EVertexPlace
{
    None = 0,
    Inside,
    Outside,
    Middle
};

class CpuRasterizer
{
public:
    // 삼각형을 하나만 그리는 함수
    // Rasterize!
    static void DrawIndexedTriangle(const size_t startIndex);

private:
    // view space -> clip space 변환
    static DirectX::SimpleMath::Vector3 worldToClip(const DirectX::SimpleMath::Vector3& pointWorld);

    static DirectX::SimpleMath::Vector3 worldToView(const DirectX::SimpleMath::Vector3& pointWorld);

    static DirectX::SimpleMath::Vector3 viewToClip(const DirectX::SimpleMath::Vector3& pointView);

    // clip space -> screen space 변환
    static DirectX::SimpleMath::Vector2 clipToScreen(const DirectX::SimpleMath::Vector3& pointClip);

    static float edgeFunction(const DirectX::SimpleMath::Vector2& v0, const DirectX::SimpleMath::Vector2& v1, const DirectX::SimpleMath::Vector2& point);

    // 평면과 삼각형의 교차
    static bool intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    static bool intersectPlaneAndLine(DirectX::SimpleMath::Vector3& outIntersectPoint, 
        const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& pointA, const DirectX::SimpleMath::Vector3& pointB);

    static void clipTriangle_recursive(std::list<struct Triangle>& triangles);

    static std::list<struct Triangle> splitTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    static EVertexPlace findVertexPlace(const float distance);
};

