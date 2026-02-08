#pragma once
#include "AudioManager.h"
#include "Component.h"
#include "Export.h"

namespace MMMEngine
{
	class MMMENGINE_API SoundController : public Component
	{
	private:
		RTTR_ENABLE(Component)
		RTTR_REGISTRATION_FRIEND
		friend class ObjectManager;
		friend class GameObject;
	protected:
		SoundController() {};
		virtual void Initialize() override {}
		virtual void UnInitialize() override {}
	public:
		void RegisterSound(const std::string& csvPath) { AudioManager::Get().RegisterSound(csvPath); }
		void ChangeBGMVolume(float volume) { MMMEngine::AudioManager::Get().BGMVolumeChange(volume); }
		void ChangeSFXVolume(float volume) { MMMEngine::AudioManager::Get().SFXVolumeChange(volume); }
	};
}
