#pragma once
#include "ScriptBehaviour.h"

namespace MMMEngine {
	class Building : public ScriptBehaviour
	{
	public:
		void Initialize() override;
		void UnInitialize() override;
		void Update();
		void GetDamage(int t) { HP - t; HP = std::max(HP, 0); };
		void PointUp(float t);
	private:
		int HP = 50;
		int point = 0;
		int exp = 0;
		int atk = 10;
	};
}
