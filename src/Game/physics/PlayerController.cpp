#include "stdafx.h"
#include "PlayerController.h"
#include "PhysicsSystem.h"
#include <app_input.h>
#include <app_keys.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Core/TempAllocator.h>
#include <cmath>

namespace
{
	JPH::Vec3 ToJolt(const glm::vec3& v)
	{
		return { v.x, v.y, v.z };
	}

	glm::vec3 ToGlm(JPH::Vec3Arg v)
	{
		return { v.GetX(), v.GetY(), v.GetZ() };
	}

	JPH::RVec3 ToRJolt(const glm::vec3& v)
	{
		return { v.x, v.y, v.z };
	}

	// Порог угла для определения стены (>= 70 градусов от горизонтали)
	constexpr float WALL_CLIMB_ANGLE_COS = 0.342f; // cos(70)
	constexpr float CLIMB_SPEED_UP = 3.0f;
	constexpr float CLIMB_MAX_HEIGHT = 3.0f;
	constexpr float CLIMB_MAX_STAMINA = 10.0f;
	constexpr float WALL_DETECT_DIST = 0.5f;
	constexpr float GRAVITY = -6.0f;
}

// ---- Construction / Destruction ----
PlayerController::PlayerController(PhysicsSystem* inPhysics) noexcept
	: m_physics(inPhysics)
{
}

PlayerController::~PlayerController()
{
	Destroy();
}

bool PlayerController::Create(float inYaw, float inPitch)
{
	m_yaw   = inYaw;
	m_pitch = inPitch;
	m_state = State::Falling;
	m_onGround = false;
	m_crouching = false;
	m_climbStamina = CLIMB_MAX_STAMINA;
	m_climbing = false;

	recreateCharacter(m_playerHeight);
	return m_character != nullptr;
}

void PlayerController::Destroy()
{
	delete m_character;
	m_character = nullptr;
}

void PlayerController::recreateCharacter(float inHeight)
{
	delete m_character;
	m_character = nullptr;

	float halfHeight = JPH::max(0.1f, inHeight - m_playerRadius);
	if (halfHeight < 0.01f) halfHeight = 0.01f;

	JPH::CapsuleShapeSettings capsuleSettings(halfHeight, m_playerRadius);
	JPH::ShapeSettings::ShapeResult shapeResult = capsuleSettings.Create();
	if (shapeResult.HasError())
	{
		core::Error(std::string("PlayerController: Failed to create capsule: ") + shapeResult.GetError().c_str());
		return;
	}

	// Сдвинуть капсулу вверх так, чтобы низ был на y=0
	// Jolt капсула центрирована, её низ = -halfHeight - radius
	// Нам нужно: низ = 0 → offset = halfHeight + radius
	JPH::RotatedTranslatedShapeSettings offsetShape(
		JPH::Vec3(0, halfHeight + m_playerRadius, 0),
		JPH::Quat::sIdentity(),
		shapeResult.Get());

	JPH::ShapeSettings::ShapeResult offsetResult = offsetShape.Create();
	if (offsetResult.HasError())
	{
		core::Error(std::string("PlayerController: Failed to offset capsule: ") + offsetResult.GetError().c_str());
		return;
	}

	JPH::CharacterVirtualSettings settings;
	settings.mShape              = offsetResult.Get();
	settings.mMass               = 70.0f;
	settings.mMaxStrength        = 100.0f;
	settings.mBackFaceMode       = JPH::EBackFaceMode::CollideWithBackFaces;
	settings.mPredictiveContactDistance = 0.1f;
	settings.mMaxCollisionIterations    = 5;
	settings.mMaxConstraintIterations   = 15;
	settings.mMinTimeRemaining          = 1.0e-4f;
	settings.mCollisionTolerance        = 1.0e-3f;
	settings.mCharacterPadding          = 0.02f;
	settings.mMaxNumHits                = 256;
	settings.mHitReductionCosMaxAngle   = 0.999f;
	settings.mPenetrationRecoverySpeed  = 1.0f;
	settings.mMaxSlopeAngle        = JPH::DegreesToRadians(m_maxSlopeAngle);
	settings.mInnerBodyShape       = nullptr;
	settings.mInnerBodyLayer       = Layers::MOVING;

	auto* joltSystem = m_physics->GetJoltSystem();
	m_character = new JPH::CharacterVirtual(&settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, joltSystem);
	m_character->SetEnhancedInternalEdgeRemoval(true);
}

