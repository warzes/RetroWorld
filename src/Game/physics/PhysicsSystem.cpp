#include "stdafx.h"
#include "PhysicsSystem.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Core/StreamWrapper.h>
#include <cassert>

// ---- Jolt глобальная инициализация (callbacks) ----
namespace
{
	// Callback для ассертов Jolt
	[[maybe_unused]] bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
	{
		std::string msg = "Jolt Assert: ";
		msg += inExpression ? inExpression : "";
		msg += " ";
		msg += inMessage ? inMessage : "";
		msg += " (";
		msg += inFile ? inFile : "";
		msg += ":";
		msg += std::to_string(inLine);
		msg += ")";
		core::Error(msg);
		return true;
	}

	// Callback для трейса Jolt
	[[maybe_unused]] void TraceImpl(const char* inFMT, ...)
	{
		va_list list;
		va_start(list, inFMT);
		char buf[1024];
		vsnprintf(buf, sizeof(buf), inFMT, list);
		va_end(list);
		core::Info(std::string("[Jolt] ") + buf);
	}
}

int PhysicsSystem::s_joltRefCount = 0;

// ---- Init / Close ----
bool PhysicsSystem::Init(JPH::uint inMaxBodies, JPH::uint inMaxBodyPairs, JPH::uint inMaxContactConstraints)
{
	// Однократная глобальная инициализация Jolt
	if (s_joltRefCount == 0)
	{
		JPH::RegisterDefaultAllocator();

		// Трейсинг и ассерты
		JPH::Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
	}
	++s_joltRefCount;

	// BroadPhase layer mapping
	m_bpInterface.MapObjectToBroadPhaseLayer(Layers::NON_MOVING, BroadPhaseLayers::NON_MOVING);
	m_bpInterface.MapObjectToBroadPhaseLayer(Layers::MOVING,     BroadPhaseLayers::MOVING);

	// Object vs Object filter: enable MOVING-MOVING, enable MOVING-NON_MOVING; NON_MOVING-NON_MOVING stays disabled
	m_objectFilter.EnableCollision(Layers::NON_MOVING, Layers::MOVING);
	m_objectFilter.EnableCollision(Layers::MOVING, Layers::MOVING);

	// Object vs BroadPhase filter (constructed from object filter table)
	m_bpFilter = std::make_unique<JPH::ObjectVsBroadPhaseLayerFilterTable>(
		m_bpInterface,
		BroadPhaseLayers::NUM_LAYERS,
		m_objectFilter,
		Layers::NUM_LAYERS
	);

	// Аллокатор и JobSystem
	m_tempAllocator = new JPH::TempAllocatorImpl(16 * 1024 * 1024); // 16 MB
	m_jobSystem     = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

	// Инициализация PhysicsSystem
	m_system.Init(
		inMaxBodies,
		0, // num body mutexes (0 = auto)
		inMaxBodyPairs,
		inMaxContactConstraints,
		m_bpInterface,
		*m_bpFilter,
		m_objectFilter
	);

	m_running = false;
	return true;
}

PhysicsSystem::~PhysicsSystem()
{
	Close();
}

void PhysicsSystem::Close()
{
	RemoveAllBodies();

	m_system.OptimizeBroadPhase();

	delete m_tempAllocator;  m_tempAllocator = nullptr;
	delete m_jobSystem;      m_jobSystem = nullptr;

	--s_joltRefCount;
	if (s_joltRefCount == 0)
	{
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}

// ---- Update ----
void PhysicsSystem::Update(float inDeltaTime)
{
	if (!m_running) return;

	constexpr int cCollisionSteps = 1;
	m_system.Update(inDeltaTime, cCollisionSteps, m_tempAllocator, m_jobSystem);
}

// ---- RebuildMapCollider ----
void PhysicsSystem::RebuildMapCollider(
	std::span<const JPH::Float3> inVertices,
	std::span<const JPH::uint32> inIndices)
{
	auto& bodyInterface = m_system.GetBodyInterface();

	// Удалить старый коллайдер
	if (!m_mapBodyID.IsInvalid())
	{
		if (bodyInterface.IsAdded(m_mapBodyID))
			bodyInterface.RemoveBody(m_mapBodyID);
		bodyInterface.DestroyBody(m_mapBodyID);
		m_mapBodyID = JPH::BodyID();
	}

	if (inVertices.empty() || inIndices.empty()) return;

	// Создать IndexedTriangleList из индексов (6 индексов на quad = 2 треугольника)
	JPH::IndexedTriangleList triList;
	triList.reserve(inIndices.size() / 3);
	for (size_t i = 0; i < inIndices.size(); i += 3)
	{
		triList.emplace_back(
			inIndices[i],
			inIndices[i + 1],
			inIndices[i + 2],
			0 // material index
		);
	}

	// Создать MeshShape
	JPH::VertexList vertexList;
	vertexList.assign(inVertices.begin(), inVertices.end());

	JPH::MeshShapeSettings meshSettings(vertexList, triList);
	meshSettings.Sanitize();

	JPH::ShapeSettings::ShapeResult shapeResult = meshSettings.Create();
	if (shapeResult.HasError())
	{
		core::Error(std::string("PhysicsSystem: Failed to create MeshShape: ") + shapeResult.GetError().c_str());
		return;
	}

	// Создать статическое тело
	JPH::BodyCreationSettings bodySettings(
		shapeResult.Get(),
		JPH::RVec3::sZero(),
		JPH::Quat::sIdentity(),
		JPH::EMotionType::Static,
		Layers::NON_MOVING
	);

	JPH::Body* body = bodyInterface.CreateBody(bodySettings);
	if (body == nullptr)
	{
		core::Error("PhysicsSystem: Failed to create map body");
		return;
	}

	m_mapBodyID = body->GetID();
	bodyInterface.AddBody(m_mapBodyID, JPH::EActivation::DontActivate);
}

// ---- RemoveAllBodies ----
void PhysicsSystem::RemoveAllBodies()
{
	auto& bodyInterface = m_system.GetBodyInterface();

	// Удалить карту
	if (!m_mapBodyID.IsInvalid())
	{
		if (bodyInterface.IsAdded(m_mapBodyID))
			bodyInterface.RemoveBody(m_mapBodyID);
		bodyInterface.DestroyBody(m_mapBodyID);
		m_mapBodyID = JPH::BodyID();
	}

	// Удалить все оставшиеся тела
	JPH::BodyIDVector bodies;
	m_system.GetBodies(bodies);
	for (const auto& id : bodies)
	{
		if (bodyInterface.IsAdded(id))
			bodyInterface.RemoveBody(id);
		bodyInterface.DestroyBody(id);
	}
}

// ---- BodyInterface access ----
JPH::BodyInterface& PhysicsSystem::GetBodyInterface() noexcept
{
	return m_system.GetBodyInterface();
}

const JPH::BodyInterface& PhysicsSystem::GetBodyInterface() const noexcept
{
	return m_system.GetBodyInterface();
}
