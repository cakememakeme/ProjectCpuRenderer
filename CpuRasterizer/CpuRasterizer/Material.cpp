#include "Material.h"

#include "Texture.h"

using namespace std;

bool Material::SetDiffuseTexture(Texture newTex)
{
	diffuseTex = nullptr;
	diffuseTex = make_shared<Texture>(newTex);
	if (diffuseTex)
	{
		return true;
	}
	return false;
}