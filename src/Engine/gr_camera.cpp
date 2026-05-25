#include "stdafx.h"
#include "gr_camera.h"
//=============================================================================
gr::Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp)
{
	m_cameraPosition = cameraPosition;
	m_cameraTarget = cameraTarget;
	m_cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
	m_cameraRightDirection = glm::normalize(glm::cross(m_cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	m_cameraUpDirection = cameraUp;
	m_yaw = glm::degrees(glm::atan2(m_cameraFrontDirection.x, -m_cameraFrontDirection.z));
	m_pitch = glm::degrees(asin(m_cameraFrontDirection.y));
}
//=============================================================================
void gr::Camera::Move(Movement direction, float speed)
{
	switch (direction)
	{
	case Movement::Forward:
		m_cameraPosition += m_cameraFrontDirection * speed;
		break;
	case Movement::Backward:
		m_cameraPosition -= m_cameraFrontDirection * speed;
		break;
	case Movement::Right:
		m_cameraPosition += m_cameraRightDirection * speed;
		break;
	case Movement::Left:
		m_cameraPosition -= m_cameraRightDirection * speed;
		break;
	case Movement::Up:
		m_cameraPosition += m_cameraUpDirection * speed;
		break;
	case Movement::Down:
		m_cameraPosition -= m_cameraUpDirection * speed;
		break;
	}
	m_cameraTarget = m_cameraPosition + m_cameraFrontDirection;
}
//=============================================================================
void gr::Camera::Rotate(float pitchDelta, float yawDelta, float rollDelta)
{
	m_yaw += yawDelta;
	m_pitch += pitchDelta;

	if (m_pitch > 89.0f) m_pitch = 89.0f;
	if (m_pitch < -89.0f) m_pitch = -89.0f;

	glm::vec3 direction;
	direction.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	direction.y = sin(glm::radians(m_pitch));
	direction.z = cos(glm::radians(m_yaw)) * -cos(glm::radians(m_pitch));
	m_cameraFrontDirection = glm::normalize(direction);

	if (rollDelta != 0.0f)
	{
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rollDelta), m_cameraFrontDirection);
		m_cameraUpDirection = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(m_cameraUpDirection, 0.0f)));
		m_cameraRightDirection = glm::normalize(glm::cross(m_cameraFrontDirection, m_cameraUpDirection));
	}
	else
	{
		m_cameraRightDirection = glm::normalize(glm::cross(m_cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
		m_cameraUpDirection = glm::normalize(glm::cross(m_cameraRightDirection, m_cameraFrontDirection));
	}

	m_cameraTarget = m_cameraPosition + m_cameraFrontDirection;
}
//=============================================================================
glm::mat4 gr::Camera::GetViewMatrix()
{
	return glm::lookAt(m_cameraPosition, m_cameraTarget, m_cameraUpDirection);
}
//=============================================================================