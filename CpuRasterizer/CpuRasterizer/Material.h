#pragma once

#include <directxtk/SimpleMath.h>

#include <string>
#include <memory>

class Texture;

class Material
{
public:
    DirectX::SimpleMath::Vector3 Ambient = DirectX::SimpleMath::Vector3(0.1f);
    DirectX::SimpleMath::Vector3 Diffuse = DirectX::SimpleMath::Vector3(1.0f);
    DirectX::SimpleMath::Vector3 Specular = DirectX::SimpleMath::Vector3(1.0f);
    float Shininess = 100.0f;

    std::shared_ptr<Texture> diffuseTex = nullptr;

public:
    bool SetDiffuseTexture(Texture newTex);
};

