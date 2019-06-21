#include "Shader.h"
#include "ShaderIncluder.h"

#include "Renderer.h"

Shader::Shader() : Shader(ShaderType::Count, "no_file")
{
}

Shader::Shader(const ShaderType& _type, const std::string& _fileName) :
	type(_type),
	fileName(_fileName),
	shaderBytecode(0),
	shaderString("no file")
{
	shaderStageInfo = {};
}

Shader::~Shader()
{
	ResetShaderBytecode();//not necessary
	CleanUp();
}

void Shader::InitShader(Renderer* _pRenderer)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("shader " + fileName + " : pRenderer is null!");
	}

	pRenderer = _pRenderer;
	if (type == ShaderType::VertexShader)
	{
		if(!CreateShaderFromFile(shaderc_vertex_shader, false))
			throw std::runtime_error("shader " + fileName + " : create shader failed!");

		shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	}
	else if (type == ShaderType::TessellationControlShader)
	{
		if(!CreateShaderFromFile(shaderc_tess_control_shader, false))
			throw std::runtime_error("shader " + fileName + " : create shader failed!");

		shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	}
	else if (type == ShaderType::TessellationEvaluationShader)
	{
		if(!CreateShaderFromFile(shaderc_tess_evaluation_shader, false))
			throw std::runtime_error("shader " + fileName + " : create shader failed!");

		shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	}
	else if (type == ShaderType::GeometryShader)
	{
		if(!CreateShaderFromFile(shaderc_geometry_shader, false))
			throw std::runtime_error("shader " + fileName + " : create shader failed!");

		shaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	else if (type == ShaderType::FragmentShader)
	{
		if(!CreateShaderFromFile(shaderc_fragment_shader, false))
			throw std::runtime_error("shader " + fileName + " : create shader failed!");

		shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	CreateShaderModule(pRenderer->GetDevice());
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main";
}

void Shader::CreateShaderModule(const VkDevice& device)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderBytecode.size() * sizeof(uint32_t);
	createInfo.pCode = shaderBytecode.data();

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create shader module!");
	}
}

VkPipelineShaderStageCreateInfo Shader::GetShaderStageInfo() const
{
	return shaderStageInfo;
}

Shader::ShaderType Shader::GetShaderType() const
{
	return type;
}

const std::string Shader::GetFileName() const
{
	return fileName;
}

void Shader::ResetShaderBytecode()
{
	shaderBytecode.clear();
}

bool Shader::ReadShaderFromFile()
{
	std::ifstream shader;
	//open file
	//important!!! open in binary mode to keep \r\n as it is, so that length is the actual length
	shader.open(fileName, std::ios::binary);//open in binary mode
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
	//options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);
	std::unique_ptr<ShaderIncluder> includer(new ShaderIncluder(&shaderc_util::FileFinder()));
	options.SetIncluder(std::move(includer));

	//std::cout << "output 1:" << shaderString << std::endl;

	// preprocess
	shaderc::PreprocessedSourceCompilationResult preprocessed = compiler.PreprocessGlsl(shaderString, kind, fileName.c_str(), options);

	if (preprocessed.GetCompilationStatus() != shaderc_compilation_status_success) 
	{
		throw std::runtime_error(preprocessed.GetErrorMessage());
		return false;
	}

	shaderString = { preprocessed.cbegin(), preprocessed.cend() };

	//std::cout << "output 2:" << shaderString << std::endl;

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

void Shader::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyShaderModule(pRenderer->GetDevice(), shaderModule, nullptr);
		pRenderer = nullptr;
	}
}