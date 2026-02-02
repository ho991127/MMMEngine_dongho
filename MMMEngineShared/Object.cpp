#include "GameObject.h"
#include "Object.h"
#include "Component.h"
#include "MissingScriptBehaviour.h"
#include "Prefab.h"
#include "rttr/registration"
#include "rttr/type"
#include "rttr/detail/policies/ctor_policies.h"
#include "ObjectManager.h"
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "Transform.h"
#include "RectTransform.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "SerializableEvent.h"
#include "StringHelper.h"
#include "json/json.hpp"

using namespace MMMEngine::Utility;

uint64_t MMMEngine::Object::s_nextInstanceID = 1;

RTTR_REGISTRATION
{
	using namespace rttr;
	using namespace MMMEngine;

	registration::class_<Object>("Object")
		.property("Name", &Object::GetName, &Object::SetName)(rttr::metadata("INSPECTOR", "HIDDEN"))
		.property("MUID", &Object::GetMUID, &Object::SetMUID)
		.property_readonly("InstanceID", &Object::GetInstanceID)
		.property_readonly("isDestroyed", &Object::IsDestroyed)(rttr::metadata("INSPECTOR", "HIDDEN"));

	registration::class_<ObjPtrBase>("ObjPtr")
		.method("IsValid", &ObjPtrBase::IsValid)
		.method("GetRaw", &ObjPtrBase::GetRaw, registration::private_access)
		.method("GetPtrID", &ObjPtrBase::GetPtrID)
		.method("GetPtrGeneration", &ObjPtrBase::GetPtrGeneration);

	registration::class_<ObjPtr<Object>>("ObjPtr<Object>")
		.constructor<>(
			[]() { 
				return Object::NewObject<Object>(); 
			})
		.method("Inject", &ObjPtr<Object>::Inject);
}

// todo : 4q 끝나고 꼭 통합관리하는 방법으로 처리하기 -> ObjectSerializer 작성할 수 있도록 하기
// 지역함수 -> Object::Instantiate 구현에 사용
namespace
{
	using namespace MMMEngine;
	using namespace rttr;

	// 클론 진행 상황을 추적하는 구조체
	struct CloneContext
	{
		std::unordered_map<const Object*, ObjPtr<Object>> objectMap;
		std::vector<std::pair<ObjPtr<Component>, ObjPtr<Component>>> componentPairs;
		std::vector<std::pair<ObjPtr<Transform>, ObjPtr<Transform>>> transformPairs;
	};

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
			for (size_t i = childCount; i-- > 0; )
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

	ObjPtr<Transform> FindMappedTransform(const CloneContext& ctx, const ObjPtr<Transform>& original)
	{
		if (!original.IsValid())
			return ObjPtr<Transform>();

		auto it = ctx.objectMap.find(original.operator->());
		if (it == ctx.objectMap.end())
			return ObjPtr<Transform>();

		return it->second.Cast<Transform>();
	}

	rttr::variant CloneVariant(const rttr::variant& src, const rttr::type& targetType, const CloneContext& ctx);

	void CloneObject(rttr::instance srcObj, rttr::instance dstObj, const CloneContext& ctx)
	{
		type t = srcObj.get_derived_type();
		const bool isObjectDerived =
			t.is_derived_from(type::get<Object>()) ||
			t == type::get<Object>();

		for (auto& prop : t.get_properties(
			rttr::filter_item::instance_item |
			rttr::filter_item::public_access |
			rttr::filter_item::non_public_access))
		{
			if (prop.is_readonly())
				continue;

			const std::string propName = prop.get_name().to_string();

			if (isObjectDerived && propName == "MUID")
				continue;

			if (t == type::get<Transform>() &&
				(propName == "Parent" || propName == "m_parent"))
				continue;

			rttr::variant value = prop.get_value(srcObj);
			rttr::variant cloned = CloneVariant(value, prop.get_type(), ctx);
			prop.set_value(dstObj, cloned);
		}
	}

