#include "CpuRenderer.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <iostream>

#include "CpuRenderPipeline.h"
#include "Mesh.h"
#include "Light.h"

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;
using namespace std;

CpuRenderer::CpuRenderer()
{
	Create();
}

CpuRenderer::~CpuRenderer()
{
	Reset();
}

bool CpuRenderer::Create()
{
	cpuPipeline = make_unique<CpuRenderPipeline>();
	if (!cpuPipeline)
	{
		return false;
	}

	return true;
}

bool CpuRenderer::Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight)
{
	mainWindowHandle = mainWindow;
	width = bufferWidth;
	height = bufferHeight;

	if (!initDirect3D())
	{
		return false;
	}

	if (!cpuPipeline->Initialize(bufferWidth, bufferHeight))
	{
		return false;
	}

	if (!initGui())
	{
		return false;
	}

	return true;
}

bool CpuRenderer::SetObjects(std::shared_ptr<std::vector<std::shared_ptr<Object>>> receivedObjects)
{
	objects = receivedObjects;
	if (objects)
	{
		return true;
	}
	
	return false;
}

void CpuRenderer::Update()
{
	if (!selectModel())
	{
		//std::cout << "no model selecedted." << std::endl;
	}

	updateGui();

	cpuPipeline->SetObjects(objects);
	cpuPipeline->SetLightType(lightType);
}

