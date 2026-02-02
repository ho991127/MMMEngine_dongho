#pragma once
#include "Singleton.hpp"
#include "GameObject.h"
#include <filesystem>

namespace MMMEngine::Editor
{
    class PrefabMaker : public Utility::Singleton<PrefabMaker>
    {
    public:
        bool CreatePrefabFromGameObject(const ObjPtr<GameObject>& root, const std::filesystem::path& directory);
    private:
        std::filesystem::path MakeFileUnique(const std::filesystem::path& parentDir,
            const std::string& fileName, const std::string& extension) const;
    };
}
