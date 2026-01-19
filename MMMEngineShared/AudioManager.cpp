#include "AudioManager.h"

#include <fmod_errors.h>
#include <fstream>
#include <sstream>

DEFINE_SINGLETON(MMMEngine::AudioManager)

void MMMEngine::AudioManager::StartUp()
{
	//시스템 생성
	FMOD_RESULT r = FMOD::System_Create(&mSystem);
	if (!FMOD_CHECK(r) || !mSystem)
		return;

	//시스템 초기화
	r = mSystem->init(512, FMOD_INIT_NORMAL, nullptr);
	if (!FMOD_CHECK(r))
		return;

	//3D 설정
	const float dopplerScale = 1.0f;
	const float distanceFactor = 1.0f;
	const float rolloffScale = 1.0f;

	r = mSystem->set3DSettings(dopplerScale, distanceFactor, rolloffScale);
	if (!FMOD_CHECK(r))
		return;

	//리스너 기본값 세팅
	mListenerPos = V3(0.0f, 0.0f, 0.0f);

	FMOD_VECTOR vel = V3(0.0f, 0.0f, 0.0f);
	FMOD_VECTOR fwd = V3(0.0f, 0.0f, 1.0f);
	FMOD_VECTOR up = V3(0.0f, 1.0f, 0.0f);

	r = mSystem->set3DListenerAttributes(0, &mListenerPos, &vel, &fwd, &up);
	if (!FMOD_CHECK(r))
		return;
	//반영
	r = mSystem->update();
	if (!FMOD_CHECK(r))
		return;

	FMOD::ChannelGroup* master = nullptr;
	mSystem->getMasterChannelGroup(&master);

	mSystem->createChannelGroup("BGM", &mBGMGroup);
	mSystem->createChannelGroup("SFX", &mSFXGroup);

	master->addGroup(mBGMGroup);
	master->addGroup(mSFXGroup);

	mBGMGroup->setVolume(BGMVolume);
	mSFXGroup->setVolume(SFXVolume);
 }

void MMMEngine::AudioManager::Update()
{
	if (mSystem) mSystem->update();
}

void MMMEngine::AudioManager::ShutDown()
{
	if (mBGMGroup) mBGMGroup->stop();
	if (mSFXGroup) mSFXGroup->stop();

	if (mBGMGroup)
	{
		mBGMGroup->release();
		mBGMGroup = nullptr;
	}
	if (mSFXGroup)
	{
		mSFXGroup->release();
		mSFXGroup = nullptr;
	}
	SoundCacheClear();
	sSoundCache.clear();
	mSystem->close();
	mSystem->release();
	mSystem = nullptr;
}

void MMMEngine::AudioManager::SoundCacheClear()
{
	if (!mSystem)
		return;
	for (auto& it : sSoundCache)
	{
		if (it.second.sound)
		{
			it.second.sound->release();
		}
	}
}

void MMMEngine::AudioManager::RegisterBGM(const std::string& id, const char* path, bool loop)
{
	if (!mSystem || id.empty() || !path)
		return;
	FMOD::Sound* newSound = nullptr;
	FMOD_MODE mode = FMOD_2D | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

	FMOD_RESULT r = mSystem->createStream(path, mode, nullptr, &newSound);
	if (!FMOD_CHECK(r) || !newSound)
		return;
	auto it = sSoundCache.find(id);
	if (it != sSoundCache.end())
	{
		if (it->second.sound) it->second.sound->release();
		it->second = SoundEntry{ SoundKind::BGM, newSound };
	}
	else {
		sSoundCache.emplace(id, SoundEntry{ SoundKind::BGM, newSound });
	}
}

