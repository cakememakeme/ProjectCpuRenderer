#pragma once

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
#include <directxtk/SimpleMath.h>

#include "Mesh.h"
#include "Texture.h"

class MeshLoader
{
private:
	std::string basePath;
	std::vector<Mesh> meshes;

public:
	std::vector<Mesh>&& Load(std::string basePath, std::string fileName);

private:
	void processNode(aiNode* node, const aiScene* scene, DirectX::SimpleMath::Matrix transform);

	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	void calcNormal();
};

class TextureLoader
{
public:
	static Texture Load(std::string fileName);
};

