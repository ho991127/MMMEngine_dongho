#pragma once
#include "ObjectManager.h"
#include "Object.h"

namespace MMMEngine
{
    template<typename T, typename... Args>
    ObjectPtr<T> Object::CreatePtr(Args&&... args)
    {
        return ObjectManager::Get()
            .CreatePtr<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    bool ObjectPtr<T>::IsValid() const
    {
        return ObjectManager::Get().IsValidPtr(m_ptrID, m_ptrGeneration, m_raw);
    }

    template<typename T>
    bool ObjectPtr<T>::IsSameObject(const ObjectPtrBase& other) const
    {
        if (m_ptrID != other.GetPtrID() ||
            m_ptrGeneration != other.GetPtrGeneration())
            return false;

        return IsValid() &&
            ObjectManager::Get().IsValidPtr(
                other.GetPtrID(),
                other.GetPtrGeneration(),
                other.GetRaw()
            );
    }


    template<typename T>
    ObjectPtr<T> Object::FindObjectByType()
    {
        return ObjectManager::Get().FindObjectByType<T>();
    }

    template<typename T>
    std::vector<ObjectPtr<T>> Object::FindObjectsByType()
    {
        return ObjectManager::Get().FindObjectsByType<T>();
    }

    template<typename T>
    ObjectPtr<T> Object::SelfPtr(T* self)
    {
#ifndef NDEBUG
        static_assert(std::is_base_of_v<Object, T>, "SelfPtr<T> : T는 Object를 상속받아야합니다.");
        assert(static_cast<Object*>(self) == this && "SelfPtr의 인자가 자신이 아닙니다!");
#endif
        return ObjectManager::Get().GetPtrFast<T>(static_cast<T*>(this), m_ptrID, m_ptrGen);
    }
}