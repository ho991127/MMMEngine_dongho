#pragma once
#include "ScriptBehaviour.h"
#include <vector>
#include <unordered_map>
#include <SimpleMath.h>
#include "rttr/type"
#include "UserScriptsCommon.h"

namespace MMMEngine {
	class Player;
	class Castle;
	class Transform;
	class USERSCRIPTS SnowballManager : public ScriptBehaviour
	{
	private:
		RTTR_ENABLE(ScriptBehaviour)
		RTTR_REGISTRATION_FRIEND
		void RemoveFromList(ObjPtr<GameObject> obj);
		void AssembleSnow();
		void SnowToCastle();
		void SnowToBuilding();
		struct ScoopState
		{
			bool active = false;
			float holdTime = 0.0f;
		};

		std::unordered_map<Player*, ScoopState> scoopStates;
		float snowSpawnDelay = 0.2f;
		float spawnOffset = 1.5f;

		ObjPtr<GameObject> castle;
		ObjPtr<Castle> castlecomp;
		ObjPtr<Transform> castletr;
		DirectX::SimpleMath::Vector3 castlepos;

		float CoinupRange = 1.0f; //성에 눈이 들어가는 거리
		float offset = 1.5f;  //눈 생성 시 플레이어와의 거리
	public:
		SnowballManager()
		{
			REGISTER_BEHAVIOUR_MESSAGE(Start)
			REGISTER_BEHAVIOUR_MESSAGE(Update)
		}
		void Start();
		void Update();
		void OnScoopStart(Player& player);
		void OnScoopHold(Player& player);
		void OnScoopEnd(Player& player);
		std::vector<ObjPtr<GameObject>> Snows;

		static ObjPtr<SnowballManager> instance;
	};
}
