#include "ModelLoader.h"

#include <vector>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Vertex.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;
using namespace std;

std::vector<Mesh>&& MeshLoader::Load(std::string basePath, std::string fileName)
{
	this->basePath = basePath;

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(
		basePath + fileName,
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	Matrix transform;
	processNode(scene->mRootNode, scene, transform);

    calcNormal();

    return std::move(meshes);
}

void MeshLoader::processNode(aiNode* node, const aiScene* scene, DirectX::SimpleMath::Matrix transform)
{
	Matrix mat;
	ai_real* aiRealTemp = &node->mTransformation.a1;
	float* floatTemp = &mat._11;
    for (int t = 0; t < 16; t++) 
    {
        floatTemp[t] = float(aiRealTemp[t]);
    }
    mat = mat.Transpose() * transform;

    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto newMesh = this->processMesh(mesh, scene);

        for (auto& v : newMesh.Vertices) 
        {
            v.Position = DirectX::SimpleMath::Vector3::Transform(v.Position, mat);
        }

        meshes.push_back(newMesh);
    }

    for (UINT i = 0; i < node->mNumChildren; i++) 
    {
        this->processNode(node->mChildren[i], scene, mat);
    }
}

Mesh MeshLoader::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<size_t> indices;

    // Walk through each of the mesh's vertices
    for (UINT i = 0; i < mesh->mNumVertices; ++i) 
    {
        Vertex vertex;

        vertex.Position.x = mesh->mVertices[i].x;
        vertex.Position.y = mesh->mVertices[i].y;
        vertex.Position.z = mesh->mVertices[i].z;

        vertex.Normal.x = mesh->mNormals[i].x;
        vertex.Normal.y = mesh->mNormals[i].y;
        vertex.Normal.z = mesh->mNormals[i].z;
        vertex.Normal.Normalize();

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoord.x = (float)mesh->mTextureCoords[0][i].x;
            vertex.TexCoord.y = (float)mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(vertex);
    }

    for (UINT i = 0; i < mesh->mNumFaces; ++i) 
    {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh newMesh;
    newMesh.Vertices = vertices;
    newMesh.Indices = indices;

    // http://assimp.sourceforge.net/lib_html/materials.html
    if (mesh->mMaterialIndex >= 0) 
    {
        aiMaterial * material = scene->mMaterials[mesh->mMaterialIndex];

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString filepath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &filepath);

            std::string fullPath =
                this->basePath +
                std::string(std::filesystem::path(filepath.C_Str())
                    .filename()
                    .string());

            newMesh.Material.SetDiffuseTexture(TextureLoader::Load(fullPath));
        }
    }

    return newMesh;
}

void MeshLoader::calcNormal()
{
    // 노멀 벡터가 없는 경우를 대비하여 다시 계산
    // 한 위치에는 한 버텍스만 있어야 연결 관계를 찾을 수 있음
    for (auto& m : this->meshes)
    {
        vector<Vector3> normalsTemp(m.Vertices.size(), Vector3(0.0f));
        vector<float> weightsTemp(m.Vertices.size(), 0.0f);

        for (int i = 0; i < m.Indices.size(); i += 3)
        {

            size_t idx0 = m.Indices[i];
            size_t idx1 = m.Indices[i + 1];
            size_t idx2 = m.Indices[i + 2];

            auto v0 = m.Vertices[idx0];
            auto v1 = m.Vertices[idx1];
            auto v2 = m.Vertices[idx2];

            auto faceNormal = (v1.Position - v0.Position).Cross(v2.Position - v0.Position);

            normalsTemp[idx0] += faceNormal;
            normalsTemp[idx1] += faceNormal;
            normalsTemp[idx2] += faceNormal;
            weightsTemp[idx0] += 1.0f;
            weightsTemp[idx1] += 1.0f;
            weightsTemp[idx2] += 1.0f;
        }

        for (int i = 0; i < m.Vertices.size(); i++)
        {
            if (weightsTemp[i] > 0.0f)
            {
                m.Vertices[i].Normal = normalsTemp[i] / weightsTemp[i];
                m.Vertices[i].Normal.Normalize();
            }
        }
    }
}

Texture TextureLoader::Load(std::string fileName)
{
    if (fileName.empty())
    {
        return Texture();
    }

    Texture newTex;

    unsigned char* img = stbi_load(fileName.c_str(), &newTex.Width, &newTex.Height, &newTex.Channels, 0);

    newTex.Image.resize(newTex.Width * newTex.Height * newTex.Channels);
    memcpy(newTex.Image.data(), img, newTex.Image.size() * sizeof(uint8_t));

    delete img;

    return newTex;
}