	rttr::variant CloneVariant(const rttr::variant& src, const rttr::type& targetType, const CloneContext& ctx)
	{
		if (!src.is_valid())
			return rttr::variant();

		if (targetType.is_enumeration())
			return src;

		if (targetType.is_arithmetic())
		{
			if (src.get_type() == targetType)
				return src;

			rttr::variant converted = src;
			if (converted.convert(targetType))
				return converted;

			return src;
		}

		if (targetType == type::get<std::string>() ||
			targetType == type::get<MMMEngine::Utility::MUID>())
		{
			return src;
		}

		if (targetType.get_name().to_string().find("ObjPtr") != std::string::npos)
		{
			auto inject = targetType.get_method("Inject");
			rttr::variant target = targetType.create();
			if (!inject.is_valid() || !target.is_valid())
				return rttr::variant();

			Object* raw = nullptr;
			if (!src.convert(raw) || raw == nullptr || raw->IsDestroyed())
			{
				ObjPtr<Object> nullObj;
				const ObjPtrBase& nullRef = nullObj;
				inject.invoke(target, nullRef);
				return target;
			}

			auto it = ctx.objectMap.find(raw);
			ObjPtr<Object> mapped = (it != ctx.objectMap.end())
				? it->second
				: ObjectManager::Get().GetPtrFromRaw<Object>(raw);

			if (!mapped.IsValid())
			{
				ObjPtr<Object> nullObj;
				const ObjPtrBase& nullRef = nullObj;
				inject.invoke(target, nullRef);
				return target;
			}

			const ObjPtrBase& baseRef = mapped;
			inject.invoke(target, baseRef);
			return target;
		}

		if (targetType.is_sequential_container())
		{
			rttr::variant target = targetType.create();
			if (!target.is_valid())
				return src;

			auto view = target.create_sequential_view();
			view.clear();

			auto args = targetType.get_wrapped_type().get_template_arguments();
			auto it = args.begin();
			if (it == args.end())
				return target;

			rttr::type valueType = *it;

			auto srcView = src.create_sequential_view();
			for (const auto& item : srcView)
			{
				rttr::variant cloned = CloneVariant(item, valueType, ctx);
				view.insert(view.end(), cloned);
			}

			return target;
		}

		if (targetType.is_associative_container())
		{
			rttr::variant target = targetType.create();
			if (!target.is_valid())
				return src;

			auto view = target.create_associative_view();
			view.clear();

			auto args = targetType.get_wrapped_type().get_template_arguments();
			auto it = args.begin();
			if (it == args.end())
				return target;

			rttr::type keyType = *it;
			++it;
			if (it == args.end())
				return target;

			rttr::type valueType = *it;

			auto srcView = src.create_associative_view();
			for (auto& item : srcView)
			{
				rttr::variant key = CloneVariant(item.first, keyType, ctx);
				rttr::variant value = CloneVariant(item.second, valueType, ctx);
				view.insert(key, value);
			}

			return target;
		}

		if (targetType.is_wrapper())
			return src;

		auto props = targetType.get_properties(
			rttr::filter_item::instance_item |
			rttr::filter_item::public_access |
			rttr::filter_item::non_public_access);

		if (props.begin() == props.end())
			return src;

		rttr::variant target = targetType.create();
		if (!target.is_valid())
			return src;

		CloneObject(src, target, ctx);
		return target;
	}

	ObjPtr<GameObject> CreateCloneShallow(const ObjPtr<GameObject>& original, CloneContext& ctx)
	{
		if (!original.IsValid() || original->IsDestroyed())
			return ObjPtr<GameObject>();

		SceneRef sceneRef = original->GetScene();
		ObjPtr<GameObject> clone = Object::NewObject<GameObject>(sceneRef, original->GetName());

		if (auto sceneRaw = SceneManager::Get().GetSceneRaw(sceneRef))
			sceneRaw->RegisterGameObject(clone);

		clone->SetName(original->GetName());
		clone->SetTag(original->GetTag());
		clone->SetLayer(original->GetLayer());
		clone->SetActive(original->IsActiveSelf());

		ctx.objectMap.emplace(original.operator->(), ObjPtr<Object>(clone));

		auto origTr = original->GetTransform();
		auto cloneTr = clone->GetTransform();
		if (origTr.IsValid() && cloneTr.IsValid())
		{
			ctx.objectMap.emplace(origTr.operator->(), ObjPtr<Object>(cloneTr));
			ctx.transformPairs.emplace_back(origTr, cloneTr);
		}

		// RigidBody를 먼저 만들고, 그 다음 나머지 컴포넌트를 생성한다.
		// Collider가 먼저 만들어지면 자동으로 RigidBody가 생성되어 복제값이 덮이는 문제 방지.
		for (auto& comp : original->GetAllComponents())
		{
			if (!comp.IsValid() || comp->IsDestroyed())
				continue;

			if (comp.Cast<Transform>())
				continue;

			rttr::type compType = rttr::type::get(*comp);
			if (compType.get_name().to_string() != "RigidBodyComponent")
				continue;

			ObjPtr<Component> clonedComp = clone->AddComponent(compType);
			if (!clonedComp.IsValid())
				continue;

			ctx.objectMap.emplace(comp.operator->(), ObjPtr<Object>(clonedComp));
			ctx.componentPairs.emplace_back(comp, clonedComp);
		}

		for (auto& comp : original->GetAllComponents())
		{
			if (!comp.IsValid() || comp->IsDestroyed())
				continue;

			if (comp.Cast<Transform>())
				continue;

			rttr::type compType = rttr::type::get(*comp);
			if (compType.get_name().to_string() == "RigidBodyComponent")
				continue;

			ObjPtr<Component> clonedComp = clone->AddComponent(compType);
			if (!clonedComp.IsValid())
				continue;

			ctx.objectMap.emplace(comp.operator->(), ObjPtr<Object>(clonedComp));
			ctx.componentPairs.emplace_back(comp, clonedComp);
		}

		return clone;
	}