// ---- SetPosition ----
void PlayerController::SetPosition(const glm::vec3& inPos, float inYaw, float inPitch)
{
	m_yaw   = inYaw;
	m_pitch = inPitch;
	if (m_character)
	{
		m_character->SetPosition(ToRJolt(inPos));
	}
}

// ---- AddRotation ----
void PlayerController::AddRotation(float inDYaw, float inDPitch)
{
	m_yaw += inDYaw;
	m_pitch = glm::clamp(m_pitch + inDPitch, -89.0f, 89.0f);
}

// ---- GetLookDirection ----
glm::vec3 PlayerController::GetLookDirection() const noexcept
{
	float yawRad = glm::radians(m_yaw);
	float pitchRad = glm::radians(m_pitch);
	float sp = sinf(pitchRad);
	float cp = cosf(pitchRad);
	return glm::vec3(cp * sinf(yawRad), sp, -cp * cosf(yawRad));
}

// ---- Tick (FixedUpdate) ----
void PlayerController::Tick(float inDeltaTime)
{
	if (!m_character || !m_physics) return;

	auto* joltSystem = m_physics->GetJoltSystem();

	// ---- Input ----
	float xm = 0.0f, zm = 0.0f;
	if (input::IsKeyDown(KeyboardType::KEY_W)) zm += 1.0f;
	if (input::IsKeyDown(KeyboardType::KEY_S)) zm -= 1.0f;
	if (input::IsKeyDown(KeyboardType::KEY_A)) xm -= 1.0f;
	if (input::IsKeyDown(KeyboardType::KEY_D)) xm += 1.0f;

	// Normalise
	float len = sqrtf(xm * xm + zm * zm);
	if (len > 0.0f) { xm /= len; zm /= len; }
	if (zm < 0.0f) { xm *= 0.8f; zm *= 0.5f; }

	bool wantJump   = input::IsKeyDown(KeyboardType::KEY_SPACE);
	bool wantCrouch = input::IsKeyDown(KeyboardType::KEY_LEFT_CONTROL);
	bool wantSprint = input::IsKeyDown(KeyboardType::KEY_LEFT_SHIFT);
	bool wantClimb  = wantSprint && (len > 0.0f || wantJump);

	// ---- Camera-relative direction ----
	float yawRad = glm::radians(m_yaw);
	float forwardX = sinf(yawRad);
	float forwardZ = -cosf(yawRad);
	float rightX   = cosf(yawRad);
	float rightZ   = sinf(yawRad);

	float worldDX = forwardX * zm + rightX * xm;
	float worldDZ = forwardZ * zm + rightZ * xm;

	// ---- Crouch toggle ----
	if (wantCrouch && !m_crouching)
	{
		m_crouching = true;
		recreateCharacter(m_crouchHeight);
	}
	else if (!wantCrouch && m_crouching)
	{
		if (canStandUp())
		{
			m_crouching = false;
			recreateCharacter(m_playerHeight);
		}
		// else — остаёмся в присяде, места нет
	}

	// ---- Wall climb ----
	if (wantClimb && m_state != State::Climbing)
	{
		// Рейкаст вперёд для обнаружения стены
		JPH::RVec3 pos = m_character->GetPosition();
		JPH::Vec3 fwd = JPH::Vec3(worldDX, 0.0f, worldDZ).NormalizedOr(JPH::Vec3::sAxisX());
		if (len < 0.01f)
		{
			// Если нет движения, используем направление камеры
			fwd = JPH::Vec3(forwardX, 0.0f, forwardZ).NormalizedOr(JPH::Vec3::sAxisX());
		}

		JPH::RRayCast ray(pos, fwd * WALL_DETECT_DIST);
		JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
		JPH::RayCastSettings rcSettings;
		rcSettings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);
		joltSystem->GetNarrowPhaseQuery().CastRay(ray, rcSettings, collector);

		if (collector.HadHit())
		{
			JPH::RVec3 hitPos = ray.GetPointOnRay(collector.mHit.mFraction);
			JPH::Vec3 normal = hitPos - pos;
			float dist = normal.Length();
			if (dist > 0.01f) normal /= dist;

			// Проверяем угол: >= 70° от горизонтали
			if (fabsf(normal.GetY()) < WALL_CLIMB_ANGLE_COS)
			{
				m_state = State::Climbing;
				m_climbing = true;
				m_climbStamina = CLIMB_MAX_STAMINA;
				m_climbStartY = pos.GetY();
				m_climbNormal = ToGlm(normal);
			}
		}
	}

	// ---- Climbing state ----
	if (m_state == State::Climbing)
	{
		handleClimb(inDeltaTime);
		return; // ExtendedUpdate не вызываем во время climb — мы сами управляем позицией
	}

	// ---- Обычное движение ----
	bool sprinting = wantSprint && len > 0.0f && m_onGround;
	float speed = m_moveSpeed * (sprinting ? m_sprintMultiplier : 1.0f);
	if (m_crouching) speed *= 0.5f;

	// Гравитацию применяем всегда — ExtendedUpdate её не добавляет к скорости
	float vertVel = m_character->GetLinearVelocity().GetY();
	vertVel += GRAVITY * inDeltaTime;

	// Jump overrides gravity
	if (wantJump && m_onGround)
	{
		vertVel = m_jumpForce;
		m_state = State::Jumping;
	}
	else if (m_onGround)
	{
		// На земле: мягкое прижатие вместо полной гравитации
		// Предотвращает тряску и даёт плавный сход с уступа
		constexpr float GROUND_PUSH = -0.02f;
		vertVel = JPH::max(vertVel, GROUND_PUSH);
	}

	JPH::Vec3 desiredVel(worldDX * speed, vertVel, worldDZ * speed);

	m_character->SetLinearVelocity(desiredVel);

	// ---- ExtendedUpdate ----
	JPH::CharacterVirtual::ExtendedUpdateSettings euSettings{};
	euSettings.mStickToFloorStepDown = JPH::Vec3::sZero();
	euSettings.mWalkStairsStepUp = JPH::Vec3::sZero();

	JPH::TempAllocatorMalloc tempAlloc;
	m_character->ExtendedUpdate(
		inDeltaTime,
		JPH::Vec3(0, -9.81f, 0), // gravity
		euSettings,
		JPH::BroadPhaseLayerFilter(),
		JPH::ObjectLayerFilter(),
		JPH::BodyFilter(),
		JPH::ShapeFilter(),
		tempAlloc
	);

	// ---- Обновить состояние ----
	updateGroundState();

	// Stamina recovery on ground
	if (m_onGround)
	{
		m_climbStamina = JPH::min(CLIMB_MAX_STAMINA, m_climbStamina + inDeltaTime * 2.0f);
	}
}

