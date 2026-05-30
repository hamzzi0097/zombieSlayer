cbuffer cbWorld : register(b0)
{
    matrix matWorld;
};
cbuffer cbMaterial : register(b1)
{
    float4 tintColor;
};

struct VS_IN
{
    float3 pos : POSITION;
    float4 col : COLOR;
};
struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PS_IN VS(VS_IN input)
{
    PS_IN output;
    output.pos = mul(float4(input.pos, 1.0f), matWorld);
    output.col = input.col;
    return output;
}

float4 PS(PS_IN input) : SV_Target
{
    // tintColor.rgb : 색상 (빨강 등)
    // tintColor.a   : 전체 강도 (페이드)
    // input.col.a   : 버텍스별 알파 (그라데이션 마스크)
    return float4(tintColor.rgb, tintColor.a * input.col.a);
}
