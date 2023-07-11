#pragma once

#include <directxtk/SimpleMath.h>

struct Transformation 
{
    DirectX::SimpleMath::Vector3 scale = DirectX::SimpleMath::Vector3(1.0f);
    DirectX::SimpleMath::Vector3 translation = DirectX::SimpleMath::Vector3(0.0f);
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
};

// 실제 씬에 올라갈 수 있는 객체만 해당됩니다
// @todo. 이름 수정 필요 -> ex) RenderObject / PrimitiveObject 등
class Object
{
public:
    virtual ~Object();

    // 모든 버텍스에 공통으로 적용되는 변환(Transformations)
    Transformation Transform;
};

