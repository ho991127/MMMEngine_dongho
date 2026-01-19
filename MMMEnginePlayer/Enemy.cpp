#include "Enemy.h"
#include "MMMTime.h"

void MMMEngine::Enemy::Update()
{
	auto player = GameObject::Find("player");
	
	if (!playerFind)
	{
		float targetX = 0.0f;
		float targetY = 0.0f;

		float dx = targetX - posX;
		float dy = targetY - posY;

		float dist = std::sqrt(dx * dx + dy * dy);
		if (dist > 0.01f)
		{
			dx /= dist;
			dy /= dist;

			float dt = Time::GetDeltaTime();
			posX += dx * velocity * dt;
			posY += dy * velocity * dt;
		}
		else
		{
			posX = targetX;
			posY = targetY;
		}

		float playerX = player->GetComponent<Player>()->posX;
		float playerY = player->GetComponent<Player>()->posY;
		float forwardX = cosf(forwardAngle);
		float forwardY = sinf(forwardAngle);

		// Enemy -> Player
		float dx = playerX - posX;
		float dy = playerY - posY;

		float dist = sqrtf(dx * dx + dy * dy);
		if (dist <= 0.0001f)
			return;

		dx /= dist;
		dy /= dist;

		float dot = forwardX * dx + forwardY * dy;

		if (dot >= 0.5f || dist<100.0f)
		{
			playerFind = true;
		}
	}
}