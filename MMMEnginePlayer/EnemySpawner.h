#pragma once
#include "ScriptBehaviour.h"
#include "UserScriptsCommon.h"
#include "rttr/type"

namespace MMMEngine {
	class USERSCRIPTS EnemySpawner : public ScriptBehaviour
	{
		RTTR_ENABLE(ScriptBehaviour)
		RTTR_REGISTRATION_FRIEND
	public:
		EnemySpawner()
		{
			REGISTER_BEHAVIOUR_MESSAGE(Start)
			REGISTER_BEHAVIOUR_MESSAGE(Update)
		}
		void Initialize() override;
		void UnInitialize() override;
		void Start();
		void Update();
		void MakeNormalEnemy();
		static ObjPtr<EnemySpawner> instance;
	private:
		float EnemySpawnTimer = 10.0f;
		std::vector<ObjPtr<GameObject>> Enemys;
	};
}
