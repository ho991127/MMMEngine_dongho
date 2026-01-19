#include "BGMComponent.h"

void MMMEngine::BGMComponent::PlayBGM(const std::string& id)
{
	StopSound();
	bgmChannel = AudioManager::Get().PlayBGM(id);
}

void MMMEngine::BGMComponent::StopSound()
{
	if (bgmChannel)
	{
		bgmChannel->stop();
		bgmChannel = nullptr;
	}
}

void MMMEngine::BGMComponent::UnInitialize()
{
	StopSound();
}