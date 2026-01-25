#include "Castle.h"
#include "MMMTime.h"

void MMMEngine::Castle::Initialize()
{

}

void MMMEngine::Castle::UnInitialize()
{

}

void MMMEngine::Castle::Update()
{
	AutoHeal();
}

void MMMEngine::Castle::AutoHeal()
{
	if (prevHP > HP)
	{
		fighting = true;
		NonfightTimer = 0.0f;
	}
	prevHP = HP;
	if (fighting)
	{
		NonfightTimer += Time::GetDeltaTime();
		if (NonfightTimer >= NonfightDelay)
		{
			fighting = false;
			healTimer = 0.0f;
		}
	}
	else if (HP < maxHP)
	{
		healTimer += Time::GetDeltaTime();
		if (healTimer >= healDelay)
		{
			HP = std::min(HP + healHP, maxHP);
			healTimer = 0.0f;
		}
	}
}

void MMMEngine::Castle::PointUp(float t)
{
	auto p = static_cast<int>(t);
	point += p;
	exp += 10 * p;
}