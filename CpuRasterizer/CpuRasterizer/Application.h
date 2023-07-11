#pragma once

#include <memory>

#include "IRenderer.h"

enum class ERenderer
{
	None = 0,
	//D3d11,
	D3d11Cpu,
	//D3d11Rt,
	//D3d11Pbr,
	//Vk,
	//VkRt,
	//VkPbr
};

class Application
{
private:
	constexpr static int windowWidth = 1280;
	constexpr static int windowHeight = 960;

	HWND mainWindowHandle;

	std::unique_ptr<IRenderer> renderer;

public:
	Application();
	~Application();

	bool Initialize(const ERenderer type);
	int Run();

private:
	bool initMainWindows();

	bool createRenderer(const ERenderer& type);
};

