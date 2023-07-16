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

enum class EPlaceFromPlane
{
    None = 0,
    Inside, //normal 방향
    Outside,//normal 반대방향
    Middle  //평면에 접함
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

    // 평면과 점의 교차
    static float intersectPlaneAndVertex(const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& point);

    // 평면과 삼각형의 교차
    static EPlaceFromPlane intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    // 평면과 선분의 교차
    static bool intersectPlaneAndLine(DirectX::SimpleMath::Vector3& outIntersectPoint, 
        const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& pointA, const DirectX::SimpleMath::Vector3& pointB);

    static void clipTriangle(std::list<struct Triangle>& triangles);

    static std::list<struct Triangle> splitTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    static EPlaceFromPlane findVertexPlace(const float distance);
};

