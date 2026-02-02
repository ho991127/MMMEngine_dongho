#include "PrefabMaker.h"
#include "Component.h"
#include "Transform.h"
#include "RectTransform.h"
#include "MissingScriptBehaviour.h"
#include "Resource.h"
#include "SerializableEvent.h"
#include "StringHelper.h"

#include "json/json.hpp"
#include "rttr/type"

#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace MMMEngine;
using namespace rttr;

namespace
{
    void CollectHierarchy(const ObjPtr<GameObject>& root, std::vector<ObjPtr<GameObject>>& out)
    {
        if (!root.IsValid() || root->IsDestroyed())
            return;

        std::vector<ObjPtr<GameObject>> stack;
        stack.push_back(root);

        while (!stack.empty())
        {
            ObjPtr<GameObject> current = stack.back();
            stack.pop_back();

            if (!current.IsValid() || current->IsDestroyed())
                continue;

            out.push_back(current);

            auto tr = current->GetTransform();
            if (!tr.IsValid())
                continue;

            const size_t childCount = tr->GetChildCount();
            for (size_t i = childCount; i-- > 0;)
            {
                auto childTr = tr->GetChild(i);
                if (!childTr.IsValid())
                    continue;

                auto childGo = childTr->GetGameObject();
                if (childGo.IsValid() && !childGo->IsDestroyed())
                    stack.push_back(childGo);
            }
        }
    }

    json SerializeVariant(const rttr::variant& var)
    {
        rttr::type t = var.get_type();

        if (t.is_enumeration())
        {
            rttr::enumeration enumType = t.get_enumeration();
            std::string enumName = enumType.value_to_name(var).to_string();

            json enumJson;
            enumJson["EnumType"] = t.get_name().to_string();
            enumJson["EnumValue"] = enumName;
            return enumJson;
        }

        if (t.is_arithmetic())
        {
            if (t == type::get<bool>()) return var.to_bool();
            if (t == type::get<int>()) return var.to_int();
            if (t == type::get<unsigned int>()) return var.to_uint32();
            if (t == type::get<long long>()) return var.to_int64();
            if (t == type::get<uint64_t>()) return var.to_uint64();
            if (t == type::get<float>()) return var.to_float();
            if (t == type::get<double>()) return var.to_double();
        }

        if (t == type::get<MMMEngine::Utility::MUID>())
        {
            return var.get_value<MMMEngine::Utility::MUID>().ToString();
        }

        if (t == type::get<std::string>())
        {
            return var.to_string();
        }

        if (t.is_sequential_container())
        {
            json arr = json::array();
            auto view = var.create_sequential_view();
            for (const auto& item : view)
            {
                arr.push_back(SerializeVariant(item));
            }
            return arr;
        }

        // shared_ptr<Resource> 처리
        if (t.is_wrapper())
        {
            auto args = t.get_template_arguments();
            if (args.begin() != args.end())
            {
                rttr::type innerType = *args.begin();
                rttr::type resourceBase = rttr::type::get<MMMEngine::Resource>();

                if (innerType.is_derived_from(resourceBase) || innerType == resourceBase)
                {
                    auto resPtr = var.get_value<std::shared_ptr<MMMEngine::Resource>>();
                    if (resPtr && !resPtr->GetFilePath().empty())
                    {
                        return MMMEngine::Utility::StringHelper::WStringToString(
                            resPtr->GetFilePath()
                        );
                    }
                    return nullptr;
                }
            }
        }

        if (var.get_type().get_name().to_string().find("ObjPtr") != std::string::npos)
        {
            MMMEngine::Object* obj = nullptr;
            if (var.convert(obj) && obj != nullptr)
            {
                return obj->GetMUID().ToString();
            }
            return nullptr;
        }

        if (t == type::get<MMMEngine::SerializableEvent>())
        {
            json arr = json::array();
            const auto& ev = var.get_value<MMMEngine::SerializableEvent>();
            for (const auto& call : ev.GetCalls())
            {
                arr.push_back({ {"TargetMUID", call.targetMUID}, {"MessageName", call.messageName} });
            }
            return arr;
        }
        if (t == type::get<MMMEngine::SerializableEventT<float>>())
        {
            json arr = json::array();
            const auto& ev = var.get_value<MMMEngine::SerializableEventT<float>>();
            for (const auto& call : ev.GetCalls())
            {
                arr.push_back({ {"TargetMUID", call.targetMUID}, {"MessageName", call.messageName} });
            }
            return arr;
        }

        if (t.is_associative_container())
        {
            json obj;
            auto view = var.create_associative_view();
            for (auto& item : view)
            {
                obj[SerializeVariant(item.first).dump()] = SerializeVariant(item.second);
            }
            return obj;
        }

        // 사용자 정의 타입 -> 재귀
        json out;
        for (auto& prop : t.get_properties(
            rttr::filter_item::instance_item |
            rttr::filter_item::public_access |
            rttr::filter_item::non_public_access))
        {
            if (prop.is_readonly())
                continue;

            rttr::variant value = prop.get_value(var);
            out[prop.get_name().to_string()] = SerializeVariant(value);
        }

        return out;
    }

