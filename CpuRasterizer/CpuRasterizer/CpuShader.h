#pragma once

#include <directxtk/SimpleMath.h>

#include "Light.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

using namespace DirectX::SimpleMath;

struct ShaderConstants
{
    DirectX::SimpleMath::Matrix worldMat;
    DirectX::SimpleMath::Matrix worldMatInv;

    Material material;
    Light light;
    int lightType = 0;
};

struct VsInput
{
    // CPU에서 GPU로 보내는 데이터는 적을 수록 좋기 때문에
    // Vector4 대신에 Vector3 사용
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 normal;
    //DirectX::SimpleMath::Vector3 color;
    DirectX::SimpleMath::Vector2 uv;
};

struct VsOutput
{
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 normal;
    //DirectX::SimpleMath::Vector3 color;
    DirectX::SimpleMath::Vector2 uv;
};

struct PsInput
{
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 normal;
    //DirectX::SimpleMath::Vector3 color;
    DirectX::SimpleMath::Vector2 uv;
};

extern ShaderConstants g_constants;
extern std::vector<size_t> g_indexBuffer;
extern Light g_light;
extern std::shared_ptr<std::vector<DirectX::SimpleMath::Vector4>> g_displayBuffer;
extern std::vector<float> g_depthBuffer;
extern std::vector<DirectX::SimpleMath::Vector3> g_vertexBuffer;
extern std::vector<DirectX::SimpleMath::Vector3> g_normalBuffer;
extern std::vector<DirectX::SimpleMath::Vector3> g_colorBuffer;
extern std::vector<DirectX::SimpleMath::Vector2> g_uvBuffer;
// Clockwise가 앞면
extern bool g_cullBackface;
// 정투영(ortho) vs 원근(perspective)투영
extern bool g_bUsePerspectiveProjection;
// 눈과 화면의 거리 (조절 가능)
extern float g_distEyeToScreen;
// 현재 사용하는 조명 (0: directional, 1: point, 2: spot)
extern int g_lightType;
extern int g_width;
extern int g_height;

static float Saturate(float x)
{
    return std::max(0.0f, std::min(1.0f, x));
}

static float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return Saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

static Vector3 BlinnPhong(Vector3 lightStrength, Vector3 lightVec, Vector3 normal, Vector3 toEye, Material mat, Vector2 uv)
{
    Vector3 halfway = toEye + lightVec;
    halfway.Normalize();
    Vector3 specular = mat.Specular * pow(std::max(halfway.Dot(normal), 0.0f), mat.Shininess);

    return mat.Ambient + (mat.diffuseTex->SampleLinear(uv) + specular) * lightStrength;
}

static Vector3 ComputeDirectionalLight(Light L, Material mat, Vector3 normal, Vector3 toEye, Vector2 uv)
{
    Vector3 lightVec = -L.Direction;

    float ndotl = std::max(lightVec.Dot(normal), 0.0f);
    Vector3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat, uv);
}

static Vector3 ComputePointLight(Light L, Material mat, Vector3 pos, Vector3 normal, Vector3 toEye, Vector2 uv)
{
    Vector3 lightVec = L.Position - pos;

    // 쉐이딩할 지점부터 조명까지의 거리 계산
    float d = lightVec.Length();

    // 너무 멀면 조명이 적용되지 않음
    if (d > L.FallOffEnd)
        return Vector3(0.0f);

    lightVec /= d;

    float ndotl = std::max(lightVec.Dot(normal), 0.0f);
    Vector3 lightStrength = L.Strength * ndotl;

    float att = CalcAttenuation(d, L.FallOffStart, L.FallOffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat, uv);
}

static Vector3 ComputeSpotLight(Light L, Material mat, Vector3 pos, Vector3 normal, Vector3 toEye, Vector2 uv)
{
    Vector3 lightVec = L.Position - pos;

    // 쉐이딩할 지점부터 조명까지의 거리 계산
    float d = lightVec.Length();

    // 너무 멀면 조명이 적용되지 않음
    if (d > L.FallOffEnd)
        return Vector3(0.0f);

    lightVec /= d;

    float ndotl = std::max(lightVec.Dot(normal), 0.0f);
    Vector3 lightStrength = L.Strength * ndotl;

    float att = CalcAttenuation(d, L.FallOffStart, L.FallOffEnd);
    lightStrength *= att;

    float spotFactor = pow(std::max(-lightVec.Dot(L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat, uv);
}

static VsOutput CpuVertexShader(VsInput vsInput)
{
    VsOutput vsOutput;

    // 마지막에 1.0f 추가
    Vector4 point = Vector4(vsInput.Position.x, vsInput.Position.y,
        vsInput.Position.z, 1.0f);

    Vector4 worldMat = Vector4::Transform(point, g_constants.worldMat);
    vsOutput.Position = Vector3(worldMat.x, worldMat.y, worldMat.z);

    // 마지막에 0.0f 추가
    Vector4 normal = Vector4(vsInput.normal.x, vsInput.normal.y, vsInput.normal.z, 0.0f);
    normal = Vector4::Transform(normal, g_constants.worldMatInv);
    normal.Normalize();
    vsOutput.normal = Vector3(normal.x, normal.y, normal.z);

    return vsOutput;
}

static Vector4 CpuPixelShader(const PsInput psInput) 
{
    Vector3 eye = Vector3(0.0f, 0.0f, -1.0f); // -distEyeToScreen
    Vector3 toEye = eye - psInput.Position;
    toEye.Normalize();

    Vector3 color;

    if (g_constants.lightType == 0) 
    {
        color = ComputeDirectionalLight(g_constants.light, g_constants.material, psInput.normal, toEye, psInput.uv);
    }
    else if (g_constants.lightType == 1) 
    {
        color = ComputePointLight(g_constants.light, g_constants.material, psInput.Position, psInput.normal, toEye, psInput.uv);
    }
    else 
    {
        color = ComputeSpotLight(g_constants.light, g_constants.material, psInput.Position, psInput.normal, toEye, psInput.uv);
    }

    return Vector4(color.x, color.y, color.z, 1.0f);
}