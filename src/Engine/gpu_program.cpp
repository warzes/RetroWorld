#include "stdafx.h"
#include "gpu_program.h"
#include "_gpu_contextState.h"
#include "core_log.h"
//=============================================================================
std::string loadShaderCode(const std::string& path, unsigned int level);
//=============================================================================
inline std::string preprocessShaderCode(const std::string& line, const std::string& directory, unsigned int level)
{
	static const std::regex re("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
	std::smatch matches;

	if (regex_search(line, matches, re))
	{
		std::string path = matches[1].str();
		return loadShaderCode(directory + "/" + path, level);
	}
	else
	{
		return line;
	}
}
//=============================================================================
inline std::string loadShaderCode(const std::string& path, unsigned int level)
{
	core::Debug("Load Shader file: " + path);

	if (level > 32)
	{
		core::Error("Header inclusion depth limit reached, might be caused by cyclic header inclusion");
		return {};
	}

	std::string directory;
	size_t lastSlash = path.find_last_of("/\\"); // поддержка Windows
	if (lastSlash != std::string::npos)
		directory = path.substr(0, lastSlash);

	std::ifstream shaderFile(path);
	if (!shaderFile.is_open())
	{
		core::Error("Fail to open file: " + path);
		return {};
	}

	std::stringstream shaderStream;
	std::string line;
	while (std::getline(shaderFile, line))
	{
		shaderStream << preprocessShaderCode(line, directory, level + 1) << std::endl;
	}

	return shaderStream.str();
}
//=============================================================================
[[nodiscard]] inline std::string shaderStageToString(GLenum stage)
{
	switch (stage)
	{
	case GL_VERTEX_SHADER:          return "GL_VERTEX_SHADER";
	case GL_FRAGMENT_SHADER:        return "GL_FRAGMENT_SHADER";
	case GL_TESS_CONTROL_SHADER:    return "GL_TESS_CONTROL_SHADER";
	case GL_TESS_EVALUATION_SHADER: return "GL_TESS_EVALUATION_SHADER";
	case GL_COMPUTE_SHADER:         return "GL_COMPUTE_SHADER";
	default: std::unreachable();
	}
}
//=============================================================================
[[nodiscard]] inline std::string printShaderSource(const char* text)
{
	if (!text) return "";

	std::ostringstream oss;
	int line = 1;
	oss << "\n(" << std::setw(3) << std::setfill(' ') << line << "): ";

	while (*text)
	{
		if (*text == '\n')
		{
			oss << '\n';
			line++;
			oss << "(" << std::setw(3) << std::setfill(' ') << line << "): ";
		}
		else if (*text != '\r')
		{
			oss << *text;
		}
		text++;
	}
	return oss.str();
}
//=============================================================================
[[nodiscard]] inline GLuint compileShaderGLSL(GLenum stage, std::string_view sourceGLSL)
{
	if (sourceGLSL.empty())
	{
		core::Error("Failed to create OpenGL shader object for stage: " + shaderStageToString(stage) + ". Source code Empty.");
		return 0;
	}

	GLuint shader = glCreateShader(stage);
	if (!shader)
	{
		core::Error("Failed to create OpenGL shader object for stage: " + shaderStageToString(stage));
		return 0;
	}
	const GLchar* codeSrc = sourceGLSL.data();
	glShaderSource(shader, 1, &codeSrc, nullptr);
	glCompileShader(shader);

	GLint compileStatus{ 0 };
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE)
	{
		GLint maxLength{ 0 };
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		std::string infoLog;
		if (maxLength > 1)
		{
			infoLog.resize(static_cast<size_t>(maxLength - 1)); // исключаем \0
			glGetShaderInfoLog(shader, maxLength, nullptr, infoLog.data());
		}
		else
		{
			infoLog = "<no info log>";
		}

		std::string logError = "OPENGL " + shaderStageToString(stage) + ": Shader compilation failed: " + infoLog;
		if (codeSrc != nullptr) logError += ", Source: \n" + printShaderSource(codeSrc);
		core::Error(logError);
		return 0;
	}
	return shader;
}
//=============================================================================
struct ShaderHandle final
{
	ShaderHandle() noexcept = default;
	~ShaderHandle() { if (handle) glDeleteShader(handle); }
	GLuint get() const { return handle; }
	operator bool() const { return handle != 0; }
	GLuint handle = 0;
};
//=============================================================================
struct gpu::program::ShaderProgram final
{
	ShaderProgram() noexcept { programID = glCreateProgram(); }
	~ShaderProgram()
	{
		if (programID)
		{
			if (context.currentShaderProgram && context.currentShaderProgram->programID == programID)
				context.currentShaderProgram = nullptr;
			core::Debug("Destroy shader program " + std::to_string(programID));
			glDeleteProgram(programID);
		}
	}

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&&) noexcept = default;
	ShaderProgram& operator=(ShaderProgram&&) noexcept = default;

	void Bind()
	{
#if defined(_DEBUG)
		core::Debug("Bind shader program " + std::to_string(programID));
#endif
		glUseProgram(programID);
		MetricsCurrent.programBindings++;
	}

	[[nodiscard]] operator bool() const noexcept { return programID > 0; }
	[[nodiscard]] unsigned Handle() const noexcept { return programID; }
	[[nodiscard]] bool IsValid() const noexcept { return programID > 0; }

	unsigned programID{ 0 };
};
//=============================================================================
std::string gpu::program::LoadShaderCode(const std::string& path, const std::vector<std::string>& defines)
{
	core::Debug("Load Shader file: " + path);

	std::stringstream shaderStream;
	std::string directory = path.substr(0, path.find_last_of('/'));

	std::ifstream shaderFile(path);
	if (!shaderFile.is_open())
	{
		core::Error("Fail to open file: " + path);
		return {};
	}

	std::string line;
	unsigned int lineNumber = 0;
	while (std::getline(shaderFile, line))
	{
		if (lineNumber == 1) // вставляем defines после первой строки, чтобы #version был первым
		{
			for (auto itr = defines.begin(); itr != defines.end(); itr++)
			{
				shaderStream << "#define " << *itr << std::endl;
			}
		}

		shaderStream << preprocessShaderCode(line, directory, 1) << std::endl;
		lineNumber++;
	}

	return shaderStream.str();
}
gpu::program::ShaderProgramPtr gpu::program::CreateShaderProgram(const GraphicsProgramCreateInfo& createInfo)
{
	assert(!createInfo.vertexShaderCode.empty());
	ShaderHandle vertexShader;
	vertexShader.handle = compileShaderGLSL(GL_VERTEX_SHADER, createInfo.vertexShaderCode);
	if (!vertexShader) return nullptr;

	ShaderHandle fragmentShader;
	if (!createInfo.fragmentShaderCode.empty())
	{
		fragmentShader.handle = compileShaderGLSL(GL_FRAGMENT_SHADER, createInfo.fragmentShaderCode);
		if (!fragmentShader) return nullptr;
	}

	ShaderHandle tessControlShader;
	if (!createInfo.tessellationControlShaderCode.empty())
	{
		tessControlShader.handle = compileShaderGLSL(GL_TESS_CONTROL_SHADER, createInfo.tessellationControlShaderCode);
		if (!tessControlShader) return nullptr;
	}

	ShaderHandle tessEvalShader;
	if (!createInfo.tessellationEvaluationShaderCode.empty())
	{
		tessEvalShader.handle = compileShaderGLSL(GL_TESS_EVALUATION_SHADER, createInfo.tessellationEvaluationShaderCode);
		if (!tessEvalShader) return nullptr;
	}

	ShaderProgramPtr program = std::make_shared<ShaderProgram>();
	if (vertexShader)      glAttachShader(program->programID, vertexShader.get());
	if (fragmentShader)    glAttachShader(program->programID, fragmentShader.get());
	if (tessControlShader) glAttachShader(program->programID, tessControlShader.get());
	if (tessEvalShader)    glAttachShader(program->programID, tessEvalShader.get());
	glLinkProgram(program->programID);
	if (vertexShader)      glDetachShader(program->programID, vertexShader.get());
	if (fragmentShader)    glDetachShader(program->programID, fragmentShader.get());
	if (tessControlShader) glDetachShader(program->programID, tessControlShader.get());
	if (tessEvalShader)    glDetachShader(program->programID, tessEvalShader.get());

	GLint linkStatus;
	glGetProgramiv(program->programID, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE)
	{
		GLint maxLength;
		glGetProgramiv(program->programID, GL_INFO_LOG_LENGTH, &maxLength);
		std::string errorLog(maxLength, ' ');
		glGetProgramInfoLog(program->programID, maxLength, &maxLength, errorLog.data());
		core::Error(" Shader program failed\n" + errorLog);
		return nullptr;
	}

	if (!createInfo.name.empty())
		glObjectLabel(GL_PROGRAM, program->programID, static_cast<GLsizei>(createInfo.name.length()), createInfo.name.data());

	core::Debug("Create shader program " + std::to_string(program->programID));

	return program;
}
//=============================================================================
gpu::program::ShaderProgramPtr gpu::program::CreateShaderProgram(const ComputeProgramCreateInfo& createInfo)
{
	assert(!createInfo.shaderCode.empty());
	ShaderHandle shader;
	shader.handle = compileShaderGLSL(GL_COMPUTE_SHADER, createInfo.shaderCode);
	if (!shader) return nullptr;

	ShaderProgramPtr program = std::make_shared<ShaderProgram>();
	if (shader) glAttachShader(program->programID, shader.get());
	glLinkProgram(program->programID);
	if (shader) glDetachShader(program->programID, shader.get());

	GLint linkStatus;
	glGetProgramiv(program->programID, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE)
	{
		GLint maxLength;
		glGetProgramiv(program->programID, GL_INFO_LOG_LENGTH, &maxLength);
		std::string errorLog(maxLength, ' ');
		glGetProgramInfoLog(program->programID, maxLength, &maxLength, errorLog.data());
		core::Error(" Shader program failed\n" + errorLog);
		return nullptr;
	}

	if (!createInfo.name.empty())
		glObjectLabel(GL_PROGRAM, program->programID, static_cast<GLsizei>(createInfo.name.length()), createInfo.name.data());

	core::Debug("Create shader program " + std::to_string(program->programID));

	return program;
}
//=============================================================================
uint32_t gpu::program::Handle(ShaderProgramPtr program) noexcept
{
	return program ? program->Handle() : 0;
}
//=============================================================================
bool gpu::program::IsValid(ShaderProgramPtr program) noexcept
{
	return program ? program->IsValid() : false;
}
//=============================================================================
void gpu::program::BindShaderProgram(ShaderProgramPtr program)
{
	if (context.currentShaderProgram != program)
	{
		context.currentShaderProgram = program;
		program ? program->Bind() : glUseProgram(0);
	}
}
//=============================================================================
void gpu::program::BindFragDataLocation(ShaderProgramPtr program, const std::string& name, unsigned index)
{
	glBindFragDataLocation(program->Handle(), index, name.c_str());
}
//=============================================================================
void gpu::program::BindAttributeLocation(ShaderProgramPtr program, const std::string& name, unsigned index)
{
	glBindAttribLocation(program->Handle(), index, name.c_str());
}
//=============================================================================
int gpu::program::GetFragDataLocation(ShaderProgramPtr program, const std::string& name)
{
	return glGetFragDataLocation(program->Handle(), name.c_str());
}
//=============================================================================
int gpu::program::GetFragDataIndex(ShaderProgramPtr program, const std::string& name)
{
	return glGetFragDataIndex(program->Handle(), name.c_str());
}
//=============================================================================
int gpu::program::GetAttributeLocation(ShaderProgramPtr program, const std::string& name)
{
	if (!program || !program->IsValid())
	{
		core::Error("Invalid shader program");
		return -1;
	}
	return glGetAttribLocation(program->Handle(), name.c_str());
}
//=============================================================================
std::vector<int> gpu::program::GetAttributeLocations(ShaderProgramPtr program, const std::vector<std::string>& names)
{
	std::vector<GLint> locations;
	locations.reserve(names.size());

	for (const auto& name : names)
	{
		locations.push_back(GetAttributeLocation(program, name));
	}

	return locations;
}
//=============================================================================
unsigned gpu::program::GetUniformBlockIndex(ShaderProgramPtr program, const std::string& name)
{
	return glGetUniformBlockIndex(program->Handle(), name.c_str());
}
//=============================================================================
void gpu::program::GetActiveUniforms(ShaderProgramPtr program, const int uniformCount, const unsigned* uniformIndices, const unsigned pname, int* params)
{
	glGetActiveUniformsiv(program->Handle(), uniformCount, uniformIndices, pname, params);
}
//=============================================================================
std::vector<int> gpu::program::GetActiveUniforms(ShaderProgramPtr program, const std::vector<unsigned>& uniformIndices, const unsigned pname)
{
	std::vector<int> result(uniformIndices.size());
	GetActiveUniforms(program, static_cast<GLint>(uniformIndices.size()), uniformIndices.data(), pname, result.data());
	return result;
}
//=============================================================================
std::vector<int> gpu::program::GetActiveUniforms(ShaderProgramPtr program, const std::vector<int>& uniformIndices, const unsigned pname)
{
	std::vector<GLuint> indices(uniformIndices.size());
	for (unsigned i = 0; i < uniformIndices.size(); ++i)
		indices[i] = static_cast<GLuint>(uniformIndices[i]);
	return GetActiveUniforms(program, indices, pname);
}
//=============================================================================
int gpu::program::GetActiveUniform(ShaderProgramPtr program, const unsigned uniformIndex, const unsigned pname)
{
	GLint result = 0;
	GetActiveUniforms(program, 1, &uniformIndex, pname, &result);
	return result;
}
//=============================================================================
std::string gpu::program::GetActiveUniformName(ShaderProgramPtr program, const GLuint uniformIndex)
{
	GLint length = GetActiveUniform(program, uniformIndex, GL_UNIFORM_NAME_LENGTH);
	assert(length > 1); // Has to include at least 1 char and '\0'

	std::vector<char> name(length);
	glGetActiveUniformName(program->Handle(), uniformIndex, length, nullptr, name.data());

	// glGetActiveUniformName() insists we query '\0' as well, but it 
	// shouldn't be passed to std::string(), otherwise std::string::size()
	// returns <actual size> + 1 (on clang)
	auto numChars = length - 1;
	return std::string(name.data(), numChars);
}
//=============================================================================
int gpu::program::GetUniformLocation(ShaderProgramPtr program, const std::string& name)
{
	if (!program || !program->IsValid())
	{
		core::Error("Invalid shader program");
		return -1;
	}
	return glGetUniformLocation(program->Handle(), name.c_str());
}
//=============================================================================
std::vector<int> gpu::program::GetUniformLocations(ShaderProgramPtr program, const std::vector<std::string>& names)
{
	std::vector<GLint> locations;
	locations.reserve(names.size());

	for (const auto& name : names)
	{
		locations.push_back(GetUniformLocation(program, name));
	}

	return locations;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, float value)
{
	if (location >= 0)
	{
		glProgramUniform1f(program->Handle(), location, value);
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, int value)
{
	if (location >= 0)
	{
		glProgramUniform1i(program->Handle(), location, value);
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, unsigned int value)
{
	if (location >= 0)
	{
		glProgramUniform1ui(program->Handle(), location, value);
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, bool value)
{
	if (location >= 0)
	{
		glProgramUniform1i(program->Handle(), location, value ? 1 : 0);
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, unsigned __int64 value)
{
	if (location >= 0)
	{
		glProgramUniformHandleui64ARB(program->Handle(), location, value);
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<float>& value)
{
	if (location >= 0)
	{
		glProgramUniform1fv(program->Handle(), location, static_cast<GLint>(value.size()), value.data());
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<int>& value)
{
	if (location >= 0)
	{
		glProgramUniform1iv(program->Handle(), location, static_cast<GLint>(value.size()), value.data());
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<unsigned int>& value)
{
	if (location >= 0)
	{
		glProgramUniform1uiv(program->Handle(), location, static_cast<GLint>(value.size()), value.data());
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::vec2& value)
{
	if (location >= 0)
	{
		glProgramUniform2fv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::vec3& value)
{
	if (location >= 0)
	{
		glProgramUniform3fv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::vec4& value)
{
	if (location >= 0)
	{
		glProgramUniform4fv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::ivec2& value)
{
	if (location >= 0)
	{
		glProgramUniform2iv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::ivec3& value)
{
	if (location >= 0)
	{
		glProgramUniform3iv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::ivec4& value)
{
	if (location >= 0)
	{
		glProgramUniform4iv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::uvec2& value)
{
	if (location >= 0)
	{
		glProgramUniform2uiv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::uvec3& value)
{
	if (location >= 0)
	{
		glProgramUniform3uiv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::uvec4& value)
{
	if (location >= 0)
	{
		glProgramUniform4uiv(program->Handle(), location, 1, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat2& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix2fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat3& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix3fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat4& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix4fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat2x3& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix2x3fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat3x2& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix3x2fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat2x4& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix2x4fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat4x2& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix4x2fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat3x4& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix3x4fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const glm::mat4x3& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix4x3fv(program->Handle(), location, 1, GL_FALSE, glm::value_ptr(value));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::vec2>& value)
{
	if (location >= 0)
	{
		glProgramUniform2fv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::vec3>& value)
{
	if (location >= 0)
	{
		glProgramUniform3fv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::vec4>& value)
{
	if (location >= 0)
	{
		glProgramUniform4fv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::ivec2>& value)
{
	if (location >= 0)
	{
		glProgramUniform2iv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const int*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::ivec3>& value)
{
	if (location >= 0)
	{
		glProgramUniform3iv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const int*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::ivec4>& value)
{
	if (location >= 0)
	{
		glProgramUniform4iv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const int*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::uvec2>& value)
{
	if (location >= 0)
	{
		glProgramUniform2uiv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const unsigned*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::uvec3>& value)
{
	if (location >= 0)
	{
		glProgramUniform3uiv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const unsigned*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::uvec4>& value)
{
	if (location >= 0)
	{
		glProgramUniform4uiv(program->Handle(), location, static_cast<GLint>(value.size()), reinterpret_cast<const unsigned*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat2>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix2fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat3>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix3fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat4>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix4fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat2x3>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix2x3fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat3x2>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix3x2fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat2x4>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix2x4fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat4x2>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix4x2fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat3x4>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix3x4fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================
bool gpu::program::SetUniform(ShaderProgramPtr program, int location, const std::vector<glm::mat4x3>& value)
{
	if (location >= 0)
	{
		glProgramUniformMatrix4x3fv(program->Handle(), location, static_cast<GLint>(value.size()), GL_FALSE, reinterpret_cast<const float*>(value.data()));
		return true;
	}
	return false;
}
//=============================================================================