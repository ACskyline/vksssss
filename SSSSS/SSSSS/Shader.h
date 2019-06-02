#pragma once

#include "GlobalInclude.h"
#include "Dependencies/shaderc/include/shaderc.hpp"

#include <fstream>

using namespace shaderc;

class Shader
{
public:
	enum ShaderType { VertexShader, TessControlShader, TessEvaluationShader, GeometryShader, FragmentShader, ComputeShader, Undefined };

	Shader();
	Shader(const ShaderType& _type, const string& _fileName);
	~Shader();

	bool CreateShader();
	const vector<uint32_t>& GetShaderBytecode();
	const string GetFileName();
	void ResetShaderBytecode();

private:
	ShaderType type;
	string fileName;
	vector<uint32_t> shaderBytecode;
	string shaderString;

	bool ReadShaderFromFile(); 
	bool CreateShaderFromFile(shaderc_shader_kind kind, bool optimize = false);
};