void CpuRenderer::Render()
{
	// REAL rendering

	// copy to resource
	{
		shared_ptr<vector<Vector4>> displayBuffer = renderViaCpu(); // CPU 렌더링
		if (!displayBuffer)
		{
			std::cout << "renderViaCpu() failed." << std::endl;
			displayBuffer = make_shared<vector<Vector4>>();
			std::fill(displayBuffer->begin(), displayBuffer->end(), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		D3D11_MAPPED_SUBRESOURCE subresource = {};
		context->Map(canvasTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
		memcpy(subresource.pData, displayBuffer->data(), displayBuffer->size() * sizeof(Vector4));
		context->Unmap(canvasTexture, 0);
	}

	// 디바이스에 렌더링
	{
		float clearColor[4] = { 0.0f, 0.2f, 0.0f, 1.0f };
		context->RSSetViewports(1, &viewport);
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		context->ClearRenderTargetView(renderTargetView, clearColor);

		context->VSSetShader(vertexShader, 0, 0);
		context->PSSetShader(pixelShader, 0, 0);

		UINT stride = sizeof(D3dVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	
		context->PSSetSamplers(0, 1, &colorSampler);
		context->PSSetShaderResources(0, 1, &canvasTextureView);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->DrawIndexed(indexCount, 0, 0);
	}
}

void CpuRenderer::OnPostRender()
{
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	swapChain->Present(1, 0);
}

bool CpuRenderer::Reset()
{
	if (colorSampler)
	{
		colorSampler->Release();
		colorSampler = nullptr;
	}
	if (indexBuffer)
	{
		indexBuffer->Release();
		indexBuffer = nullptr;
	}
	if (vertexBuffer)
	{
		vertexBuffer->Release();
		vertexBuffer = nullptr;
	}
	if (layout)
	{
		layout->Release();
		layout = nullptr;
	}
	if (pixelShader)
	{
		pixelShader->Release();
		pixelShader = nullptr;
	}
	if (vertexShader)
	{
		vertexShader->Release();
		vertexShader = nullptr;
	}
	if (canvasTextureView)
	{
		canvasTextureView->Release();
		canvasTextureView = nullptr;
	}
	if (canvasTexture)
	{
		canvasTexture->Release();
		canvasTexture = nullptr;
	}
	if (renderTargetView)
	{
		renderTargetView->Release();
		renderTargetView = nullptr;
	}

	DestroyWindow(mainWindowHandle);
	return false;
}

bool CpuRenderer::initDirect3D()
{
	//https://learn.microsoft.com/ko-kr/windows/uwp/gaming/simple-port-from-direct3d-9-to-11-1-part-1--initializing-direct3d

	// device, context 생성
	const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
	UINT createDeviceFlags = 0;
	const D3D_FEATURE_LEVEL featureLevels[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_9_3 };
	D3D_FEATURE_LEVEL featureLevel;
	{
		if (FAILED(D3D11CreateDevice(nullptr,
			driverType,
			0,
			createDeviceFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&device,
			&featureLevel,
			&context)))
		{
			std::cout << "D3D11CreateDevice() failed." << std::endl;
			return false;
		}
		if (featureLevel != D3D_FEATURE_LEVEL_11_0)
		{
			std::cout << "D3D FEATURE LEVEL 11 0 unsupported." << std::endl;
			return false;
		}
	}

	// 스왑체인
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	{
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.OutputWindow = mainWindowHandle;
		swapChainDesc.Windowed = true;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr,
			driverType,
			nullptr,
			createDeviceFlags,
			featureLevels,
			1,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			&swapChain,
			&device,
			&featureLevel,
			&context)))
		{
			std::cout << "D3D11CreateDeviceAndSwapChain() failed." << std::endl;
			return false;
		}
	}

	// 백버퍼
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		if (FAILED(device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView)))
		{
			std::cout << "renderTargetView CreateRenderTargetView() failed." << std::endl;
			return false;
		}
	}

	// 뷰포트
	{
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		context->RSSetViewports(1, &viewport);
	}

	if (!initShaders())
	{
		std::cout << "initShaders() failed." << std::endl;
		return false;
	}

	// 텍스처 샘플러 설정
	D3D11_SAMPLER_DESC sampDesc = {};
	{
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // D3D11_FILTER_MIN_MAG_MIP_LINEAR
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(device->CreateSamplerState(&sampDesc, &colorSampler)))
		{
			std::cout << "CreateSamplerState() failed." << std::endl;
			return false;
		}
	}

	// plane을 화면에 깔고, 거기에 CpuRenderPipeline에서 처리한 fragment를 plane에 그린다
	// plane 메시데이터 설정
	{
		const std::vector<D3dVertex> vertices = {
			{
				{-1.0f, -1.0f, 0.0f, 1.0f},
				{0.f, 1.f},
			},
			{
				{1.0f, -1.0f, 0.0f, 1.0f},
				{1.f, 1.f},
			},
			{
				{1.0f, 1.0f, 0.0f, 1.0f},
				{1.f, 0.f},
			},
			{
				{-1.0f, 1.0f, 0.0f, 1.0f},
				{0.f, 0.f},
			},
		};

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // write access access by
		// CPU and GPU
		bufferDesc.ByteWidth = UINT(
			sizeof(D3dVertex) * vertices.size()); // size is the VERTEX struct * 3
		bufferDesc.BindFlags =
			D3D11_BIND_VERTEX_BUFFER; // use as a vertex buffer
		bufferDesc.CPUAccessFlags =
			D3D11_CPU_ACCESS_WRITE; // allow CPU to write
		// in buffer
		bufferDesc.StructureByteStride = sizeof(D3dVertex);

		D3D11_SUBRESOURCE_DATA vertexBufferData = {
			0,
		};
		vertexBufferData.pSysMem = vertices.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		const HRESULT hr =
			device->CreateBuffer(&bufferDesc, &vertexBufferData, &vertexBuffer);
		if (FAILED(hr)) {
			std::cout << "CreateBuffer() failed. " << std::hex << hr
				<< std::endl;
		};
	}

	// 인덱스 설정
	{
		const std::vector<uint16_t> indices = {
			3, 1, 0, 2, 1, 3,
		};

		indexCount = UINT(indices.size());

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // write access access by
		// CPU and GPU
		bufferDesc.ByteWidth = UINT(sizeof(uint16_t) * indices.size());
		bufferDesc.BindFlags =
			D3D11_BIND_INDEX_BUFFER; // use as a vertex buffer
		bufferDesc.CPUAccessFlags =
			D3D11_CPU_ACCESS_WRITE; // allow CPU to write
		// in buffer
		bufferDesc.StructureByteStride = sizeof(uint16_t);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = indices.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;

		device->CreateBuffer(&bufferDesc, &indexBufferData, &indexBuffer);
	}

	// 텍스처 설정
	D3D11_TEXTURE2D_DESC textureDesc = {};
	{
		textureDesc.ArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags = 0;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.Width = width;
		textureDesc.Height = height;

		if (FAILED(device->CreateTexture2D(&textureDesc, nullptr, &canvasTexture)))
		{
			std::cout << "CreateTexture2D() failed." << std::endl;
			return false;
		}

		if (FAILED(device->CreateShaderResourceView(canvasTexture, nullptr, &canvasTextureView)))
		{
			std::cout << "CreateShaderResourceView() failed." << std::endl;
			return false;
		}
	}
	
	return true;
}

bool CpuRenderer::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

	ImGui::StyleColorsLight();

	if (!ImGui_ImplDX11_Init(device.Get(), context.Get()))
	{
		std::cout << "ImGui_ImplDX11_Init() failed." << std::endl;
		return false;
	}

	if (!ImGui_ImplWin32_Init(mainWindowHandle))
	{
		std::cout << "ImGui_ImplWin32_Init() failed." << std::endl;
		return false;
	}

	return true;
}

