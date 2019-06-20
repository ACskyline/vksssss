#pragma once

#include "Renderer.h"
#include "GlobalInclude.h"
#include "Dependencies/shaderc/include/shaderc.hpp"

#include <fstream>

class Renderer;

using namespace shaderc;

class Shader
{
public:
	enum ShaderType { VertexShader, TessControlShader, TessEvaluationShader, GeometryShader, FragmentShader, ComputeShader, Undefined };

	Shader();
	Shader(const ShaderType& _type, const std::string& _fileName);
	~Shader();

	void InitShader(Renderer* _pRenderer);
	const std::string GetFileName() const;
	const VkPipelineShaderStageCreateInfo& GetShaderStageInfo() const;
	void ResetShaderBytecode();

	void CleanUp();

private:
	Renderer* pRenderer;
	ShaderType type;
	std::string fileName;
	std::vector<uint32_t> shaderBytecode;
	std::string shaderString;
	VkShaderModule shaderModule;
	VkPipelineShaderStageCreateInfo shaderStageInfo;

	bool ReadShaderFromFile(); 
	bool CreateShaderFromFile(shaderc_shader_kind kind, bool optimize = false);
	void CreateShaderModule(const VkDevice& device);
};
