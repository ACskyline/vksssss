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
#define SKIN_FRESNEL_F0 0.028
#define PI 3.14159265359

#define TSM_USE_BIAS_MAX
#define TSM_USE_BIAS_MIN

#ifdef TSM_USE_BIAS_MIN
	#define TSM_BIAS_MIN(X, Y) (X)
#else
	#define TSM_BIAS_MIN(X, Y) (Y)
#endif

#ifdef TSM_USE_BIAS_MAX
	#define TSM_BIAS_MAX(X, Y) (X)
#else
	#define TSM_BIAS_MAX(X, Y) (Y)
#endif

struct LightData {
	mat4 view;
	mat4 proj;
	mat4 viewInv;
	mat4 projInv;
	vec4 color;//Warning: Implementations sometimes get the std140 layout wrong for vec3 components. You are advised to manually pad your structures/arrays out and avoid using vec3 at all.
	vec4 position;//https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
	int textureIndex;
	float near;
	float far;
	uint PADDING0;
};

layout(set = SCENE_SET, binding = UBO_SLOT(SCENE, 0)) uniform SceneUniformBufferObject {
    uint time;
	uint deferredMode;
	uint shadowMode;
	uint lightCount;
	//for skin
	float m;
	float rho_s;
	//for sub-surface scattering
	float stretchAlpha;
	float stretchBeta;
	//for translucency
	uint tsmMode;
	float scattering;
	float absorption;
	float translucencyScale;
	float translucencyPower;
	float tsmBiasMax;
	float tsmBiasMin;
	float distortion;
	float distanceScale;
	//for shadow
	float shadowBias;
	float shadowScale;
	uint PADDING0;
	LightData lightArr[MAX_LIGHTS_PER_SCENE];
} sceneUBO;

layout(set = PASS_SET, binding = UBO_SLOT(PASS, 0)) uniform PassUniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 viewInv;
	mat4 projInv;
	vec4 cameraPosition;
	uint passNum;
	float near;
	float far;
	uint PADDING_0;
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
		ndc.z < depth + sceneUBO.shadowBias)
		return 1;
	else
		return 0;
}

float ShadowFeelerPCF(mat4 view, mat4 proj, vec3 pos, sampler2DShadow shadowMap)
{
	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	vec3 comp = vec3(uv, ndc.z - sceneUBO.shadowBias);
	if (-1 < ndc.x && ndc.x < 1 &&
		-1 < ndc.y && ndc.y < 1)
		return dot(vec4(0.25, 0.25, 0.25, 0.25), textureGather(shadowMap, comp.xy, comp.z));//texture(shadowMap, comp).x;
	else
		return 0;
}

float ShadowFeelerPCF_5X5(mat4 view, mat4 proj, vec3 pos, sampler2DShadow shadowMap)
{
	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	float result = 0;
	vec2 shadowMapSize = textureSize(shadowMap, 0);
	for (int i = -2; i <= 2; i++)
	{
		for (int j = -2; j <= 2; j++)
		{
			vec3 comp = vec3(uv + vec2(i, j) / shadowMapSize, ndc.z - sceneUBO.shadowBias);
			if (-1 < ndc.x && ndc.x < 1 &&
				-1 < ndc.y && ndc.y < 1)
				result += dot(vec4(0.25, 0.25, 0.25, 0.25), textureGather(shadowMap, comp.xy, comp.z)); //texture(shadowMap, comp).x;
		}
	}
	return result / 25.0;
}

float ShadowFeelerPCF_7X7(mat4 view, mat4 proj, vec3 pos, sampler2DShadow shadowMap)
{
	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	float result = 0;
	vec2 shadowMapSize = textureSize(shadowMap, 0);
	for (int i = -3; i <= 3; i++)
	{
		for (int j = -3; j <= 3; j++)
		{
			vec3 comp = vec3(uv + vec2(i, j) / shadowMapSize, ndc.z - sceneUBO.shadowBias);
			if (-1 < ndc.x && ndc.x < 1 &&
				-1 < ndc.y && ndc.y < 1)
				result += dot(vec4(0.25, 0.25, 0.25, 0.25), textureGather(shadowMap, comp.xy, comp.z)); //texture(shadowMap, comp).x;
		}
	}
	return result / 49.0;
}

//Schlick's approximation
float FresnelReflectanceSchlick(vec3 H, vec3 V, float F0)
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
		float F = FresnelReflectanceSchlick(H, V, SKIN_FRESNEL_F0);
		float frSpec = max(PH * F / dot(h, h), 0);
		result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s
	}
	return result;
}

//assuming z axis in NDC ranges from 0 to 1
float LinearizeDepth(float depth, float near, float far)
{
	return far * near / ((far - near) * depth - far);
}