	ObjPtr<GameObject> InstantiateGameObjectInternal(const ObjPtr<GameObject>& original, CloneContext& ctx)
	{
		std::vector<ObjPtr<GameObject>> originals;
		CollectHierarchy(original, originals);
		if (originals.empty())
			return ObjPtr<GameObject>();

		ObjPtr<GameObject> rootClone;
		for (auto& go : originals)
		{
			ObjPtr<GameObject> clone = CreateCloneShallow(go, ctx);
			if (!clone.IsValid())
				continue;

			if (go == original)
				rootClone = clone;
		}

		for (auto& pair : ctx.transformPairs)
		{
			auto& srcTr = pair.first;
			auto& dstTr = pair.second;
			if (!srcTr.IsValid() || !dstTr.IsValid())
				continue;

			dstTr->SetLocalPosition(srcTr->GetLocalPosition());
			dstTr->SetLocalRotation(srcTr->GetLocalRotation());
			dstTr->SetLocalScale(srcTr->GetLocalScale());
		}

		for (auto& pair : ctx.componentPairs)
		{
			auto& srcComp = pair.first;
			auto& dstComp = pair.second;
			if (!srcComp.IsValid() || !dstComp.IsValid())
				continue;

			CloneObject(*srcComp, *dstComp, ctx);

			auto missingSrc = srcComp.Cast<MissingScriptBehaviour>();
			if (missingSrc.IsValid())
			{
				auto missingDst = dstComp.Cast<MissingScriptBehaviour>();
				if (missingDst.IsValid())
					missingDst->SetOriginalPropsMsgPack(missingSrc->GetOriginalPropsMsgPack());
			}
		}

		for (auto& go : originals)
		{
			if (!go.IsValid() || go->IsDestroyed())
				continue;

			auto origTr = go->GetTransform();
			auto cloneTr = FindMappedTransform(ctx, origTr);
			if (!cloneTr.IsValid())
				continue;

			const size_t childCount = origTr->GetChildCount();
			for (size_t i = 0; i < childCount; ++i)
			{
				auto childTr = origTr->GetChild(i);
				auto childCloneTr = FindMappedTransform(ctx, childTr);
				if (!childCloneTr.IsValid())
					continue;

				childCloneTr->SetParent(cloneTr, false);
			}
		}

		if (rootClone.IsValid())
		{
			auto origRootParent = original->GetTransform()->GetParent();
			if (origRootParent.IsValid() && !origRootParent->IsDestroyed())
			{
				if (!FindMappedTransform(ctx, origRootParent).IsValid())
					rootClone->GetTransform()->SetParent(origRootParent, false);
			}
		}

		return rootClone;
	}
}

// todo : 4q 끝나고 꼭 통합관리하는 방법으로 처리하기 -> ObjectSerializer 작성할 수 있도록 하기
// Prefab Instantiate 지원
namespace
{
    using namespace MMMEngine;
    using namespace rttr;
    using json = nlohmann::json;

    struct PrefabDeserializeContext
    {
        std::unordered_map<std::string, ObjPtr<Object>> objectTable;
        std::unordered_map<std::string, std::string> muidRemap;
    };

    static bool IsMissingScriptTargetVariant(const rttr::variant& v)
    {
        MMMEngine::Object* o = nullptr;
        if (!v.convert(o) || !o)
            return false;

        rttr::type ot = rttr::type::get(*o);
        return ot.is_derived_from(rttr::type::get<MMMEngine::MissingScriptBehaviour>());
    }

    static std::string RemapMuid(const PrefabDeserializeContext& ctx, const std::string& oldMuid)
    {
        if (oldMuid.empty())
            return {};

        auto it = ctx.muidRemap.find(oldMuid);
        if (it == ctx.muidRemap.end())
            return {};

        return it->second;
    }

    void DeserializeVariantPrefab(rttr::variant& target, const json& j, type target_type, const PrefabDeserializeContext& ctx);

