#pragma once

#include "Dependencies/shaderc_util/include/file_finder.h"
#include "Dependencies/shaderc/include/shaderc.hpp"
#include <unordered_set>

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
	explicit ShaderIncluder(const shaderc_util::FileFinder* file_finder)
		: file_finder_(*file_finder) {}

	~ShaderIncluder() override;

	// Resolves a requested source file of a given type from a requesting
	// source into a shaderc_include_result whose contents will remain valid
	// until it's released.
	shaderc_include_result* GetInclude(const char* requested_source,
		shaderc_include_type type,
		const char* requesting_source,
		size_t include_depth) override;
	// Releases an include result.
	void ReleaseInclude(shaderc_include_result* include_result) override;

	// Returns a reference to the member storing the set of included files.
	const std::unordered_set<std::string>& file_path_trace() const {
		return included_files_;
	};

private:
	// Used by GetInclude() to get the full filepath.
	const shaderc_util::FileFinder& file_finder_;
	// The full path and content of a source file.
	struct FileInfo {
		const std::string full_path;
		std::vector<char> contents;
	};

	// The set of full paths of included files.
	std::unordered_set<std::string> included_files_;
};
