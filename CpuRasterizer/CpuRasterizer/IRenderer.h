#pragma once

#include <windows.h>

#include <vector>
#include <memory>

class Object;

class IRenderer
{
public:
	virtual bool Create() = 0;

	virtual bool Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight) = 0;

	virtual bool SetObjects(std::shared_ptr<std::vector<std::shared_ptr<Object>>> receivedObjects) = 0;

	virtual void Update() = 0;

	virtual void Render() = 0;

	virtual void OnPostRender() = 0;

	virtual bool Reset() = 0;
};

