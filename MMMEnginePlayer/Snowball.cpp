#include "Snowball.h"
#include "MMMTime.h"
#include "Player.h"
#include "Transform.h"
#include "rttr/registration"
#include "rttr/detail/policies/ctor_policies.h"

RTTR_PLUGIN_REGISTRATION
{
	using namespace rttr;
	using namespace MMMEngine;

	registration::class_<Player>("Snowball")
		(rttr::metadata("wrapper_type", rttr::type::get<ObjPtr<Snowball>>()));

	registration::class_<ObjPtr<Snowball>>("ObjPtr<Snowball>")
		.constructor(
			[]() {
				return Object::NewObject<Snowball>();
			});

	type::register_wrapper_converter_for_base_classes<MMMEngine::ObjPtr<Snowball>>();
}

void Start()
{

}

void MMMEngine::Snowball::Update()
{
	if (IsCarried())
	{
		pos = GetTransform()->GetWorldPosition();
		auto trPlayer = carrier->GetTransform();
		playerpos = trPlayer->GetWorldPosition();
		playerrot = trPlayer->GetWorldRotation();
		RollSnow();
	}

	DirectX::SimpleMath::Vector3 scaleVector = { scale, scale, scale };
	GetTransform()->SetWorldScale(scaleVector);
}

void MMMEngine::Snowball::RollSnow()
{
	scale = std::min(scale + scaleup * point, 4.0f);
	auto fwd = DirectX::SimpleMath::Vector3::Transform(
		DirectX::SimpleMath::Vector3::Forward, playerrot);
	fwd.y = 0.0f;
	if (fwd.LengthSquared() > 1e-8f) fwd.Normalize();
	auto target = playerpos + fwd * offset;
	auto to = target - pos;
	to.y = 0.0f;

	float dist = to.Length();
	if (dist > 1e-3f)
	{
		auto dir = to / dist;
		float step = velocity * Time::GetDeltaTime();
		if (step >= dist) pos = target;
		else pos += dir * step;

		GetTransform()->SetWorldPosition(pos);
	}
}

void MMMEngine::Snowball::EatSnow(ObjPtr<GameObject> other)
{
	if (!other || other == GetGameObject()) return;
	auto snowcomp = other->GetComponent<Snowball>();
	if (snowcomp == nullptr)
		return;
	if (snowcomp->scale > maxscale - scale)
		scale = maxscale;
	else
		scale += snowcomp->scale;
	Destroy(other);
}
