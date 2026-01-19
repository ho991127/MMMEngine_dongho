#include "Player.h"
#include "MMMInput.h"
#include "MMMTime.h"

void MMMEngine::Player::Update()
{
	if (Input::GetKey(KeyCode::LeftArrow))
	{
		posX -= velocity * Time::GetDeltaTime();
	}
	if (Input::GetKey(KeyCode::RightArrow))
	{
		posX += velocity * Time::GetDeltaTime();
	}
	if (Input::GetKey(KeyCode::UpArrow))
	{
		posY += velocity * Time::GetDeltaTime();
	}
	if (Input::GetKey(KeyCode::DownArrow))
	{
		posY -= velocity * Time::GetDeltaTime();
	}
	if (!invincible)
	{
		if (normalhit)
		{
			HP -= 5;
			invincible = true;
			invincibleTimer = 1.5f;
			normalhit = false;
		}
		if (bighit)
		{
			HP -= 10;
			invincible = true;
			invincibleTimer = 1.5f;
			bighit = false;
		}
	}
	else
	{
		if (invincibleTimer > 0.0f)
		{
			invincibleTimer -= Time::GetDeltaTime();
			if (invincibleTimer <= 0.0f)
			{
				invincibleTimer = 0.0f;
				invincible = false;
			}
		}
	}
}