    void DeserializeObjectPrefab(rttr::instance obj, const json& j, const PrefabDeserializeContext& ctx)
    {
        type t = obj.get_derived_type();
        bool isObjectDerived = (t.is_derived_from(type::get<Object>()) || t == type::get<Object>());

        for (auto& prop : t.get_properties(
            rttr::filter_item::instance_item |
            rttr::filter_item::public_access |
            rttr::filter_item::non_public_access))
        {
            if (prop.is_readonly())
                continue;

            std::string propName = prop.get_name().to_string();
            if (isObjectDerived && (propName == "MUID" || propName == "m_muid"))
                continue;

            if (!j.contains(propName))
                continue;

            rttr::variant currentValue = prop.get_value(obj);
            DeserializeVariantPrefab(currentValue, j[propName], prop.get_type(), ctx);
            prop.set_value(obj, currentValue);
        }
    }

    void DeserializeVariantPrefab(rttr::variant& target, const json& j, type target_type, const PrefabDeserializeContext& ctx)
    {
        if (j.is_null())
        {
            target = rttr::variant();
            return;
        }

        if (target_type.is_enumeration())
        {
            if (j.contains("EnumType") && j.contains("EnumValue"))
            {
                std::string enumValueName = j["EnumValue"].get<std::string>();
                rttr::enumeration enumType = target_type.get_enumeration();
                rttr::variant enumValue = enumType.name_to_value(enumValueName);
                if (enumValue.is_valid())
                    target = enumValue;
            }
            return;
        }

        if (target_type.is_arithmetic())
        {
            if (target_type == type::get<bool>()) target = j.get<bool>();
            else if (target_type == type::get<int>()) target = j.get<int>();
            else if (target_type == type::get<unsigned int>()) target = j.get<unsigned int>();
            else if (target_type == type::get<long long>()) target = j.get<long long>();
            else if (target_type == type::get<uint64_t>()) target = j.get<uint64_t>();
            else if (target_type == type::get<float>()) target = j.get<float>();
            else if (target_type == type::get<double>()) target = j.get<double>();
            return;
        }

        if (target_type == type::get<MMMEngine::Utility::MUID>())
        {
            std::string muidStr = j.get<std::string>();
            if (auto parsed = MMMEngine::Utility::MUID::Parse(muidStr); parsed.has_value())
                target = parsed.value();
            else
                target = MMMEngine::Utility::MUID::Empty();
            return;
        }

        if (target_type == type::get<std::string>())
        {
            target = j.get<std::string>();
            return;
        }

        if (target_type == type::get<MMMEngine::SerializableEvent>())
        {
            std::vector<MMMEngine::PersistentCall> calls;
            if (j.is_array())
            {
                for (const auto& item : j)
                {
                    std::string oldMuid = item.contains("TargetMUID") ? item["TargetMUID"].get<std::string>() : "";
                    std::string messageName = item.contains("MessageName") ? item["MessageName"].get<std::string>() : "";
                    std::string newMuid = RemapMuid(ctx, oldMuid);
                    calls.emplace_back(std::move(newMuid), std::move(messageName));
                }
            }
            MMMEngine::SerializableEvent ev;
            ev.SetCalls(std::move(calls));
            target = ev;
            return;
        }
        if (target_type == type::get<MMMEngine::SerializableEventT<float>>())
        {
            std::vector<MMMEngine::PersistentCall> calls;
            if (j.is_array())
            {
                for (const auto& item : j)
                {
                    std::string oldMuid = item.contains("TargetMUID") ? item["TargetMUID"].get<std::string>() : "";
                    std::string messageName = item.contains("MessageName") ? item["MessageName"].get<std::string>() : "";
                    std::string newMuid = RemapMuid(ctx, oldMuid);
                    calls.emplace_back(std::move(newMuid), std::move(messageName));
                }
            }
            MMMEngine::SerializableEventT<float> ev;
            ev.SetCalls(std::move(calls));
            target = ev;
            return;
        }

        if (target_type.is_sequential_container())
        {
            if (!target.is_valid() || target.get_type() != target_type)
                target = target_type.create();

            auto view = target.create_sequential_view();
            view.clear();

            auto args = target_type.get_wrapped_type().get_template_arguments();
            auto it = args.begin();
            if (it == args.end())
                return;

            rttr::type value_type = *it;
            for (const auto& item : j)
            {
                rttr::variant element = value_type.create();
                DeserializeVariantPrefab(element, item, value_type, ctx);
                view.insert(view.end(), element);
            }
            return;
        }

        if (target_type.is_associative_container())
        {
            if (!target.is_valid() || target.get_type() != target_type)
                target = target_type.create();

            auto view = target.create_associative_view();
            view.clear();

            auto args = target_type.get_wrapped_type().get_template_arguments();
            auto it = args.begin();
            if (it == args.end())
                return;

            rttr::type key_type = *it;
            ++it;
            if (it == args.end())
                return;

            rttr::type value_type = *it;
            for (auto& [key, value] : j.items())
            {
                rttr::variant k = key_type.create();
                rttr::variant v = value_type.create();

                json keyJson = json::parse(key);
                DeserializeObjectPrefab(k, keyJson, ctx);
                DeserializeObjectPrefab(v, value, ctx);

                view.insert(k, v);
            }
            return;
        }

        if (target_type.is_wrapper())
        {
            auto args = target_type.get_template_arguments();
            if (args.begin() != args.end())
            {
                rttr::type innerType = *args.begin();
                if (innerType.is_derived_from(rttr::type::get<Resource>()) ||
                    innerType == rttr::type::get<Resource>())
                {
                    std::string pathStr = j.get<std::string>();
                    std::wstring filePath = Utility::StringHelper::StringToWString(pathStr);

                    rttr::variant loadedResource = ResourceManager::Get().Load(innerType, filePath);
                    if (loadedResource.convert(target.get_type()))
                        target = loadedResource;
                    return;
                }
            }
        }

        if (target_type.get_name().to_string().find("ObjPtr") != std::string::npos)
        {
            auto inject = target_type.get_method("Inject");
            if (!inject.is_valid())
            {
                target = rttr::variant();
                return;
            }

            if (j.is_null())
            {
                ObjPtr<Object> nullObj;
                const ObjPtrBase& nullRef = nullObj;
                inject.invoke(target, nullRef);
                return;
            }

            std::string muidStr = j.get<std::string>();
            auto it = ctx.objectTable.find(muidStr);
            if (it == ctx.objectTable.end() || IsMissingScriptTargetVariant(it->second))
            {
                ObjPtr<Object> nullObj;
                const ObjPtrBase& nullRef = nullObj;
                inject.invoke(target, nullRef);
                return;
            }

            rttr::variant src = it->second;
            if (src.is_type<ObjPtr<Object>>())
            {
                ObjPtr<Object> base = src.get_value<ObjPtr<Object>>();
                const ObjPtrBase& baseRef = base;
                inject.invoke(target, baseRef);
                return;
            }

            ObjPtr<Object> nullObj;
            const ObjPtrBase& nullRef = nullObj;
            inject.invoke(target, nullRef);
            return;
        }

        if (!target.is_valid() || target.get_type() != target_type)
        {
            target = target_type.create();
        }

        DeserializeObjectPrefab(target, j, ctx);
    }

