#pragma once
#include "AudioManager.h"
#include "ScriptBehaviour.h"

namespace MMMEngine {
	class SoundManager : public ScriptBehaviour
	{
	public:
		void Initialize() {};
		void Uninitialize() { AudioManager::Get().SoundCacheClear(); };
		void RegisterSound(const std::string& csvPath) { AudioManager::Get().RegisterSound(csvPath); };
		void ChangeBGMVolume(float volume) { MMMEngine::AudioManager::Get().BGMVolumeChange(volume); }
		void ChangeSFXVolume(float volume) { MMMEngine::AudioManager::Get().SFXVolumeChange(volume); }
		
	private:
		
	};
}