// ---- updateGroundState ----
void PlayerController::updateGroundState()
{
	if (!m_character) return;

	auto groundState = m_character->GetGroundState();
	m_onGround = (groundState == JPH::CharacterVirtual::EGroundState::OnGround);

	if (m_onGround)
	{
		if (m_state == State::Jumping || m_state == State::Falling || m_state == State::Climbing)
		{
			m_state = State::Grounded;
		}
	}
	else
	{
		if (m_state == State::Grounded || m_state == State::Jumping)
		{
			// Если не на земле и не прыгаем вверх — падаем
			if (m_character->GetLinearVelocity().GetY() < 0.0f)
				m_state = State::Falling;
		}
	}

	// После прыжка с земли переключаемся в Falling когда скорость < 0
	if (m_state == State::Jumping && m_character->GetLinearVelocity().GetY() <= 0.0f)
	{
		m_state = State::Falling;
	}
}

// ---- handleClimb ----
void PlayerController::handleClimb(float inDeltaTime)
{
	if (!m_character) return;

	bool wantClimb = input::IsKeyDown(KeyboardType::KEY_LEFT_SHIFT);
	if (!wantClimb)
	{
		// Отпустили Shift — падаем
		m_state = State::Falling;
		m_climbing = false;
		return;
	}

	JPH::RVec3 pos = m_character->GetPosition();
	float climbHeight = pos.GetY() - m_climbStartY;

	// Проверка: превышена максимальная высота
	if (climbHeight >= CLIMB_MAX_HEIGHT)
	{
		m_state = State::Falling;
		m_climbing = false;
		return;
	}

	// Stamina
	m_climbStamina -= inDeltaTime;
	if (m_climbStamina <= 0.0f)
	{
		m_state = State::Falling;
		m_climbing = false;
		return;
	}

	// Движение вверх + вдоль стены
	float xm = 0.0f, zm = 0.0f;
	if (input::IsKeyDown(KeyboardType::KEY_A)) xm -= 1.0f;
	if (input::IsKeyDown(KeyboardType::KEY_D)) xm += 1.0f;

	float len = sqrtf(xm * xm + zm * zm);
	if (len > 0.0f) { xm /= len; zm /= len; }

	// Касательное направление вдоль стены (горизонтальная проекция нормали стены)
	glm::vec3 tangent(0.0f);
	if (fabsf(m_climbNormal.y) < 0.999f)
	{
		glm::vec3 up(0, 1, 0);
		tangent = glm::cross(up, m_climbNormal);
		tangent = glm::normalize(tangent);
	}

	glm::vec3 moveDir = tangent * (xm * 2.0f) + glm::vec3(0, CLIMB_SPEED_UP, 0);
	JPH::Vec3 vel = ToJolt(moveDir);

	m_character->SetLinearVelocity(vel);

	// Вручную обновляем позицию (ExtendedUpdate не вызываем — climbs не поддерживает)
	JPH::Vec3 newPos = pos + vel * inDeltaTime;
	m_character->SetPosition(JPH::RVec3(newPos));

	// Mantle check: рейкаст вверх + вперёд от головы
	{
		JPH::Vec3 headPos = newPos + JPH::Vec3(0, m_playerHeight * 1.0f, 0);
		JPH::Vec3 mantleDir = JPH::Vec3(m_climbNormal.x, 1.0f, m_climbNormal.z).Normalized();
		JPH::RRayCast mantleRay(JPH::RVec3(headPos), mantleDir * 0.6f);
		JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> mantleCollector;
		auto* joltSystem = m_physics->GetJoltSystem();

		JPH::RayCastSettings mantleSettings;
		mantleSettings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);
		joltSystem->GetNarrowPhaseQuery().CastRay(mantleRay, mantleSettings, mantleCollector);

		if (!mantleCollector.HadHit())
		{
			// Путь наверх свободен — перелезаем
			glm::vec3 groundPos = ToGlm(newPos) + ToGlm(mantleDir * 1.0f);
			groundPos.y += 0.5f; // немножко над полом
			m_character->SetPosition(ToRJolt(groundPos));
			m_state = State::Falling;
			m_climbing = false;
			m_onGround = false; // пусть гравитация посадит на пол
		}
	}
}

