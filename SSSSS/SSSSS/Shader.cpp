#include "Shader.h"

Shader::Shader() : Shader(Undefined, "no_file")
{
}

Shader::Shader(const ShaderType& _type, const string& _fileName) :
	type(_type),
	fileName(_fileName),
	shaderBytecode(0),
	shaderString("no file")
{
}

Shader::~Shader()
{
	ResetShaderBytecode();
}

bool Shader::CreateShader()
{
	if (type == ShaderType::VertexShader)
	{
		return CreateShaderFromFile(shaderc_vertex_shader, false);
	}
	else if (type == ShaderType::TessControlShader)
	{
		return CreateShaderFromFile(shaderc_tess_control_shader, false);
	}
	else if (type == ShaderType::TessEvaluationShader)
	{
		return CreateShaderFromFile(shaderc_tess_evaluation_shader, false);
	}
	else if (type == ShaderType::GeometryShader)
	{
		return CreateShaderFromFile(shaderc_geometry_shader, false);
	}
	else if (type == ShaderType::FragmentShader)
	{
		return CreateShaderFromFile(shaderc_fragment_shader, false);
	}
	else if (type == ShaderType::ComputeShader)
	{
		return CreateShaderFromFile(shaderc_compute_shader, false);
	}
	return false;
}

const vector<uint32_t>& Shader::GetShaderBytecode()
{
	return shaderBytecode;
}

const string Shader::GetFileName()
{
	return fileName;
}

void Shader::ResetShaderBytecode()
{
	shaderBytecode.clear();
}

bool Shader::ReadShaderFromFile()
{
	ifstream shader;
	//open file
	//important!!! open in binary mode to keep \r\n as it is, so that length is the actual length
	shader.open(fileName, ios::binary);//open in binary mode
	if (!shader.is_open())
		return false;
	//get length of file
	shader.seekg(0, shader.end);
	size_t length = shader.tellg();
	shader.seekg(0, shader.beg);
	//resize buffer
	shaderString.resize(length);
	//read
	shader.read(&shaderString[0], length);
	//close file
	shader.close();
	return true;
}

bool Shader::CreateShaderFromFile(shaderc_shader_kind kind, bool optimize)
{
	// read shader string
	if (!ReadShaderFromFile())
		return false;

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// like -DMY_DEFINE=1
	// options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

	// compile
	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderString, kind, fileName.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		throw std::runtime_error(module.GetErrorMessage());
		return false;
	}

	shaderBytecode = { module.cbegin(), module.cend() };// not sure why sample code copy vector like this

	return true;
}
