#pragma once
#include "AudioManager.h"
#include "Component.h"
#include "Export.h"

namespace MMMEngine {
	class MMMENGINE_API SFX3DComponent : public Component
	{
	private:
		RTTR_ENABLE(Component)
		RTTR_REGISTRATION_FRIEND
		friend class ObjectManager;
		friend class GameObject;
		FMOD::Channel* sfxChannel[5] = {};
		FMOD::Channel* loopsfxChannel[3] = {};
		int mNextSlot = 0;

	protected:
		SFX3DComponent();
		virtual void Initialize() override {};
		virtual void UnInitialize() override;
		int AcquireSlot();
	public:
		void PlaySFX3D(const std::string& id, float x, float y, float z);
		void PlayLoopSFX3D(const std::string& id, float x, float y, float z, int slot);
		void StopLoopSFX3D(int slot);
		void PosChange(float x, float y, float z);
	};
}