// ---- canStandUp ----
bool PlayerController::canStandUp() const
{
	if (!m_character || !m_physics) return false;

	JPH::RVec3 pos = m_character->GetPosition();
	float checkHeight = m_playerHeight - m_crouchHeight;
	if (checkHeight <= 0.0f) return true;

	JPH::Vec3 halfExtent(m_playerRadius, checkHeight * 0.5f, m_playerRadius);
	JPH::Vec3 boxCenter(0, m_crouchHeight + checkHeight * 0.5f, 0);
	JPH::BoxShape box(halfExtent);

	JPH::CollideShapeSettings collideSettings;
	collideSettings.mMaxSeparationDistance = 0.0f;

	JPH::TempAllocatorMalloc allocator;
	JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;
	auto* joltSystem = m_physics->GetJoltSystem();

	joltSystem->GetNarrowPhaseQuery().CollideShape(
		&box, JPH::Vec3::sReplicate(1.0f),
		JPH::RMat44::sTranslation(pos + JPH::RVec3(0, boxCenter.GetY(), 0)),
		collideSettings,
		{},
		collector);

	return !collector.HadHit();
}

// ---- Getters ----
glm::vec3 PlayerController::GetPosition() const noexcept
{
	if (!m_character) return glm::vec3(0.0f);
	return ToGlm(m_character->GetPosition());
}
