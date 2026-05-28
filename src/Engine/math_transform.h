#pragma once

namespace math
{

	class Transform final
	{
	public:
		// Compute local matrix from position/rotation/scale (cached with dirty flag)
		glm::mat4 GetLocalMatrix()
		{
			if (m_dirty)
			{
				glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
				glm::mat4 r = glm::mat4_cast(rotation);
				glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
				m_cachedLocal = t * r * s;
				m_dirty = false;
			}
			return m_cachedLocal;
		}

		// Compute world matrix = local * parentWorld
		glm::mat4 GetWorldMatrix(const glm::mat4& parentWorld)
		{
			return parentWorld * GetLocalMatrix();
		}

		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f);

	private:
		glm::mat4 m_cachedLocal = glm::mat4(1.0f);
		bool      m_dirty = true;
	};

} // namespace math