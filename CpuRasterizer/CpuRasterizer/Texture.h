#pragma once

#include <directxtk/SimpleMath.h>

#include <vector>
#include <string>

using namespace DirectX::SimpleMath;

class Texture
{
public:
	int Width;
	int Height;
	int Channels;
	std::vector<uint8_t> Image;

public:
	Vector3 SampleLinear(const Vector2& uv);

private:
	Vector3 getWrapped(int i, int j);

	Vector3 bilinearInterpolation(const float dx, const float dy, 
		const Vector3& c00, const Vector3& c10, const Vector3& c01, const Vector3& c11);
};

