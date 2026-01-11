#pragma once
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <vector>

#include "MUID.h"
#include "Singleton.hpp"

#include "Resource.h"
#include "IAssetResolver.h"
#include "IBytesProvider.h"


namespace MMMEngine
{
	template <typename T>
	using ResPtr = std::shared_ptr<T>;

	template <typename T>
	using ResWeakPtr = std::weak_ptr<T>;

	class ResourceManager : public Utility::Singleton<ResourceManager>
	{
	private:
		std::unordered_map<Utility::MUID, ResWeakPtr<Resource>, Utility::MUID::Hash> m_cache;
		const IAssetResolver* m_resolver = nullptr;
		IBytesProvider* m_bytes = nullptr;
		
		template<class T>
		ResPtr<T> LoadInternal(const AssetEntry& entry)
		{
			static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");
			if (!m_bytes) return nullptr;

			// cache
			if (auto it = m_cache.find(entry.muid); it != m_cache.end())
				if (auto sp = it->second.lock())
					return std::dynamic_pointer_cast<T>(sp);

			// read bytes
			std::vector<uint8_t> bytes;
			if (!m_bytes->ReadAll(entry, bytes)) return nullptr;

			// create
			auto res = std::make_shared<T>();
			res->SetMUID(entry.muid);
			if (!res->LoadFromBytes(bytes.data(), bytes.size()))
				return nullptr;

			m_cache[entry.muid] = res;
			return res;
		}


	public:
		void SetResolver(const IAssetResolver* r) { m_resolver = r; }
		void SetBytesProvider(IBytesProvider* p) { m_bytes = p; }

		void ClearCache() { m_cache.clear(); }

		template<class T>
		ResPtr<T> Load(std::string_view sourcePath)
		{
			AssetEntry entry{};
			if (!m_resolver || !m_resolver->Resolve(sourcePath, entry)) return nullptr;
			return LoadInternal<T>(entry);
		}

		template<class T>
		ResPtr<T> LoadByGuid(const Utility::MUID& muid)
		{
			AssetEntry entry{};
			if (!m_resolver || !m_resolver->Resolve(muid, entry)) return nullptr;
			entry.muid = muid; // ¾ÈÀü
			return LoadInternal<T>(entry);
		}

		bool Containskey(const Utility::MUID& key)
		{
			auto resource_iter = m_cache.find(key);
			if (resource_iter != m_cache.end())
			{
				auto res_shared = resource_iter->second.lock();
				if (res_shared)
					return true;

				m_cache.erase(resource_iter);
			}

			return false;
		}
	};
}