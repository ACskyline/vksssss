#include "ShaderIncluder.h"
#include "Dependencies/shaderc_util/include/io.h"

shaderc_include_result* MakeErrorIncludeResult(const char* message) {
	return new shaderc_include_result{ "", 0, message, strlen(message) };
}

ShaderIncluder::~ShaderIncluder() = default;

shaderc_include_result* ShaderIncluder::GetInclude(
	const char* requested_source, shaderc_include_type include_type,
	const char* requesting_source, size_t) {

	const std::string full_path =
		(include_type == shaderc_include_type_relative)
		? file_finder_.FindRelativeReadableFilepath(requesting_source,
			requested_source)
		: file_finder_.FindReadableFilepath(requested_source);

	if (full_path.empty())
		return MakeErrorIncludeResult("Cannot find or open include file.");

	// In principle, several threads could be resolving includes at the same
	// time.  Protect the included_files.

	// Read the file and save its full path and contents into stable addresses.
	FileInfo* new_file_info = new FileInfo{ full_path, {} };
	if (!shaderc_util::ReadFile(full_path, &(new_file_info->contents))) {
		return MakeErrorIncludeResult("Cannot read file");
	}

	included_files_.insert(full_path);

	return new shaderc_include_result{
		new_file_info->full_path.data(), new_file_info->full_path.length(),
		new_file_info->contents.data(), new_file_info->contents.size(),
		new_file_info };
}

void ShaderIncluder::ReleaseInclude(shaderc_include_result* include_result) {
	FileInfo* info = static_cast<FileInfo*>(include_result->user_data);
	delete info;
	delete include_result;
}