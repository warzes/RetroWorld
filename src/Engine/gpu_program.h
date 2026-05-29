#pragma once

namespace gpu::program
{
	struct ShaderProgram;
	using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;

	struct ComputeProgramCreateInfo final
	{
		std::string_view name;

		std::string shaderCode{};
	};

	struct GraphicsProgramCreateInfo final
	{
		std::string_view name;

		std::string vertexShaderCode{};
		std::string fragmentShaderCode{};
		std::string tessellationControlShaderCode{};
		std::string tessellationEvaluationShaderCode{};
		std::string geometryShaderCode{};
	};

	struct ProgramReflect final
	{
		std::vector<std::pair<std::string, uint32_t>> uniformBlocks;
		std::vector<std::pair<std::string, uint32_t>> storageBlocks;
		std::vector<std::pair<std::string, uint32_t>> samplersAndImages;
	};

	[[nodiscard]] std::string LoadShaderCode(const std::string& filename, const std::vector<std::string>& defines = {});

	[[nodiscard]] ShaderProgramPtr CreateShaderProgram(const GraphicsProgramCreateInfo& createInfo);
	[[nodiscard]] ShaderProgramPtr CreateShaderProgram(const ComputeProgramCreateInfo& createInfo);

	[[nodiscard]] uint32_t Handle(const ShaderProgramPtr& program) noexcept;
	[[nodiscard]] bool IsValid(const ShaderProgramPtr& program) noexcept;

	void BindShaderProgram(const ShaderProgramPtr& program);

	void BindFragDataLocation(const ShaderProgramPtr& program, const std::string& name, unsigned index);
	void BindAttributeLocation(const ShaderProgramPtr& program, const std::string& name, unsigned index);
	int GetFragDataLocation(const ShaderProgramPtr& program, const std::string& name);
	int GetFragDataIndex(const ShaderProgramPtr& program, const std::string& name);

	int GetAttributeLocation(const ShaderProgramPtr& program, const std::string& name);
	std::vector<int> GetAttributeLocations(const ShaderProgramPtr& program, const std::vector<std::string>& names);

	unsigned GetUniformBlockIndex(const ShaderProgramPtr& program, const std::string& name);
	void GetActiveUniforms(const ShaderProgramPtr& program, const int uniformCount, const unsigned* uniformIndices, const unsigned pname, int* params);
	std::vector<int> GetActiveUniforms(const ShaderProgramPtr& program, const std::vector<unsigned>& uniformIndices, const unsigned pname);
	std::vector<int> GetActiveUniforms(const ShaderProgramPtr& program, const std::vector<int>& uniformIndices, const unsigned pname);
	int GetActiveUniform(const ShaderProgramPtr& program, const unsigned uniformIndex, const unsigned pname);
	std::string GetActiveUniformName(const ShaderProgramPtr& program, const uint32_t uniformIndex);

	int GetUniformLocation(const ShaderProgramPtr& program, const std::string& name);
	std::vector<int> GetUniformLocations(const ShaderProgramPtr& program, const std::vector<std::string>& names);

	bool SetUniform(const ShaderProgramPtr& program, int location, float value);
	bool SetUniform(const ShaderProgramPtr& program, int location, int value);
	bool SetUniform(const ShaderProgramPtr& program, int location, unsigned int value);
	bool SetUniform(const ShaderProgramPtr& program, int location, bool value);
	bool SetUniform(const ShaderProgramPtr& program, int location, unsigned __int64 value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<float>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<int>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<unsigned int>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::vec2& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::vec3& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::vec4& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::ivec2& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::ivec3& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::ivec4& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::uvec2& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::uvec3& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::uvec4& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat2& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat3& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat4& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat2x3& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat3x2& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat2x4& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat4x2& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat3x4& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const glm::mat4x3& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::vec2>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::vec3>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::vec4>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::ivec2>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::ivec3>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::ivec4>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::uvec2>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::uvec3>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::uvec4>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat2>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat3>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat4>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat2x3>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat3x2>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat2x4>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat4x2>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat3x4>& value);
	bool SetUniform(const ShaderProgramPtr& program, int location, const std::vector<glm::mat4x3>& value);


	ProgramReflect ReflectProgram(const ShaderProgramPtr& program);

} // namespace gpu::program