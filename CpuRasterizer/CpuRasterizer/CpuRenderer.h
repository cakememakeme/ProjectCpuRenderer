#pragma once

#include "IRenderer.h"

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>

enum class ELightType;
class CpuRenderPipeline;
class Object;
class Mesh;
class Light;

struct D3dVertex 
{
	DirectX::SimpleMath::Vector4 pos;
	DirectX::SimpleMath::Vector2 uv;
};

class CpuRenderer : public IRenderer
{
	int width;
	int height;

private:
	// Cpu Rendering
	std::unique_ptr<CpuRenderPipeline> cpuPipeline;

	std::shared_ptr<std::vector<std::shared_ptr<Object>>> objects;
	
	std::shared_ptr<Mesh> selectedMesh;

	std::shared_ptr<Light> selectedLight;

	ELightType lightType;

	// 윈도우 핸들
	HWND mainWindowHandle;

	// 디바이스
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	
	// ComPtr 쓰면 제대로 렌더링을 못한다. 왜?
	// 뷰포트
	D3D11_VIEWPORT viewport;

	// 메인 렌더타겟
	ID3D11RenderTargetView* renderTargetView;

	// Cpu Rasterization 구현 목적의 2DTexture. 여기에 픽셀을 찍어서 렌더링한다
	ID3D11Texture2D* canvasTexture;
	ID3D11ShaderResourceView* canvasTextureView;

	// 텍스처 설정을 위한 샘플러
	ID3D11SamplerState* colorSampler;

	// 쉐이더
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;

	// 버퍼
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	UINT indexCount;

public:
	CpuRenderer();
	virtual ~CpuRenderer();

	bool Create() override;

	bool Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight) override;

	bool SetObjects(std::shared_ptr<std::vector<std::shared_ptr<Object>>> receivedObjects) override;

	void Update() override;

	void Render() override;

	void OnPostRender() override;

	bool Reset() override;

private:
	bool initDirect3D();
	bool initShaders();
	bool initGui();

	void updateGui();
	bool selectModel();
	std::shared_ptr<std::vector<DirectX::SimpleMath::Vector4>> renderViaCpu();
};