    struct PendingComponentProps
    {
        ObjPtr<Component> comp;
        const json* props = nullptr;
    };

    ObjPtr<Component> CreateComponentForDeserializePrefab(const json& compJson, ObjPtr<GameObject> obj, bool& outIsMissing,
        PrefabDeserializeContext& ctx)
    {
        outIsMissing = false;

        if (!compJson.contains("Type"))
            return {};

        std::string typeName = compJson["Type"].get<std::string>();
        type compType = type::get_by_name(typeName);

        const json* propsPtr = compJson.contains("Props") ? &compJson["Props"] : nullptr;

        if (!compType.is_valid())
        {
            compType = rttr::type::get<MissingScriptBehaviour>();
            auto compVar = obj->AddComponent(compType);
            if (!compVar.IsValid())
                return {};

            if (propsPtr && propsPtr->contains("MUID"))
            {
                std::string muid = (*propsPtr)["MUID"].get<std::string>();
                ctx.objectTable[muid] = ObjPtr<Object>(compVar);
                ctx.muidRemap[muid] = compVar->GetMUID().ToString();
            }

            ObjPtr<MissingScriptBehaviour> missing = compVar.Cast<MissingScriptBehaviour>();
            if (missing.IsValid())
            {
                missing->SetOriginalTypeName(typeName);
                if (propsPtr)
                {
                    std::vector<uint8_t> packed = json::to_msgpack(*propsPtr);
                    missing->SetOriginalPropsMsgPack(std::move(packed));
                }
            }

            outIsMissing = true;
            return compVar;
        }

        auto comp = obj->AddComponent(compType);
        if (!comp.IsValid())
            return {};

        if (propsPtr && propsPtr->contains("MUID"))
        {
            std::string muid = (*propsPtr)["MUID"].get<std::string>();
            ctx.objectTable[muid] = ObjPtr<Object>(comp);
            ctx.muidRemap[muid] = comp->GetMUID().ToString();
        }

        return comp;
    }

