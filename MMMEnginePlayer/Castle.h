#pragma once
#include "ScriptBehaviour.h"
#include "rttr/type"
#include "UserScriptsCommon.h"

namespace MMMEngine {
	class USERSCRIPTS Castle : public ScriptBehaviour
	{
	private:
		RTTR_ENABLE(ScriptBehaviour)
		RTTR_REGISTRATION_FRIEND
		void AutoHeal();
		int HP = 300;
		int prevHP = 300;
		int maxHP = 300;
		int healHP = 10;
		int point = 0;
		int exp = 0;
		int atk = 10;
		bool fighting = false;
		float healTimer = 0.0f;
		float healDelay = 1.0f;
		float NonfightTimer = 0.0f;
		float NonfightDelay = 5.0f;
	public:
		Castle()
		{
			REGISTER_BEHAVIOUR_MESSAGE(Start)
			REGISTER_BEHAVIOUR_MESSAGE(Update)
		}
		void Start();
		void Update();
		void PointUp(int t);
		void GetDamage(int t) { HP -= t; HP = std::max(HP, 0); };
		bool CastleDeath() const { return HP <= 0; }
	};
}
