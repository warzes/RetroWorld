#pragma once

#include "gpu_program.h"

namespace gpu::uniform
{
	template <typename T>
	struct Uniform final
	{
		Uniform() noexcept = default;
		Uniform(const Uniform&) = default;
		Uniform(const T& value) noexcept : value(value) {}

		Uniform& operator=(const Uniform&) = default;
		Uniform& operator=(const T& newValue) noexcept
		{
			value = newValue;
			return *this;
		}

		bool isValid() const { return location >= 0; }

		std::string name;
		T value = {};
		gpu::program::ShaderProgramPtr program;
		int location{ -1 };
	};

	template<typename T>
	bool InitUniform(Uniform<T>& uniform, gpu::program::ShaderProgramPtr program, const std::string& name)
	{
		uniform.name = name;
		uniform.program = program;
		uniform.location = gpu::program::GetUniformLocation(program, name);
		return uniform.isValid();
	}

	template<typename T>
	bool BindUniform(const Uniform<T>& uniform)
	{
		return gpu::program::Uniform(uniform.program, uniform.location, uniform.value);
	}

	template<typename T>
	bool BindUniform(Uniform<T>& uniform, const T& value)
	{
		uniform = value;
		return BindUniform(uniform);
	}

} // namespace gpu::uniform