cbuffer cbWorld : register(b0)
{
    matrix matWorld;
};

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

PS_IN VS(VS_IN input)
{
    PS_IN output;
    output.pos = mul(float4(input.pos, 1.0f), matWorld);
    output.uv = input.uv;
    return output;
}

float4 PS(PS_IN input) : SV_Target
{
    return tex0.Sample(samp0, input.uv);
}