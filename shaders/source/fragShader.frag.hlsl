Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

const static float SCREEN_WIDTH = 2400.0f;
const static float SCREEN_HEIGHT = 1350.0f;

const static float sizeX = 100.0f;
const static float sizeY = 100.0f;

//float ratioX = SCREEN_WIDTH / sizeX;
//float ratioY = SCREEN_WIDTH / sizeY;
//float2 ratio = float2(SCREEN_WIDTH / sizeX, SCREEN_WIDTH / sizeY);

float4 main(float2 TexCoord : TEXCOORD0) : SV_Target0
{
    //return float4(TexCoord.x, TexCoord.y, 0.0f, 1.0f);
    return Texture.Sample(Sampler, TexCoord);
}