// [texture.hlsl] 텍스처 매핑 셰이더
// - b0: 월드 행렬(MeshRenderer가 바인딩) → 다른 오브젝트처럼 이동/회전/스케일 적용
// - b1: 틴트 색(TextureMaterial) → 곱해서 페이드/색조 조절(기본 흰색이면 원본 그대로)
// - t0: 텍스처, s0: 샘플러
// 입력 레이아웃은 POSITION(offset 0) + TEXCOORD(offset 28). COLOR(offset 12)는 사용 안 함.

cbuffer cbWorld : register(b0)
{
    matrix matWorld;
};
cbuffer cbMaterial : register(b1)
{
    float4 tintColor;
};

Texture2D    tex : register(t0);
SamplerState sam : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
};
struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

PS_IN VS(VS_IN input)
{
    PS_IN o;
    o.pos = mul(float4(input.pos, 1.0f), matWorld);
    o.uv  = input.uv;
    return o;
}

float4 PS(PS_IN input) : SV_Target
{
    return tex.Sample(sam, input.uv) * tintColor;
}
