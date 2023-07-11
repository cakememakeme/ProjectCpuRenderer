Texture2D baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(VSOutput vsOutput) : SV_TARGET
{
	return baseColorTexture.Sample(baseColorSampler, vsOutput.uv);
}
