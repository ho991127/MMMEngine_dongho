#pragma once
#include "Enemy.h"

namespace MMMEngine {
	class NormalEnemy : public Enemy
	{
	protected:
		void Configure() override
		{
			HP = 30;
			atk = 2;
			velocity = 13.0f;
			playercheckdist = 12.0f;
			battledist = 1.7f;
			attackDelay = 0.65f;
		}
	};
}
