#define SCENE_SET 0//per scene
#define FRAME_SET 1//per frame
#define PASS_SET 2//per pass
#define OBJECT_SET 3//per draw call

#define SCENE_UBO_BINDING_COUNT 1
#define FRAME_UBO_BINDING_COUNT 1
#define PASS_UBO_BINDING_COUNT 1
#define OBJECT_UBO_BINDING_COUNT 1

#define UBO_SLOT(NAME, NUMBER) (NUMBER)
#define TEXTURE_SLOT(NAME, NUMBER) (NAME ## _UBO_BINDING_COUNT + NUMBER)

#define MAX_LIGHTS_PER_SCENE 10
#define SHADOW_BIAS 0.0001

struct LightData {
	mat4 view;
	mat4 proj;
	vec4 color;//Warning: Implementations sometimes get the std140 layout wrong for vec3 components. You are advised to manually pad your structures/arrays out and avoid using vec3 at all.
	vec4 position;//https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
	int textureIndex;
	uint PADDING0;
	uint PADDING1;
	uint PADDING2;
};

layout(set = SCENE_SET, binding = UBO_SLOT(SCENE, 0)) uniform SceneUniformBufferObject {
    uint time;
	uint deferredMode;
	uint shadowMode;
	uint lightCount;
	float m;
	float rho_s;
	float stretchAlpha;
	float stretchBeta;
	LightData lightArr[MAX_LIGHTS_PER_SCENE];
} sceneUBO;

layout(set = PASS_SET, binding = UBO_SLOT(PASS, 0)) uniform PassUniformBufferObject {
	mat4 view;
	mat4 proj;
	vec4 cameraPosition;
	uint passNum;
	uint widthTex;
	uint heightTex;
} passUBO;

layout(set = OBJECT_SET, binding = UBO_SLOT(OBJECT, 0)) uniform ObjectUniformBufferObject {
    mat4 model;
	mat4 modelInvTrans;
} objectUBO;

layout(set = FRAME_SET, binding = UBO_SLOT(FRAME, 0)) uniform FrameUniformBufferObject {
    uint frameNum;
} frameUBO;

vec2 FlipV(vec2 uv)
{
	return vec2(uv.x, 1.0 - uv.y);
}

float ShadowFeeler(mat4 view, mat4 proj, vec3 pos, sampler2D shadowMap)
{
	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	float depth = texture(shadowMap, uv.xy).x;
	if (-1 < ndc.x && ndc.x < 1 &&
		-1 < ndc.y && ndc.y < 1 &&
		ndc.z < depth + SHADOW_BIAS)
		return 1;
	else
		return 0;
}

float ShadowFeelerPCF(mat4 view, mat4 proj, vec3 pos, sampler2DShadow shadowMap)
{
	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	vec3 comp = vec3(uv, ndc.z - SHADOW_BIAS);
	if (-1 < ndc.x && ndc.x < 1 &&
		-1 < ndc.y && ndc.y < 1)
		return texture(shadowMap, comp).x;
	else
		return 0;
}

float ShadowFeelerPCF_7X7(mat4 view, mat4 proj, vec3 pos, sampler2DShadow shadowMap)
{
	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	float result = 0;
	for (int i = -3; i <= 3; i++)
	{
		for (int j = -3; j <= 3; j++)
		{
			vec3 comp = vec3(uv + vec2(i / 800.0, j / 800.0), ndc.z - SHADOW_BIAS);
			if (-1 < ndc.x && ndc.x < 1 &&
				-1 < ndc.y && ndc.y < 1)
				result += texture(shadowMap, comp).x;
		}
	}
	return result / 49.0;
}

float fresnelReflectance(vec3 H, vec3 V, float F0)
{
	float base = 1.0 - dot(V, H);
	float exponential = pow(base, 5.0);
	return exponential + F0 * (1.0 - exponential);
}

float PHBeckmann(float ndoth, float m)
{
	float alpha = acos(ndoth);
	float ta = tan(alpha);
	float val = 1.0 / (m*m*pow(ndoth, 4.0))*exp(-(ta*ta) / (m*m));
	return val;
}

float KS_Skin_Specular(vec3 N,//Bumped surface normal
	vec3 L,//Points to light
	vec3 V,//Points to eye
	float m,//Roughness
	float rho_s)//Specular brightness
{
	float result = 0.0;
	float ndotl = dot(N, L);
	if (ndotl > 0.0)
	{
		vec3 h = L + V; // Unnormalized half-way vector
		vec3 H = normalize(h);
		float ndoth = dot(N, H);
		float PH = PHBeckmann(ndoth, m);
		float F = fresnelReflectance(H, V, 0.028);
		float frSpec = max(PH * F / dot(h, h), 0);
		result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s
	}
	return result;
}
