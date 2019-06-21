#pragma once

#include <fstream>
#include "Dependencies/shaderc/include/shaderc.hpp"
#include "GlobalInclude.h"

class Renderer;

using namespace shaderc;

class Shader
{
public:
	enum class ShaderType { VertexShader, TessellationControlShader, TessellationEvaluationShader, GeometryShader, FragmentShader, Count };

	Shader();
	Shader(const ShaderType& _type, const std::string& _fileName);
	~Shader();

	const std::string GetFileName() const;
	VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;
	ShaderType GetShaderType() const;

	void ResetShaderBytecode();

	void InitShader(Renderer* _pRenderer);
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
