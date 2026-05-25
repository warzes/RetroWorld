#pragma once

namespace gpu::program
{
	struct ShaderProgram;
	using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;

	std::string LoadShaderCode(const std::string& filename, const std::vector<std::string>& defines = {});

	ShaderProgramPtr LoadShaderProgram(const std::string& vsFile, const std::vector<std::string>& defines = {});
	ShaderProgramPtr LoadShaderProgram(const std::string& vsFile, const std::string& fsFile, const std::vector<std::string>& defines = {});
	ShaderProgramPtr LoadShaderProgram(const std::string& vsFile, const std::string& gsFile, const std::string& fsFile, const std::vector<std::string>& defines = {});

	ShaderProgramPtr CreateShaderProgram(const std::string& vertexShaderSrc);
	ShaderProgramPtr CreateShaderProgram(const std::string& vertexShaderSrc, const std::string& fragmentShaderSrc);
	ShaderProgramPtr CreateShaderProgram(const std::string& vertexShaderSrc, const std::string& geometryShaderSrc, const std::string& fragmentShaderSrc);

	void BindShaderProgram(ShaderProgramPtr program);

	void BindFragDataLocation(ShaderProgramPtr program, const std::string& name, unsigned index);
	void BindAttributeLocation(ShaderProgramPtr program, const std::string& name, unsigned index);
	int GetFragDataLocation(ShaderProgramPtr program, const std::string& name);
	int GetFragDataIndex(ShaderProgramPtr program, const std::string& name);

	int GetAttributeLocation(ShaderProgramPtr program, const std::string& name);
	std::vector<int> GetAttributeLocations(ShaderProgramPtr program, const std::vector<std::string>& names);

	unsigned GetUniformBlockIndex(ShaderProgramPtr program, const std::string& name);
	void GetActiveUniforms(ShaderProgramPtr program, const int uniformCount, const unsigned* uniformIndices, const unsigned pname, int* params);
	std::vector<int> GetActiveUniforms(ShaderProgramPtr program, const std::vector<unsigned>& uniformIndices, const unsigned pname);
	std::vector<int> GetActiveUniforms(ShaderProgramPtr program, const std::vector<int>& uniformIndices, const unsigned pname);
	int GetActiveUniform(ShaderProgramPtr program, const unsigned uniformIndex, const unsigned pname);
	std::string GetActiveUniformName(ShaderProgramPtr program, const GLuint uniformIndex);

	int GetUniformLocation(ShaderProgramPtr program, const std::string& name);
	std::vector<int> GetUniformLocations(ShaderProgramPtr program, const std::vector<std::string>& names);

	bool Uniform(ShaderProgramPtr program, int location, float value);
	bool Uniform(ShaderProgramPtr program, int location, int value);
	bool Uniform(ShaderProgramPtr program, int location, unsigned int value);
	bool Uniform(ShaderProgramPtr program, int location, bool value);
	bool Uniform(ShaderProgramPtr program, int location, unsigned __int64 value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<float>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<int>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<unsigned int>& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::vec2& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::vec3& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::vec4& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::ivec2& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::ivec3& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::ivec4& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::uvec2& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::uvec3& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::uvec4& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat2& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat3& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat4& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat2x3& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat3x2& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat2x4& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat4x2& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat3x4& value);
	bool Uniform(ShaderProgramPtr program, int location, const glm::mat4x3& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::vec2>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::vec3>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::vec4>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::ivec2>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::ivec3>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::ivec4>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::uvec2>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::uvec3>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::uvec4>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat2>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat3>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat4>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat2x3>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat3x2>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat2x4>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat4x2>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat3x4>& value);
	bool Uniform(ShaderProgramPtr program, int location, const std::vector<glm::mat4x3>& value);

} // namespace gpu::program