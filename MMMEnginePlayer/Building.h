#pragma once
#include "ScriptBehaviour.h"
#include "rttr/type"
#include "UserScriptsCommon.h"

namespace MMMEngine {
	class USERSCRIPTS Building : public ScriptBehaviour
	{
	private:
		RTTR_ENABLE(ScriptBehaviour)
		RTTR_REGISTRATION_FRIEND
		int HP = 50;
		int point = 0;
		int exp = 0;
		int atk = 10;
	public:
		Building()
		{
			REGISTER_BEHAVIOUR_MESSAGE(Start)
			REGISTER_BEHAVIOUR_MESSAGE(Update)
		}
		void Start();
		void Update();
		void GetDamage(int t) { HP - t; HP = std::max(HP, 0); };
		void PointUp(int t);
	};
}
