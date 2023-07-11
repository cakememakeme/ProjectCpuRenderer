struct VSInput
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VSOutput main(VSInput vsInput)
{
	VSOutput vsOutput;

	vsOutput.position = vsInput.position;
	vsOutput.uv = vsInput.uv;

	return vsOutput;
}