float EvaluateTSM(float lod, vec2 uv, out vec3 xIn, out vec3 nIn, float near, float far, mat4 viewInv, mat4 projInv, sampler2D TSM)
{
	vec4 texel = textureLod(TSM, uv, lod);
	float depth = texel.r;
	vec3 geometryNormalIn = texel.gba * 2.0 - 1.0;
	float linearDepth = LinearizeDepth(depth, near, far);
	vec2 ndcXY = uv * 2.0 - 1.0;
	vec4 clipIn = vec4(ndcXY, depth, 1.0) * linearDepth;
	vec4 posIn = viewInv * projInv * clipIn;
	posIn /= posIn.w;//in case w is -1, beacuse opengl style projection matrix use -z as w instead of z

	xIn = posIn.xyz;
	nIn = normalize(geometryNormalIn);

	return depth;
}

float CompareDepthTSM(vec3 ndc, float depth)
{
	if (-1 < ndc.x && ndc.x < 1 &&
		-1 < ndc.y && ndc.y < 1 &&
		ndc.z < depth + sceneUBO.shadowBias)
		return 1;
	else
		return 0;
}

float BSSRDF(vec3 xOut, vec3 xIn, vec3 nIn, float eta)
{
	float sigmaT = sceneUBO.absorption + sceneUBO.scattering;
	float alpha = sceneUBO.scattering / sigmaT;
	float sigmaTR = sqrt(3.0*sceneUBO.absorption*sigmaT);
	float Fdr = -1.44 / (eta * eta) + 0.71 / eta + 0.668 + 0.0636 * eta;
	float A = (1.0 + Fdr) / (1.0 - Fdr);
	float D = 1.0 / (3.0 * sigmaT);
	float zR = 1.0 / sigmaT;
	float zV = zR + 4.0*A*D;
	vec3 xR = xIn - zR * nIn;
	vec3 xV = xIn + zV * nIn;
	float dR = length(xR - xOut);
	float dV = length(xV - xOut);
	float R = alpha / (4.0 * PI) *
		(zR * (sigmaTR * dR + 1) * exp(-sigmaTR * dR) / (sigmaT * dR * dR * dR) +
			zV * (sigmaTR * dV + 1) * exp(-sigmaTR * dV) / (sigmaT * dV * dV * dV));
	return R;
}