void MMMEngine::AudioManager::RegisterSFX2D(const std::string& id, const char* path, bool loop)
{
	if (!mSystem || id.empty() || !path)
		return;
	FMOD::Sound* newSound = nullptr;
	FMOD_MODE mode = FMOD_2D | (loop? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

	FMOD_RESULT r = mSystem->createSound(path, mode, nullptr, &newSound);
	if (!FMOD_CHECK(r) || !newSound)
		return;
	auto it = sSoundCache.find(id);
	if (it != sSoundCache.end())
	{
		if (it->second.sound) it->second.sound->release();
		it->second = SoundEntry{ SoundKind::SFX2D, newSound };
	}
	else {
		sSoundCache.emplace(id, SoundEntry{ SoundKind::SFX2D, newSound });
	}
}

void MMMEngine::AudioManager::RegisterSFX3D(const std::string& id, const char* path, float min, float max, bool loop)
{
	if (!mSystem || id.empty() || !path)
		return;
	FMOD::Sound* newSound = nullptr;
	FMOD_MODE mode = FMOD_3D | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

	FMOD_RESULT r = mSystem->createSound(path, mode, nullptr, &newSound);
	if (!FMOD_CHECK(r) || !newSound)
		return;
	r = newSound->set3DMinMaxDistance(min, max);
	if (!FMOD_CHECK(r))
	{
		newSound->release();
		return;
	}
	auto it = sSoundCache.find(id);
	if (it != sSoundCache.end())
	{
		if (it->second.sound) it->second.sound->release();
		it->second = SoundEntry{ SoundKind::SFX3D, newSound };
	}
	else {
		sSoundCache.emplace(id, SoundEntry{ SoundKind::SFX3D, newSound });
	}
}

void MMMEngine::AudioManager::SetListenerPosition(float x, float y, float z,
	float fwdx, float fwdy, float fwdz,
	float upx, float upy, float upz)
{
	if (!mSystem)
		return;

	mListenerPos.x = x;
	mListenerPos.y = y;
	mListenerPos.z = z;

	FMOD_VECTOR vel = V3(0.0f, 0.0f, 0.0f);
	FMOD_VECTOR fwd = V3(fwdx, fwdy, fwdz);
	FMOD_VECTOR up = V3(upx, upy, upz);

	mSystem->set3DListenerAttributes(0, &mListenerPos, &vel, &fwd, &up);
}

FMOD::Channel* MMMEngine::AudioManager::PlayBGM(const std::string& id)
{
	if (!mSystem) return nullptr;
	auto it = sSoundCache.find(id);
	if (it == sSoundCache.end() || it->second.kind != SoundKind::BGM || !it->second.sound)
		return nullptr;

	FMOD::Channel* ch = nullptr;
	FMOD_RESULT r = mSystem->playSound(it->second.sound, nullptr, true, &ch);
	if (!FMOD_CHECK(r) || !ch)
		return nullptr;

	ch->setVolume(1.0f);
	if (mBGMGroup) ch->setChannelGroup(mBGMGroup);
	ch->setPaused(false);
	
	return ch;
}

FMOD::Channel* MMMEngine::AudioManager::PlaySFX2D(const std::string& id)
{
	if (!mSystem) return nullptr;
	auto it = sSoundCache.find(id);
	if (it == sSoundCache.end() || it->second.kind != SoundKind::SFX2D || !it->second.sound)
		return nullptr;

	FMOD::Channel* ch = nullptr;
	FMOD_RESULT r = mSystem->playSound(it->second.sound, nullptr, true, &ch);
	if (!FMOD_CHECK(r) || !ch)
		return nullptr;

	ch->setVolume(1.0f);
	if (mBGMGroup) ch->setChannelGroup(mSFXGroup);
	ch->setPaused(false);

	return ch;
}

FMOD::Channel* MMMEngine::AudioManager::PlaySFX3D(const std::string& id, float x, float y, float z)
{
	if (!mSystem) return nullptr;
	auto it = sSoundCache.find(id);
	if (it == sSoundCache.end() || it->second.kind != SoundKind::SFX3D || !it->second.sound)
		return nullptr;

	FMOD::Channel* ch = nullptr;
	FMOD_RESULT r = mSystem->playSound(it->second.sound, nullptr, true, &ch);
	if (!FMOD_CHECK(r) || !ch)
		return nullptr;

	ch->setVolume(1.0f);
	if (mBGMGroup) ch->setChannelGroup(mSFXGroup);
	ch->setPaused(false);
	FMOD_VECTOR pos = V3(x, y, z);
	FMOD_VECTOR vel = V3(0.0f, 0.0f, 0.0f);
	ch->set3DAttributes(&pos, &vel);
	return ch;
}


static inline void Trim(std::string& s)
{
	auto notSpace = [](unsigned char c) { return !std::isspace(c); };

	s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
	s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}

bool MMMEngine::AudioManager::RegisterSound(const std::string& csvPath)
{
	if (!mSystem) return false;

	std::ifstream fin(csvPath);
	if (!fin.is_open()) return false;

	std::string line;
	bool firstLine = true;

	while (std::getline(fin, line))
	{
		if (line.empty()) continue;
		if (line[0] == '#') continue;
		if (firstLine)
		{
			firstLine = false;
			if (line.find("kind") != std::string::npos && line.find("id") != std::string::npos)
				continue;
		}
		std::vector<std::string> cols;
		{
			std::stringstream ss(line);
			std::string cell;
			while (std::getline(ss, cell, ',')) {
				Trim(cell);
				cols.push_back(cell);
			}
		}
		if (cols.size() < 3) continue;

		std::string kind = cols[0];
		std::string id = cols[1];
		std::string filename = cols[2];
		std::string path;
		if (kind == "bgm")
			path = mBGMPath + "/" + filename;
		else if (kind == "sfx2d" || kind == "sfx3d")
			path = mSFXPath + "/" + filename;
		bool loop = false;
		float min = 1.f;
		float max = 20.f;
		if (cols.size() >= 4)
		{
			std::string v = cols[3];
			for (auto& c : v) c = (char)tolower(c);
			loop = (v == "1" || v == "true" || v == "yes");
		}

		if (cols.size() >= 6 && kind == "sfx3d")
		{
			try {
				min = std::stof(cols[4]);
				max = std::stof(cols[5]);
			}
			catch (...) {
				min = 1.f;
				max = 20.f;
			}
			if (min > max) std::swap(min, max);
		}
		if (kind == "bgm")
			RegisterBGM(id, path.c_str(), loop);
		else if (kind == "sfx2d")
			RegisterSFX2D(id, path.c_str(), loop);
		else if (kind == "sfx3d")
			RegisterSFX3D(id, path.c_str(), min, max, loop);
		else
			continue;

	}
	return true;
}