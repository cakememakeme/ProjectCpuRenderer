#pragma once

#include "Mesh.h"

class GeometryGenerator
{
public:
	static std::vector<Mesh> ReadFromFile(std::string basePath, std::string filename);
	static Mesh MakeBox();
};
