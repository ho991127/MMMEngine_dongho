#include "SFX2DComponent.h"
#include "rttr/registration.h"

RTTR_REGISTRATION
{
	using namespace rttr;
	using namespace MMMEngine;

	registration::class_<SFX2DComponent>("SFX2DComponent")
		(rttr::metadata("wrapper_type_name", "ObjPtr<SFX2DComponent>"));
	registration::class_<ObjPtr<SFX2DComponent>>("ObjPtr<SFX2DComponent>")
		.constructor<>(
			[]() {
				return Object::NewObject<SFX2DComponent>();
			}).method("Inject", &ObjPtr<SFX2DComponent>::Inject);
}


void MMMEngine::SFX2DComponent::UnInitialize()
{
	for (int i = 0; i < 5; i++) {
		if (sfxChannel[i]) {
			sfxChannel[i]->stop();
			sfxChannel[i] = nullptr;
		}
	}
	if (loopsfxChannel) {
		loopsfxChannel->stop();
		loopsfxChannel = nullptr;
	}
}

void MMMEngine::SFX2DComponent::PlaySFX2D(const std::string& id)
{
	sfxChannel[AcquireSlot()] = AudioManager::Get().PlaySFX2D(id);
}

void MMMEngine::SFX2DComponent::PlayLoopSFX2D(const std::string& id)
{

	StopLoopSFX2D();
	loopsfxChannel = AudioManager::Get().PlaySFX2D(id);
}

void MMMEngine::SFX2DComponent::StopLoopSFX2D()
{
	loopsfxChannel->stop();
	loopsfxChannel = nullptr;
}

int MMMEngine::SFX2DComponent::AcquireSlot()
{
	int slot = mNextSlot;
	mNextSlot = (mNextSlot + 1) % 5;

	if (sfxChannel[slot])
	{
		bool playing = false;
		if (sfxChannel[slot]->isPlaying(&playing) != FMOD_OK || !playing)
		{
			sfxChannel[slot] = nullptr;
		}
		else
		{
			sfxChannel[slot]->stop();
			sfxChannel[slot] = nullptr;
		}
	}
	return slot;
}
