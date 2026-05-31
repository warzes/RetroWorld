#pragma once

#include <memory>
#include <span>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>

// Слои коллизий
namespace Layers
{
	constexpr JPH::ObjectLayer NON_MOVING = 0;
	constexpr JPH::ObjectLayer MOVING     = 1;
	constexpr JPH::uint        NUM_LAYERS = 2;
};

// BroadPhase слои (для broad-phase оптимизации)
namespace BroadPhaseLayers
{
	constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	constexpr JPH::BroadPhaseLayer MOVING(1);
	constexpr JPH::uint            NUM_LAYERS = 2;
};

class PhysicsSystem final
{
public:
	PhysicsSystem() = default;
	~PhysicsSystem();

	PhysicsSystem(const PhysicsSystem&) = delete;
	PhysicsSystem& operator=(const PhysicsSystem&) = delete;

	// Инициализация Jolt + PhysicsSystem
	// @param inMaxBodies — макс. количество тел (карта + игрок + ...)
	// @param inMaxBodyPairs — макс. пар тел для broad-phase
	// @param inMaxContactConstraints — макс. контактных ограничений
	[[nodiscard]] bool Init(
		JPH::uint inMaxBodies = 65536,
		JPH::uint inMaxBodyPairs = 65536,
		JPH::uint inMaxContactConstraints = 65536);

	void Close();

	// Вкл/выкл симуляции (при входе/выходе из game mode)
	void StartSimulation() noexcept { m_running = true; }
	void StopSimulation()  noexcept { m_running = false; }

	// Шаг физики (вызывать из FixedUpdate)
	void Update(float inDeltaTime);

	// Создать/пересоздать коллайдер карты из данных TileMeshGen
	// inPositions — вершины, inIndices — индексы (uint32)
	void RebuildMapCollider(
		std::span<const JPH::Float3> inVertices,
		std::span<const JPH::uint32> inIndices);

	// Удалить все динамические тела (при выходе в редактор)
	void RemoveAllBodies();

	// Debug draw
	void SetDebugDrawEnabled(bool enabled) noexcept { m_debugDraw = enabled; }
	bool IsDebugDrawEnabled() const noexcept { return m_debugDraw; }

	// Доступ к Jolt-объектам
	JPH::PhysicsSystem* GetJoltSystem() noexcept { return &m_system; }
	const JPH::PhysicsSystem* GetJoltSystem() const noexcept { return &m_system; }
	JPH::BodyInterface& GetBodyInterface() noexcept;
	const JPH::BodyInterface& GetBodyInterface() const noexcept;

private:
	// Jolt глобальная инициализация (однократно на весь процесс)
	static int s_joltRefCount;

	JPH::TempAllocatorImpl*     m_tempAllocator  = nullptr;
	JPH::JobSystemThreadPool*   m_jobSystem      = nullptr;
	JPH::PhysicsSystem          m_system;
	JPH::BodyID                 m_mapBodyID;

	// Layer фильтры (должны жить всё время жизни PhysicsSystem)
	JPH::BroadPhaseLayerInterfaceTable  m_bpInterface{ Layers::NUM_LAYERS, BroadPhaseLayers::NUM_LAYERS };
	std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> m_bpFilter;
	JPH::ObjectLayerPairFilterTable     m_objectFilter{ Layers::NUM_LAYERS };

	bool m_running    = false;
	bool m_debugDraw  = false;
};
