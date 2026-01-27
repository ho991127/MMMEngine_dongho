#pragma once
#include "ScriptBehaviour.h"
#include "SimpleMath.h"
#include "UserScriptsCommon.h"
#include "rttr/type"

namespace MMMEngine {
	class Transform;
	class Player;
	class USERSCRIPTS Snowball : public ScriptBehaviour
	{
		RTTR_ENABLE(ScriptBehaviour)
		RTTR_REGISTRATION_FRIEND
	public:
		Snowball()
		{
			REGISTER_BEHAVIOUR_MESSAGE(Start)
			REGISTER_BEHAVIOUR_MESSAGE(Update)
		}
		void Start();
		void Update();
		void EatSnow(ObjPtr<GameObject> other);
		float GetPoint() const { return point; };
		Player* carrier = nullptr;
		bool IsCarried() const { return carrier != nullptr; }
	private:
		void RollSnow();
		float scale = 0.3f;
		float scaleup = 0.2f;
		float maxscale = 10.0f;
		int point = 1;
		float offset = 1.5f; //눈과 플레이어간의 거리
		float velocity = 25.0f;
		DirectX::SimpleMath::Vector3 pos;
		DirectX::SimpleMath::Vector3 playerpos;
		DirectX::SimpleMath::Quaternion playerrot;
	};
}
