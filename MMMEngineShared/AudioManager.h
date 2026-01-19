#pragma once
#include <fmod.hpp>
#include <unordered_map>
#include <string>
#include "ExportSingleton.hpp"

#pragma warning(push)
#pragma warning(disable: 4251)

namespace MMMEngine {
	class MMMENGINE_API AudioManager : public Utility::ExportSingleton<AudioManager>
	{
	public:
		void StartUp();
		void Update();
		void ShutDown();
		void SoundCacheClear();

		void SetListenerPosition(float x, float y, float z,
			float fwdx, float fwdy, float fwdz,
			float upx, float upy, float upz);
		
		//각 컴포넌트에서 호출
		FMOD::Channel* PlayBGM(const std::string& id);
		FMOD::Channel* PlaySFX2D(const std::string& id);
		FMOD::Channel* PlaySFX3D(const std::string& id, float x, float y, float z);

		//SoundManager에서 호출
		void RegisterBGM(const std::string& id, const char* path, bool loop);
		void RegisterSFX2D(const std::string& id, const char* path, bool loop);
		void RegisterSFX3D(const std::string& id, const char* path, float min, float max, bool loop);
		bool RegisterSound(const std::string& csvPath);
		void BGMVolumeChange(float v) { BGMVolume = v; if (mBGMGroup) mBGMGroup->setVolume(v); }
		void SFXVolumeChange(float v) { SFXVolume = v; if (mSFXGroup) mSFXGroup->setVolume(v); }
		FMOD::ChannelGroup* mBGMGroup = nullptr;
		FMOD::ChannelGroup* mSFXGroup = nullptr;
	private:
		FMOD::System* mSystem = nullptr;
		enum class SoundKind { BGM, SFX2D, SFX3D };
		struct SoundEntry {
			SoundKind kind;
			FMOD::Sound* sound;
		};
		std::unordered_map<std::string, SoundEntry> sSoundCache;
		FMOD_VECTOR mListenerPos{ 0.0f, 0.0f, 0.0f };
		std::string mBGMPath = ".";
		std::string mSFXPath = ".";
		float BGMVolume = 1.0f;
		float SFXVolume = 1.0f;
	};
	static inline bool FMOD_CHECK(FMOD_RESULT r)
	{
		return r == FMOD_OK;
	}

	static inline FMOD_VECTOR V3(float x, float y, float z)
	{
		FMOD_VECTOR v; v.x = x; v.y = y; v.z = z;
		return v;
	}
}

#pragma warning(pop)