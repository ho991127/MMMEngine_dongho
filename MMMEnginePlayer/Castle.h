#pragma once
#include "ScriptBehaviour.h"

namespace MMMEngine {
	class Castle : public ScriptBehaviour
	{
	public:
		void Initialize() override;
		void UnInitialize() override;
		void Update();
		void PointUp(float t);
		void GetDamage(int t) { HP - t; HP = std::max(HP, 0); };
		bool CastleDeath() const { return HP <= 0; }
	private:
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
	};
}