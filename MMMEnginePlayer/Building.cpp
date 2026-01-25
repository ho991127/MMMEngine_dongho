#include "Building.h"

void MMMEngine::Building::Initialize()
{

}

void MMMEngine::Building::UnInitialize()
{

}

void MMMEngine::Building::Update()
{
	if (HP <= 0)
		Destroy(GetGameObject());
}

void MMMEngine::Building::PointUp(float t)
{
	auto p = static_cast<int>(t);
	point += p;
	exp += 10 * p;
}