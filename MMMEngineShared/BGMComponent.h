#pragma once
#include "AudioManager.h"
#include "Component.h"
#include "Export.h"

namespace MMMEngine {
	class MMMENGINE_API BGMComponent : public Component
	{
	private:
		RTTR_ENABLE(Component)
		RTTR_REGISTRATION_FRIEND
		friend class ObjectManager;
		friend class GameObject;
		FMOD::Channel* bgmChannel = nullptr;

	protected:
		BGMComponent() {};
		virtual void Initialize() override {};
		virtual void UnInitialize() override;
	public:
		void PlayBGM(const std::string& id);
		void StopSound();
	};
}

