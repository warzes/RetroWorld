#pragma once

namespace math
{

	class Transform final
	{
	public:
		// Compute local matrix from position/rotation/scale (always recomputes)
		glm::mat4 GetLocalMatrix()
		{
			glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 r = glm::mat4_cast(rotation);
			glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
			return t * r * s;
		}

		// Compute world matrix = local * parentWorld
		glm::mat4 GetWorldMatrix(const glm::mat4& parentWorld)
		{
			return parentWorld * GetLocalMatrix();
		}

		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
	};

} // namespace math