    void DeserializeTransformPrefab(Transform& tr, const json& j, const type& t, const PrefabDeserializeContext& ctx)
    {
        for (auto& prop : t.get_properties(
            rttr::filter_item::instance_item |
            rttr::filter_item::public_access |
            rttr::filter_item::non_public_access))
        {
            if (prop.is_readonly())
                continue;

            std::string name = prop.get_name().to_string();
            if (name == "Parent" || name == "m_parent" || name == "MUID" || name == "m_muid")
                continue;

            if (!j.contains(name))
                continue;

            rttr::variant v = prop.get_value(tr);
            DeserializeVariantPrefab(v, j[name], prop.get_type(), ctx);
            prop.set_value(tr, v);
        }
    }

    struct TransformCompInfo
    {
        const json* comp = nullptr;
        bool isRect = false;
    };

    static TransformCompInfo FindTransformComp(const json& components)
    {
        TransformCompInfo info;

        for (const auto& c : components)
        {
            if (!c.contains("Type")) continue;
            std::string t = c["Type"].get<std::string>();
            if (t == "RectTransform")
            {
                info.comp = &c;
                info.isRect = true;
                return info;
            }
        }

        for (const auto& c : components)
        {
            if (!c.contains("Type")) continue;
            std::string t = c["Type"].get<std::string>();
            if (t == "Transform")
            {
                info.comp = &c;
                info.isRect = false;
                return info;
            }
        }

        return info;
    }
}

MMMEngine::Object::Object() : m_instanceID(s_nextInstanceID++)
{
	if (!ObjectManager::Get().IsCreatingObject())
	{
		throw std::runtime_error(
			"Object는 CreatePtr로만 생성할 수 있습니다.\n"
			"스택 생성이나 직접 new 사용이 감지되었습니다."
		);
	}

	m_name = "<Unnamed> [ Instance ID : " + std::to_string(m_instanceID) + " ]";
	m_muid = MUID::NewMUID();
	m_ptrID = UINT32_MAX;
	m_ptrGen = 0;
}

void MMMEngine::Object::SetMUID(const Utility::MUID& muid)
{
    if (m_muid == muid)
        return;

    Utility::MUID oldMuid = m_muid;
    m_muid = muid;
    ObjectManager::Get().UpdateObjectMUID(this, oldMuid, m_muid);
}

MMMEngine::Object::~Object()
{
	if (!ObjectManager::Get().IsDestroyingObject())
	{
#ifdef _DEBUG
		// Debug: 디버거 중단 + 스택 트레이스
		std::cerr << "\n=== 오브젝트 파괴 오류 ===" << std::endl;
		std::cerr << "Object는 Destroy로만 파괴할 수 있습니다." << std::endl;
		std::cerr << "직접 delete 사용이 감지되었습니다." << std::endl;
		std::cerr << "\n>>> 호출 스택 확인 <<<\n" << std::endl;
		__debugbreak();
#endif
		// Release와 Debug 모두: 즉시 종료
		std::cerr << "\nFATAL ERROR: 허용되지 않는 방법으로 오브젝트 파괴" << std::endl;
		std::abort(); 
	}
}

MMMEngine::ObjPtr<MMMEngine::GameObject> MMMEngine::Object::Instantiate(const ObjPtr<GameObject>& original)
{
	if (!original.IsValid() || original->IsDestroyed())
		return ObjPtr<GameObject>();

	CloneContext ctx;
	return InstantiateGameObjectInternal(original, ctx);
}

MMMEngine::ObjPtr<MMMEngine::Component> MMMEngine::Object::Instantiate(const ObjPtr<Component>& original)
{
	if (!original.IsValid() || original->IsDestroyed())
		return ObjPtr<Component>();

	auto owner = original->GetGameObject();
	if (!owner.IsValid() || owner->IsDestroyed())
		return ObjPtr<Component>();

	CloneContext ctx;
	ObjPtr<GameObject> clonedRoot = InstantiateGameObjectInternal(owner, ctx);
	if (!clonedRoot.IsValid())
		return ObjPtr<Component>();

	auto it = ctx.objectMap.find(original.operator->());
	if (it != ctx.objectMap.end())
		return it->second.Cast<Component>();

	rttr::type originalType = rttr::type::get(*original);
	for (auto& comp : clonedRoot->GetAllComponents())
	{
		if (!comp.IsValid() || comp->IsDestroyed())
			continue;

		if (rttr::type::get(*comp) == originalType)
			return comp;
	}

	return ObjPtr<Component>();
}

