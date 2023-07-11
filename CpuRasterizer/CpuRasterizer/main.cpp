#include <iostream>

#include "Application.h"

static ERenderer GetRendererType(const std::string& arg)
{
	if (arg == "D3d11Cpu")
	{
		return ERenderer::D3d11Cpu;
	}

	return ERenderer::None;
}

int main(int argc, char* argv[])
{
	ERenderer type = GetRendererType(argc < 2 ? "D3d11Cpu" : argv[1]);
	
	Application renderingApp;
	if(!renderingApp.Initialize(type))
	{ 
		std::cout << "Initialization failed." << std::endl;
		return -1;
	}
	std::cout << "Initialization success." << std::endl;

	return renderingApp.Run();
}