    json SerializeComponent(const ObjPtr<Component>& comp)
    {
        json compJson;
        type type = type::get(*comp);
        compJson["Type"] = type.get_name().to_string();

        ObjPtr<MissingScriptBehaviour> missing;
        try { missing = comp.Cast<MissingScriptBehaviour>(); }
        catch (...) { /* ignore */ }

        if (missing.IsValid())
        {
            const std::string& originalType = missing->GetOriginalTypeName();
            compJson["Type"] = originalType.empty() ? std::string("MissingScriptBehaviour") : originalType;

            if (missing->HasOriginalProps())
            {
                json props = json::from_msgpack(missing->GetOriginalPropsMsgPack());
                compJson["Props"] = props;
            }
            else
            {
                compJson["Props"] = json::object();
                compJson["Props"]["MUID"] = comp->GetMUID().ToString();
            }

            return compJson;
        }

        for (auto& prop : type.get_properties(
            rttr::filter_item::instance_item |
            rttr::filter_item::public_access |
            rttr::filter_item::non_public_access))
        {
            if (prop.is_readonly())
                continue;

            rttr::variant value = prop.get_value(*comp);
            compJson["Props"][prop.get_name().to_string()] = SerializeVariant(value);
        }

        return compJson;
    }
}

std::filesystem::path MMMEngine::Editor::PrefabMaker::MakeFileUnique(const std::filesystem::path& parentDir,
    const std::string& fileName, const std::string& extension) const
{
    fs::path path = parentDir / (fileName + extension);
    for (int i = 1; i < 100; i++)
    {
        if (!fs::exists(path))
            break;
        path = parentDir / (fileName + "_" + std::to_string(i) + extension);
    }
    return path;
}

bool MMMEngine::Editor::PrefabMaker::CreatePrefabFromGameObject(const ObjPtr<GameObject>& root,
    const std::filesystem::path& directory)
{
    if (!root.IsValid() || root->IsDestroyed())
        return false;

    std::vector<ObjPtr<GameObject>> gameObjects;
    CollectHierarchy(root, gameObjects);
    if (gameObjects.empty())
        return false;

    json snapshot;
    json goArray = json::array();

    for (auto& goPtr : gameObjects)
    {
        if (!goPtr.IsValid())
            continue;

        json goJson;
        goJson["Name"] = goPtr->GetName();
        goJson["MUID"] = goPtr->GetMUID().ToString();
        goJson["Layer"] = goPtr->GetLayer();
        goJson["Tag"] = goPtr->GetTag();
        goJson["Active"] = goPtr->IsActiveSelf();

        json compArray = json::array();
        for (auto& comp : goPtr->GetAllComponents())
        {
            compArray.push_back(SerializeComponent(comp));
        }
        goJson["Components"] = compArray;

        goArray.push_back(goJson);
    }

    snapshot["GameObjects"] = goArray;
    std::vector<uint8_t> v = json::to_msgpack(snapshot);

    fs::path dirPath = directory;
    if (!dirPath.empty() && !fs::exists(dirPath))
        fs::create_directories(dirPath);

    fs::path outPath = MakeFileUnique(dirPath, root->GetName(), ".Prefab");
    std::ofstream file(outPath, std::ios::binary);
    if (!file.is_open())
        return false;

    file.write(reinterpret_cast<const char*>(v.data()), v.size());
    file.close();

    return true;
}
