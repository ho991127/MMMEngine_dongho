#pragma once
#include "ScriptBehaviour.h"

namespace MMMEngine {
	class Player : public ScriptBehaviour
	{
	public:
		void Initialize() {};
		void Update();
		float velocity = 1.0f;

		//피격 판정
		bool normalhit = false;
		bool bighit = false;
		float posX = 0.0f;
		float posY = 0.0f;
	private:
		int HP = 100;
		bool invincible = false;
		float invincibleTimer = 0.0f;
		float velocity = 200.0f;
	};
}