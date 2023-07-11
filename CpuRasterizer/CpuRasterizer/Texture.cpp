#include "Texture.h"

using namespace DirectX::SimpleMath;

Vector3 Texture::getWrapped(int i, int j)
{
	i %= Width;
	j %= Height;

	if (i < 0)
	{
		i += Width;
	}
	if (j < 0)
	{
		j += Height;
	}

	const float r = Image[(i + Width * j) * Channels + 0] / 255.0f;
	const float g = Image[(i + Width * j) * Channels + 1] / 255.0f;
	const float b = Image[(i + Width * j) * Channels + 2] / 255.0f;

	return Vector3(r, g, b);
}

Vector3 Texture::bilinearInterpolation(const float dx, const float dy, const Vector3& c00, const Vector3& c10, const Vector3& c01, const Vector3& c11)
{
	Vector3 a = c00 * (1.0f - dx) + c10 * dx;
	Vector3 b = c01 * (1.0f - dx) + c11 * dx;
	return a * (1.0f - dy) + b * dy;
}

Vector3 Texture::SampleLinear(const Vector2& uv)
{
	if (Image.empty())
	{
		return Vector3::Zero;
	}

	// 텍스처 uv [0.0, 1.0] x [0.0, 1.0]
	// 이미지 xy [-0.5, Width + 0.5] x [-0.5, Height + 0.5]
	const Vector2 xy = uv * Vector2(static_cast<float>(Width), static_cast<float>(Height)) - Vector2(0.5f);
	const int i = static_cast<int>(std::floor(xy.x));
	const int j = static_cast<int>(std::floor(xy.y));
	const float dx = xy.x - static_cast<float>(i);
	const float dy = xy.y - static_cast<float>(j);

	return bilinearInterpolation(dx, dy, getWrapped(i, j), getWrapped(i + 1, j), getWrapped(i, j + 1), getWrapped(i + 1, j + 1));
}