void CpuRenderer::updateGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Scene Control");

	// gui update
	if(selectedMesh && selectedLight)
	{
		ImGui::SliderAngle("Object RotationAboutX", &selectedMesh->Transform.rotationX);

		ImGui::SliderAngle("Object RotationAboutY", &selectedMesh->Transform.rotationY);

		ImGui::SliderAngle("Object RotationAboutZ", &selectedMesh->Transform.rotationZ);

		ImGui::SliderFloat3("Object Translation", &selectedMesh->Transform.translation.x, -3.0f, 4.0f);

		ImGui::SliderFloat3("Object Scale", &selectedMesh->Transform.scale.x, 0.1f, 2.0f);

		ImGui::SliderFloat3("Material ambient", &selectedMesh->Material.Ambient.x, 0.0f, 1.0f);

		if (!selectedMesh->Material.diffuseTex)
		{
			ImGui::SliderFloat3("Material diffuse", &selectedMesh->Material.Diffuse.x, 0.0f, 1.0f);
		}

		ImGui::SliderFloat3("Material specular", &selectedMesh->Material.Specular.x, 0.0f, 1.0f);

		ImGui::SliderFloat("Material shininess", &selectedMesh->Material.Shininess, 0.0f, 256.0f);

		if (ImGui::RadioButton("Directional Light", static_cast<int>(lightType) == 0))
		{
			lightType = ELightType::Directional;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Point Light", static_cast<int>(lightType) == 1))
		{
			lightType = ELightType::Point;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Spot Light", static_cast<int>(lightType) == 2))
		{
			lightType = ELightType::Spot;
		}

		ImGui::SliderFloat3("Light Strength", &selectedLight->Strength.x, 0.0f, 1.0f);

		if (ImGui::SliderFloat3("Light Direction", &selectedLight->Direction.x, -3.0f, 3.0f))
		{
			if (selectedLight->Direction.Length() > 1e-5f)
			{
				selectedLight->Direction.Normalize();
			}
		};

		ImGui::SliderFloat3("Light Position", &selectedLight->Position.x, -2.0f, 2.0f);

		ImGui::SliderFloat("Light fallOffStart", &selectedLight->FallOffStart, 0.0f, 5.0f);

		ImGui::SliderFloat("Light fallOffEnd", &selectedLight->FallOffEnd, 0.0f, 10.0f);

		ImGui::SliderFloat("Light spotPower", &selectedLight->SpotPower, 0.0f, 512.0f);
	}

	ImGui::End();
	ImGui::Render();
}

shared_ptr<vector<Vector4>> CpuRenderer::renderViaCpu()
{
	return cpuPipeline->Process();
}

bool CpuRenderer::selectModel()
{
	if (!objects)
	{
		return false;
	}

	if (objects->empty())
	{
		return false;
	}

	// @todo. 여러 개의 오브젝트 설정 구현
	for (auto& object : *objects.get())
	{
		shared_ptr<Mesh> mesh = dynamic_pointer_cast<Mesh>(object);
		if (mesh)
		{
			selectedMesh = mesh;
		}
		shared_ptr<Light> light = dynamic_pointer_cast<Light>(object);
		if (light)
		{
			selectedLight = light;
		}
	}

	if (selectedMesh && selectedLight)
	{
		return true;
	}

	return false;
}

bool CpuRenderer::initShaders()
{
	ComPtr<ID3DBlob> vertexBlob = nullptr;
	ComPtr<ID3DBlob> pixelBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	if (FAILED(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBlob, &errorBlob)))
	{
		if (errorBlob)
		{
			std::cout << "Vertex shader compile errer\n" << (char*)errorBlob->GetBufferPointer() << std::endl;
			return false;
		}
	}

	if (FAILED(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBlob, &errorBlob)))
	{
		if (errorBlob)
		{
			std::cout << "Pixel shader compile errer\n" << (char*)errorBlob->GetBufferPointer() << std::endl;
			return false;
		}
	}

	if (FAILED(device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &vertexShader)))
	{
		std::cout << "CreateVertexShader() failed." << std::endl;
		return false;
	}

	if (FAILED(device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &pixelShader)))
	{
		std::cout << "CreatePixelShader() failed." << std::endl;
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC inputElemnetDest[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	if (FAILED(device->CreateInputLayout(inputElemnetDest, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &layout)))
	{
		std::cout << "CreateInputLayout() failed." << std::endl;
		return false;
	}

	context->IASetInputLayout(layout);
	return true;
}