//r = shadow, gba = translucency
vec4 MultiLevelBSSRDF(vec3 cameraDirWorld, vec3 transmitanceMask, vec3 pos, vec3 normal, vec3 lightPos, vec3 lightCol, float near, float far, mat4 view, mat4 proj, mat4 viewInv, mat4 projInv, sampler2D TSM)
{
	vec3 color = vec3(0);
	int colorCount = 0;
	float shadow = 0;
	int shadowCount = 0;

	vec4 clip = proj * view * vec4(pos, 1.0);
	vec3 ndc = clip.xyz / clip.w;
	vec2 uv = ndc.xy * 0.5 + 0.5;
	vec3 lightDir = normalize(lightPos - pos);
	ivec2 sizeTSM = textureSize(TSM, 0);

	float w = sizeTSM.x;
	float h = sizeTSM.y;
	float lod = 0;

	//debug stuff
	float bssrdfAsColor = 0;
	vec3 distanceDifferenceAsColor = 0.0.xxx;

	//lod 0
	//     x
	//     x
	// x x x x x
	//     x
	//     x
	for (int i = -2; i <= 2;i++)
	{
		vec3 xIn = vec3(0);
		vec3 nIn = vec3(0);
		vec2 uvIn = uv + vec2(float(i) / w, 0);
		float depth = EvaluateTSM(lod, uvIn, xIn, nIn, near, far, viewInv, projInv, TSM);
		shadow += CompareDepthTSM(ndc, depth);
		shadowCount++;
		vec3 lightDirIn = vec3(0);
		float bssrdf = 0;
		if (TSM_BIAS_MIN(depth > ndc.z - sceneUBO.tsmBiasMin, true) && TSM_BIAS_MAX(depth < ndc.z + sceneUBO.tsmBiasMax, true))//caution! not linear!
		{
			//within a certain threshold
			lightDirIn = normalize(lightPos - xIn);
			bssrdf = BSSRDF(pos, xIn, nIn, SKIN_FRESNEL_F0);
			color += bssrdf * lightCol * max(0, dot(nIn, lightDirIn)) * (1.0 - FresnelReflectanceSchlick(nIn, lightDirIn, SKIN_FRESNEL_F0));
			colorCount++;
			bssrdfAsColor += bssrdf;
			distanceDifferenceAsColor += (depth == 0.0 ? vec3(1, 0, 0) : vec3(length(xIn - pos)));
		}

		//only one sample at the center
		if (i != 0)
		{
			uvIn = uv + vec2(0, float(i) / h);
			depth = EvaluateTSM(lod, uvIn, xIn, nIn, near, far, viewInv, projInv, TSM);
			shadow += CompareDepthTSM(ndc, depth);
			shadowCount++;
			if (TSM_BIAS_MIN(depth > ndc.z - sceneUBO.tsmBiasMin, true) && TSM_BIAS_MAX(depth < ndc.z + sceneUBO.tsmBiasMax, true))//caution! not linear!
			{
				//within a certain threshold
				lightDirIn = normalize(lightPos - xIn);
				bssrdf = BSSRDF(pos, xIn, nIn, SKIN_FRESNEL_F0);
				color += bssrdf * lightCol * max(0, dot(nIn, lightDirIn)) * (1.0 - FresnelReflectanceSchlick(nIn, lightDirIn, SKIN_FRESNEL_F0));
				colorCount++;
				bssrdfAsColor += bssrdf;
				distanceDifferenceAsColor += (depth == 0.0 ? vec3(1, 0, 0) : vec3(length(xIn - pos)));
			}
		}
	}

	w /= 2.0;
	h /= 2.0;
	lod++;

	//lod 1
	// o o x o o
	// o o x o o
	// x x x x x
	// o o x o o
	// o o x o o
	for (int i = -1; i <= 1; i += 2)
	{
		for (int j = -1; j <= 1; j += 2)
		{
			vec3 xIn = vec3(0);
			vec3 nIn = vec3(0);
			vec2 uvIn = uv + vec2(float(i) / w, float(j) / h);
			float depth = EvaluateTSM(lod, uvIn, xIn, nIn, near, far, viewInv, projInv, TSM);
			shadow += CompareDepthTSM(ndc, depth);
			shadowCount++;
			if (TSM_BIAS_MIN(depth > ndc.z - sceneUBO.tsmBiasMin, true) && TSM_BIAS_MAX(depth < ndc.z + sceneUBO.tsmBiasMax, true))//caution! not linear!
			{
				//within a certain threshold
				vec3 lightDirIn = normalize(lightPos - xIn);
				float bssrdf = BSSRDF(pos, xIn, nIn, SKIN_FRESNEL_F0);
				color += bssrdf * lightCol * max(0, dot(nIn, lightDirIn)) * (1.0 - FresnelReflectanceSchlick(nIn, lightDirIn, SKIN_FRESNEL_F0));
				colorCount++;
				bssrdfAsColor += bssrdf;
				distanceDifferenceAsColor += (depth == 0.0 ? vec3(1, 0, 0) : vec3(length(xIn - pos)));
			}
		}
	}

	w /= 2.0;
	h /= 2.0;
	lod++;

	//lod 2
	// u u u u           u u u u
	// u u u u           u u u u
	// u u u u           u u u u
	// u u u u           u u u u
	//         o o x o o
	//         o o x o o
	//         x x x x x
	//         o o x o o
	//         o o x o o
	// u u u u           u u u u
	// u u u u           u u u u
	// u u u u           u u u u
	// u u u u           u u u u
	for (int i = -1; i <= 1; i += 2)
	{
		for (int j = -1; j <= 1; j += 2)
		{
			vec3 xIn = vec3(0);
			vec3 nIn = vec3(0);
			vec2 uvIn = uv + vec2(float(i) / w, float(j) / h);
			float depth = EvaluateTSM(lod, uvIn, xIn, nIn, near, far, viewInv, projInv, TSM);
			shadow += CompareDepthTSM(ndc, depth);
			shadowCount++;
			if (TSM_BIAS_MIN(depth > ndc.z - sceneUBO.tsmBiasMin, true) && TSM_BIAS_MAX(depth < ndc.z + sceneUBO.tsmBiasMax, true))//caution! not linear!
			{
				//within a certain threshold
				vec3 lightDirIn = normalize(lightPos - xIn);
				float bssrdf = BSSRDF(pos, xIn, nIn, SKIN_FRESNEL_F0);
				color += bssrdf * lightCol * max(0, dot(nIn, lightDirIn)) * (1.0 - FresnelReflectanceSchlick(nIn, lightDirIn, SKIN_FRESNEL_F0));
				colorCount++;
				bssrdfAsColor += bssrdf;
				distanceDifferenceAsColor += (depth == 0.0 ? vec3(1, 0, 0) : vec3(length(xIn - pos)));
			}
		}
	}

	w /= 2.0;
	h /= 2.0;
	lod++;

	//lod 3
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//             u u ! ! ! ! ! ! ! ! ! u u
	//             u u ! ! ! ! ! ! ! ! ! u u
	// ! ! ! ! ! ! ! ! u u           u u ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! ! u u           u u ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! !     o o x o o     ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! !     o o x o o     ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! !     x x x x x     ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! !     o o x o o     ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! !     o o x o o     ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! ! u u           u u ! ! ! ! ! ! ! !
	// ! ! ! ! ! ! ! ! u u           u u ! ! ! ! ! ! ! !
	//             u u ! ! ! ! ! ! ! ! ! u u
	//             u u ! ! ! ! ! ! ! ! ! u u
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	//                 ! ! ! ! ! ! ! ! !
	for (int i = -1; i <= 1; i+=2)
	{
		vec3 xIn = vec3(0);
		vec3 nIn = vec3(0);
		vec2 uvIn = uv + vec2(float(i) / w, 0);
		float depth = EvaluateTSM(lod, uvIn, xIn, nIn, near, far, viewInv, projInv, TSM);
		shadow += CompareDepthTSM(ndc, depth);
		shadowCount++;
		if (TSM_BIAS_MIN(depth > ndc.z - sceneUBO.tsmBiasMin, true) && TSM_BIAS_MAX(depth < ndc.z + sceneUBO.tsmBiasMax, true))//caution! not linear!
		{
			vec3 lightDirIn = normalize(lightPos - xIn);
			float bssrdf = BSSRDF(pos, xIn, nIn, SKIN_FRESNEL_F0);
			color += bssrdf * lightCol * max(0, dot(nIn, lightDirIn)) * (1.0 - FresnelReflectanceSchlick(nIn, lightDirIn, SKIN_FRESNEL_F0));
			colorCount++;
			bssrdfAsColor += bssrdf;
			distanceDifferenceAsColor += (depth == 0.0 ? vec3(1, 0, 0) : vec3(length(xIn - pos)));
		}

		//no sample at center
		{
			uvIn = uv + vec2(0, float(i) / h);
			depth = EvaluateTSM(lod, uvIn, xIn, nIn, near, far, viewInv, projInv, TSM);
			shadow += CompareDepthTSM(ndc, depth);
			shadowCount++;
			if (TSM_BIAS_MIN(depth > ndc.z - sceneUBO.tsmBiasMin, true) && TSM_BIAS_MAX(depth < ndc.z + sceneUBO.tsmBiasMax, true))//caution! not linear!
			{
				vec3 lightDirIn = normalize(lightPos - xIn);
				float bssrdf = BSSRDF(pos, xIn, nIn, SKIN_FRESNEL_F0);
				color += bssrdf * lightCol * max(0, dot(nIn, lightDirIn)) * (1.0 - FresnelReflectanceSchlick(nIn, lightDirIn, SKIN_FRESNEL_F0));
				colorCount++;
				bssrdfAsColor += bssrdf;
				distanceDifferenceAsColor += (depth == 0.0 ? vec3(1, 0, 0) : vec3(length(xIn - pos)));
			}
		}
	}

	bssrdfAsColor /= (colorCount == 0 ? 1 : colorCount);
	distanceDifferenceAsColor /= (colorCount == 0 ? 1 : colorCount);
	color /= (colorCount == 0 ? 1 : colorCount);
	shadow /= (shadowCount == 0 ? 1 : shadowCount);

	if (sceneUBO.tsmMode == 3)
		color = vec3(clamp(dot(-normal, normalize(lightPos - pos)), 0.0, 1.0)) *
				vec3(clamp(dot(normal, cameraDirWorld), 0.0, 1.0)) *
				exp(-sceneUBO.scattering * max((distanceDifferenceAsColor), transmitanceMask));
	else if (sceneUBO.tsmMode == 2)
		color = (exp(-max(vec3(0), distanceDifferenceAsColor * sceneUBO.distanceScale)));
	else if (sceneUBO.tsmMode == 1)
		color = clamp((distanceDifferenceAsColor * sceneUBO.distanceScale), 0.0, 1.0);
	else
		color = clamp(color, 0.0, 1.0) *
				vec3(clamp(dot(-normal, normalize(lightPos - pos)), 0.0, 1.0)) *
				vec3(clamp(dot(normal, cameraDirWorld), 0.0, 1.0)) *
				exp(-sceneUBO.scattering * max((distanceDifferenceAsColor), transmitanceMask));

	return vec4(shadow, color);
}

//r = shadow, gba = translucency
vec4 ShadowFeelerTSM(vec3 cameraDirWorld, vec3 transmitanceMask, vec3 pos, vec3 normal, vec3 lightPos, vec3 lightCol, float near, float far, mat4 view, mat4 proj, mat4 viewInv, mat4 projInv, sampler2D TSM)
{
	return MultiLevelBSSRDF(cameraDirWorld, transmitanceMask, pos, normal, lightPos, lightCol, near, far, view, proj, viewInv, projInv, TSM);
}
