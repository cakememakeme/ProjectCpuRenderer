#pragma once
#include "Object.h"

class Light : public Object
{
public:
    DirectX::SimpleMath::Vector3 Strength = DirectX::SimpleMath::Vector3(1.0f);
    DirectX::SimpleMath::Vector3 Direction = DirectX::SimpleMath::Vector3(0.0f, -0.5f, 0.5f);
    DirectX::SimpleMath::Vector3 Position = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
    float FallOffStart = 0.0f;
    float FallOffEnd = 1.8f;
    float SpotPower = 0.0f;
};

