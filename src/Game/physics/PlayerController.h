#pragma once

#include <glm/glm.hpp>

class PhysicsSystem;
namespace JPH { class CharacterVirtual; }

class PlayerController final
{
public:
	explicit PlayerController(PhysicsSystem* inPhysics) noexcept;
	~PlayerController();

	PlayerController(const PlayerController&) = delete;
	PlayerController& operator=(const PlayerController&) = delete;

	// Создать/удалить CharacterVirtual
	[[nodiscard]] bool Create(float inYaw, float inPitch);
	void               Destroy();

	// Основной тик (вызывать из FixedUpdate)
	void Tick(float inDeltaTime);

	// Установить позицию при спавне
	void SetPosition(const glm::vec3& inPos, float inYaw, float inPitch);

	// Управление вращением (мышка)
	void AddRotation(float inDYaw, float inDPitch);

	// Направление взгляда (для камеры)
	glm::vec3 GetLookDirection() const noexcept;

	// Getters
	glm::vec3         GetPosition()  const noexcept;
	float             GetYaw()       const noexcept { return m_yaw; }
	float             GetPitch()     const noexcept { return m_pitch; }
	bool              IsOnGround()   const noexcept { return m_onGround; }
	bool              IsClimbing()   const noexcept { return m_state == State::Climbing; }
	float             GetEyeHeight() const noexcept { return m_crouching ? m_crouchHeight * 0.5f : m_playerHeight * 0.9f; }
	bool              IsCrouching()  const noexcept { return m_crouching; }

	// Настраиваемые параметры
	float m_moveSpeed        = 5.0f;
	float m_sprintMultiplier = 1.5f;
	float m_jumpForce        = 8.0f;
	float m_playerHeight     = 0.34f;
	float m_playerRadius     = 0.16f;
	float m_crouchHeight     = 0.18f;
	float m_maxStepHeight    = 0.25f;
	float m_maxSlopeAngle    = 60.0f; // градусы

private:
	enum class State : uint8_t
	{
		Grounded,
		Jumping,
		Falling,
		Climbing
	};

	void recreateCharacter(float inHeight);
	void updateGroundState();
	void handleClimb(float inDeltaTime);
	bool canStandUp() const;

	PhysicsSystem*         m_physics   = nullptr;
	JPH::CharacterVirtual* m_character = nullptr;

	State   m_state         = State::Falling;
	float   m_yaw           = 0.0f;
	float   m_pitch         = 0.0f;
	bool    m_onGround      = false;
	bool    m_crouching     = false;

	// Climbing state
	bool    m_climbing      = false;
	float   m_climbStamina  = 10.0f;
	float   m_climbStartY   = 0.0f;
	glm::vec3 m_climbNormal{ 0.0f };
};
