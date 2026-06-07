#pragma once

#include <vector>
#include <cassert>
#include <glm/glm.hpp>

#include "editor_math.hpp"

namespace ed
{
	// Represents a 3 dimensional array of tiles and provides functions for converting coordinates.
	template<class Cel>
	class Grid
	{
	public:
		Grid(size_t width, size_t height, size_t length, float spacing, const Cel& fill)
			: _width(width), _height(height), _length(length), _spacing(spacing)
		{
			_grid.resize(width * height * length, fill);
		}

		Grid(size_t width, size_t height, size_t length, float spacing)
			: Grid(width, height, length, spacing, Cel())
		{}

		Grid()
			: Grid(0, 0, 0, 0.0f)
		{}

		virtual ~Grid() = default;

		glm::vec3 WorldToGridPos(glm::vec3 worldPos) const
		{
			return glm::vec3(
				glm::floor(worldPos.x / _spacing),
				glm::floor(worldPos.y / _spacing),
				glm::floor(worldPos.z / _spacing));
		}

		glm::vec3 GridToWorldPos(glm::vec3 gridPos, bool center) const
		{
			if (center)
			{
				return glm::vec3(
					(gridPos.x * _spacing) + (_spacing * 0.5f),
					(gridPos.y * _spacing) + (_spacing * 0.5f),
					(gridPos.z * _spacing) + (_spacing * 0.5f));
			}
			return glm::vec3(
				gridPos.x * _spacing,
				gridPos.y * _spacing,
				gridPos.z * _spacing);
		}

		glm::vec3 SnapToCelCenter(glm::vec3 worldPos) const
		{
			worldPos.x = glm::floor(worldPos.x / _spacing) * _spacing + (_spacing * 0.5f);
			worldPos.y = glm::floor(worldPos.y / _spacing) * _spacing + (_spacing * 0.5f);
			worldPos.z = glm::floor(worldPos.z / _spacing) * _spacing + (_spacing * 0.5f);
			return worldPos;
		}

		size_t FlatIndex(int i, int j, int k) const
		{
			assert(i >= 0 && j >= 0 && k >= 0);
			assert((size_t)i < _width && (size_t)j < _height && (size_t)k < _length);
			return static_cast<size_t>(i) + (static_cast<size_t>(k) * _width) + (static_cast<size_t>(j) * _width * _length);
		}

		glm::vec3 UnflattenIndex(size_t idx) const
		{
			assert(idx < _grid.size());
			return glm::vec3(
				static_cast<float>(idx % _width),
				static_cast<float>(idx / (_width * _length)),
				static_cast<float>((idx / _width) % _length));
		}

		size_t GetWidth()  const { return _width; }
		size_t GetHeight() const { return _height; }
		size_t GetLength() const { return _length; }
		float  GetSpacing() const { return _spacing; }

		glm::vec3 GetMinCorner() const
		{
			return glm::vec3(0.0f);
		}

		glm::vec3 GetMaxCorner() const
		{
			return glm::vec3(
				static_cast<float>(_width) * _spacing,
				static_cast<float>(_height) * _spacing,
				static_cast<float>(_length) * _spacing);
		}

		glm::vec3 GetCenterPos() const
		{
			return glm::vec3(
				static_cast<float>(_width) * _spacing * 0.5f,
				static_cast<float>(_height) * _spacing * 0.5f,
				static_cast<float>(_length) * _spacing * 0.5f);
		}

	protected:
		void SetCel(int i, int j, int k, const Cel& cel)
		{
			if (i >= 0 && j >= 0 && k >= 0 &&
				static_cast<size_t>(i) < _width &&
				static_cast<size_t>(j) < _height &&
				static_cast<size_t>(k) < _length)
			{
				_grid[FlatIndex(i, j, k)] = cel;
			}
		}

		Cel GetCel(int i, int j, int k) const
		{
			if (i >= 0 && j >= 0 && k >= 0 &&
				static_cast<size_t>(i) < _width &&
				static_cast<size_t>(j) < _height &&
				static_cast<size_t>(k) < _length)
			{
				return _grid[FlatIndex(i, j, k)];
			}
			return Cel();
		}

		void CopyCels(int i, int j, int k, const Grid<Cel>& src)
		{
			if (i < 0 || j < 0 || k < 0 ||
				static_cast<size_t>(i) >= _width ||
				static_cast<size_t>(j) >= _height ||
				static_cast<size_t>(k) >= _length) return;

			int xEnd = Min(i + static_cast<int>(src._width), static_cast<int>(_width));
			int yEnd = Min(j + static_cast<int>(src._height), static_cast<int>(_height));
			int zEnd = Min(k + static_cast<int>(src._length), static_cast<int>(_length));

			for (int z = k; z < zEnd; ++z)
			{
				for (int y = j; y < yEnd; ++y)
				{
					size_t ourBase = FlatIndex(0, y, z);
					size_t theirBase = src.FlatIndex(0, y - j, z - k);
					for (int x = i; x < xEnd; ++x)
					{
						_grid[ourBase + static_cast<size_t>(x)] =
							src._grid[theirBase + static_cast<size_t>(x - i)];
					}
				}
			}
		}

		void SubsectionCopy(int i, int j, int k, int w, int h, int l, Grid<Cel>& out) const
		{
			for (int z = k; z < k + l; ++z)
			{
				for (int y = j; y < j + h; ++y)
				{
					size_t ourBase = FlatIndex(0, y, z);
					size_t theirBase = out.FlatIndex(0, y - j, z - k);
					for (int x = i; x < i + w; ++x)
					{
						out._grid[theirBase + static_cast<size_t>(x - i)] =
							_grid[ourBase + static_cast<size_t>(x)];
					}
				}
			}
		}

		std::vector<Cel> _grid;
		size_t _width = 0, _height = 0, _length = 0;
		float _spacing = 0.0f;
	};
} // namespace ed