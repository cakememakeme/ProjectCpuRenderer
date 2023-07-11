#pragma once

#include <directxtk/SimpleMath.h>

class Vertex
{
public:
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 TexCoord;
};