MMMEngine::ObjPtr<MMMEngine::GameObject> MMMEngine::Object::Instantiate(const ResPtr<Prefab>& prefab)
{
    if (!prefab)
        return ObjPtr<GameObject>();

    const SnapShot& snapshot = prefab->GetSnapshot();
    if (!snapshot.contains("GameObjects"))
        return ObjPtr<GameObject>();

    const auto& gameObjects = snapshot["GameObjects"];
    if (!gameObjects.is_array() || gameObjects.empty())
        return ObjPtr<GameObject>();

    SceneRef sceneRef = SceneManager::Get().GetCurrentScene();
    Scene* sceneRaw = SceneManager::Get().GetSceneRaw(sceneRef);
    if (!sceneRaw)
        return ObjPtr<GameObject>();

    PrefabDeserializeContext ctx;
    std::unordered_map<std::string, std::string> pendingParent; // childTrMUID -> parentTrMUID

    std::string rootMuid = gameObjects[0].contains("MUID")
        ? gameObjects[0]["MUID"].get<std::string>()
        : std::string();

    // 1-pass: GO + Transform 생성/복원
    for (const auto& goJson : gameObjects)
    {
        std::string goName = goJson.contains("Name") ? goJson["Name"].get<std::string>() : "GameObject";
        std::string goMUID = goJson.contains("MUID") ? goJson["MUID"].get<std::string>() : "";
        uint32_t goLayer = goJson.contains("Layer") ? goJson["Layer"].get<uint32_t>() : 0;
        std::string goTag = goJson.contains("Tag") ? goJson["Tag"].get<std::string>() : "";
        bool active = goJson.contains("Active") ? goJson["Active"].get<bool>() : true;

        ObjPtr<GameObject> go = Object::NewObject<GameObject>(sceneRef, goName);
        if (auto currentScene = SceneManager::Get().GetSceneRaw(sceneRef))
            currentScene->RegisterGameObject(go);
        go->SetName(goName);
        go->SetLayer(goLayer);
        go->SetTag(goTag);
        go->SetActive(active);

        if (!goMUID.empty())
        {
            ctx.objectTable[goMUID] = ObjPtr<Object>(go);
            ctx.muidRemap[goMUID] = go->GetMUID().ToString();
        }

        if (!goJson.contains("Components"))
            continue;

        const nlohmann::json& components = goJson["Components"];
        TransformCompInfo trCompInfo = FindTransformComp(components);
        if (!trCompInfo.comp || !trCompInfo.comp->contains("Props"))
            continue;

        const nlohmann::json& trProps = (*trCompInfo.comp)["Props"];

        if (trCompInfo.isRect)
            go->EnsureRectTransform();

        auto tr = go->GetTransform();
        if (!tr.IsValid())
            continue;

        if (trProps.contains("MUID"))
        {
            std::string trMUID = trProps["MUID"].get<std::string>();
            if (!trMUID.empty())
            {
                ctx.objectTable[trMUID] = ObjPtr<Object>(tr);
                ctx.muidRemap[trMUID] = tr->GetMUID().ToString();
            }
        }

        auto trType = trCompInfo.isRect ? type::get<RectTransform>() : type::get<Transform>();
        DeserializeTransformPrefab(*tr, trProps, trType, ctx);

        if (trProps.contains("Parent") && trProps.contains("MUID") && !trProps["Parent"].is_null())
            pendingParent[trProps["MUID"].get<std::string>()] = trProps["Parent"].get<std::string>();
    }

    std::vector<PendingComponentProps> pendingComponentProps;

    // 2-pass: 컴포넌트 생성 (RigidBody 먼저)
    for (const auto& goJson : gameObjects)
    {
        if (!goJson.contains("MUID"))
            continue;

        std::string goMUID = goJson["MUID"].get<std::string>();
        auto itGo = ctx.objectTable.find(goMUID);
        if (itGo == ctx.objectTable.end())
            continue;

        ObjPtr<GameObject> go = itGo->second.Cast<GameObject>();
        if (!goJson.contains("Components"))
            continue;

        const nlohmann::json& components = goJson["Components"];

        for (const auto& compJson : components)
        {
            if (!compJson.contains("Type"))
                continue;

            std::string typeName = compJson["Type"].get<std::string>();
            if (typeName != "RigidBodyComponent")
                continue;

            bool isMissing = false;
            ObjPtr<Component> comp = CreateComponentForDeserializePrefab(compJson, go, isMissing, ctx);
            if (!comp.IsValid())
                continue;

            if (!isMissing && compJson.contains("Props"))
            {
                PendingComponentProps pending;
                pending.comp = comp;
                pending.props = &compJson["Props"];
                pendingComponentProps.push_back(std::move(pending));
            }
        }

        for (const auto& compJson : components)
        {
            if (!compJson.contains("Type"))
                continue;

            std::string typeName = compJson["Type"].get<std::string>();
            if (typeName == "Transform" || typeName == "RectTransform" || typeName == "RigidBodyComponent")
                continue;

            bool isMissing = false;
            ObjPtr<Component> comp = CreateComponentForDeserializePrefab(compJson, go, isMissing, ctx);
            if (!comp.IsValid())
                continue;

            if (!isMissing && compJson.contains("Props"))
            {
                PendingComponentProps pending;
                pending.comp = comp;
                pending.props = &compJson["Props"];
                pendingComponentProps.push_back(std::move(pending));
            }
        }
    }

    // 3-pass: 컴포넌트 프로퍼티 복원
    for (auto& pending : pendingComponentProps)
    {
        if (!pending.comp.IsValid() || pending.comp->IsDestroyed())
            continue;

        if (!pending.props)
            continue;

        DeserializeObjectPrefab(*pending.comp, *pending.props, ctx);
    }

    // 4-pass: Parent 연결
    for (auto& [childTrMUID, parentTrMUID] : pendingParent)
    {
        auto itChild = ctx.objectTable.find(childTrMUID);
        auto itParent = ctx.objectTable.find(parentTrMUID);
        if (itChild == ctx.objectTable.end() || itParent == ctx.objectTable.end())
            continue;

        auto childTr = itChild->second.Cast<Transform>();
        auto parentTr = itParent->second.Cast<Transform>();
        childTr->SetParent(parentTr, false);
    }

    SerializableEvent::SetResolver([](const Utility::MUID& muid) { return ObjectManager::Get().GetObjectByMUID(muid); });
    SerializableEventT<float>::SetResolver([](const Utility::MUID& muid) { return ObjectManager::Get().GetObjectByMUID(muid); });

    if (!rootMuid.empty())
    {
        auto itRoot = ctx.objectTable.find(rootMuid);
        if (itRoot != ctx.objectTable.end())
            return itRoot->second.Cast<GameObject>();
    }

    return ObjPtr<GameObject>();
}

