#pragma once

namespace math
{
	struct AABB final
	{
		AABB() = default;
		AABB(const glm::vec3& min_, const glm::vec3& max_) : min(min_), max(max_) {}

		void Reset()
		{
			min = glm::vec3(std::numeric_limits<float>::max());
			max = glm::vec3(-std::numeric_limits<float>::max());
		}

		// Expand to include a point
		void Expand(const glm::vec3& point)
		{
			min.x = std::min(min.x, point.x);
			min.y = std::min(min.y, point.y);
			min.z = std::min(min.z, point.z);
			max.x = std::max(max.x, point.x);
			max.y = std::max(max.y, point.y);
			max.z = std::max(max.z, point.z);
		}

		// Expand to include another AABB
		void Expand(const AABB& other)
		{
			Expand(other.min);
			Expand(other.max);
		}

		bool IsValid() const
		{
			return min.x <= max.x && min.y <= max.y && min.z <= max.z;
		}

		glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
		glm::vec3 GetExtents() const { return (max - min) * 0.5f; }

		// Transform all 8 corners and compute new AABB in world space
		AABB Transform(const glm::mat4& m) const
		{
			const glm::vec3 corners[] = {
				glm::vec3(min.x, min.y, min.z),
				glm::vec3(max.x, min.y, min.z),
				glm::vec3(min.x, max.y, min.z),
				glm::vec3(max.x, max.y, min.z),
				glm::vec3(min.x, min.y, max.z),
				glm::vec3(max.x, min.y, max.z),
				glm::vec3(min.x, max.y, max.z),
				glm::vec3(max.x, max.y, max.z)
			};

			AABB result;
			result.Reset();
			for (int i = 0; i < 8; ++i)
			{
				glm::vec4 tp = m * glm::vec4(corners[i], 1.0f);
				result.Expand(glm::vec3(tp.x, tp.y, tp.z));
			}
			return result;
		}

		bool Contains(const glm::vec3& point) const
		{
			return point.x >= min.x && point.x <= max.x &&
				point.y >= min.y && point.y <= max.y &&
				point.z >= min.z && point.z <= max.z;
		}

		// Check intersection with another AABB
		bool Intersects(const AABB& other) const
		{
			return 
				(min.x <= other.max.x && max.x >= other.min.x) &&
				(min.y <= other.max.y && max.y >= other.min.y) &&
				(min.z <= other.max.z && max.z >= other.min.z);
		}

		glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());
	};

} // namespace math