void MMMEngine::Object::DontDestroyOnLoad(const ObjPtrBase& objPtr)
{
	// GameObject인 경우 그 자체를 씬에게 넘기기
	if (auto go = ObjectManager::Get().GetPtr<Object>(objPtr.GetPtrID(), objPtr.GetPtrGeneration()).Cast<GameObject>())
	{
		// 이미 파괴되었거나 이미 DontDestroyOnLoad 씬에 있으면 처리하지 않음
		if (go->IsDestroyed() || go->GetScene().id_DDOL)
			return;

		// 부모가 있는 경우 부모를 끊기
		go->GetTransform()->SetParent(nullptr);

		std::vector<ObjPtr<GameObject>> gameObjectsToProcess;
		gameObjectsToProcess.push_back(go);

		// BFS (너비 우선 탐색) 방식으로 계층 구조를 순회하여 스택 오버플로우 방지
		while (!gameObjectsToProcess.empty())
		{
			ObjPtr<GameObject> currentGo = gameObjectsToProcess.back();
			gameObjectsToProcess.pop_back();

			// 이미 처리했거나 파괴되었거나 DontDestroyOnLoad 씬에 있으면 건너뜀
			if (currentGo->IsDestroyed() || currentGo->GetScene().id_DDOL)
				continue;

			// 자신을 현재 씬에서 해제하고 DontDestroyOnLoad 씬에 등록
			if (auto sceneRaw = SceneManager::Get().GetSceneRaw(currentGo->GetScene())) // 씬이 유효한지 확인
			{
				sceneRaw->UnRegisterGameObject(currentGo);
			}
			SceneManager::Get().RegisterGameObjectToDDOL(currentGo);

			// 자식들을 큐에 추가하여 다음 반복에서 처리
			for (size_t i = 0; i < currentGo->GetTransform()->GetChildCount(); ++i)
			{
				if (auto childGo = currentGo->GetTransform()->GetChild(i)->GetGameObject())
				{
					gameObjectsToProcess.push_back(childGo);
				}
			}
		}
	}
}

void MMMEngine::Object::Destroy(const ObjPtrBase& objPtr, float delay)
{
	if (ObjectManager::Get().GetPtr<Object>(objPtr.GetPtrID(), objPtr.GetPtrGeneration()).Cast<Transform>())
	{
#ifdef _DEBUG
		assert(false && "Transform은 파괴할 수 없습니다.");
#endif
		return;
	}

	ObjectManager::Get().Destroy(objPtr, delay);
}
