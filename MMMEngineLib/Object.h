#pragma once
#include "rttr/type"
#include "rttr/registration_friend.h"
#include "GUID.h"
#include <cassert>

namespace MMMEngine
{
	class Object
	{
	private:
		friend class ObjectManager;

		template<typename T>
		friend class ObjectPtr;
		
		RTTR_ENABLE()
		RTTR_REGISTRATION_FRIEND

		static uint64_t s_nextInstanceID;

		uint64_t		m_instanceID;
		std::string		m_name;
		GUID			m_guid;

		bool			m_isDestroyed = false;

		inline void		MarkDestory() { m_isDestroyed = true; }

		inline void		SetGUID(const GUID& guid) { m_guid = guid; }
	protected:
        Object();
        virtual ~Object();
	public:
		Object(const Object&) = delete;
		Object& operator=(const Object&) = delete;

		template<typename T, typename ...Args>
        static ObjectPtr<T> CreateInstance(Args && ...args);

        template<typename T>
		static void Destroy(ObjectPtr<T> objPtr);

		inline uint64_t				GetInstanceID() const { return m_instanceID; }

		inline const GUID&			GetGUID()		const { return m_guid; }

		inline const std::string&	GetName()		const { return m_name; }
		inline void					SetName(const std::string& name) { m_name = name; }

		inline const bool&			IsDestroyed()	const { return m_isDestroyed; }
	};
    
    class ObjectPtrBase
    {
    private:
        RTTR_ENABLE()
        RTTR_REGISTRATION_FRIEND

        template<typename T>
        friend class ObjectPtr;

        virtual Object* GetBase() const = 0;
    public:
        virtual uint32_t    GetHandleID() const = 0;
        virtual uint32_t    GetGeneration() const = 0;
        virtual bool        IsValid() const = 0;
        
        virtual bool IsSameObject(const ObjectPtrBase& other) const = 0;

        virtual bool operator==(const ObjectPtrBase& other) const = 0;
        virtual bool operator!=(const ObjectPtrBase& other) const = 0;
        virtual bool operator==(std::nullptr_t) const = 0;
        virtual bool operator!=(std::nullptr_t) const = 0;
    };

    template<typename T>
    class ObjectPtr final : public ObjectPtrBase
    {
    private:
        RTTR_ENABLE(ObjectPtrBase)
        RTTR_REGISTRATION_FRIEND
        friend class ObjectManager;
        friend class ObjectSerializer;

        T* m_ptr = nullptr;
        uint32_t m_handleID = UINT32_MAX;
        uint32_t m_handleGeneration = 0;

        virtual Object* GetBase() const override { return m_ptr; }

        T* Get() const
        {
            if (!IsValid())
                return nullptr;
            return m_ptr;
        }

        // private 생성자 - ObjectManager만 생성 가능
        ObjectPtr(T* ptr, uint32_t id, uint32_t gen)
            : m_ptr(ptr)
            , m_handleID(id)
            , m_handleGeneration(gen)
        {
        }

    public:
        // 기본 생성자 (null handle)
        ObjectPtr() = default;

        // 복사/이동은 허용
        ObjectPtr(const ObjectPtr&) = default;
        ObjectPtr(ObjectPtr&&) noexcept = default;
        ObjectPtr& operator=(const ObjectPtr&) = default;
        ObjectPtr& operator=(ObjectPtr&&) noexcept = default;

        T& operator*() const 
        {
            T* ptr = Get();
            assert(ptr && "ObjectPtr의 역참조가 잘못되었습니다!");
            return *ptr;
        }

        T* operator->() const
        {
            T* ptr = Get();
            assert(ptr && "유효하지 않은 ObjectPtr에 접근했습니다!");
            return ptr;
        }

        bool operator==(const ObjectPtr<T>& other) const
        {
            // 같은 핸들 슬롯 + 같은 세대 = 같은 핸들
            return m_handleID == other.m_handleID &&
                m_handleGeneration == other.m_handleGeneration;
        }

        bool operator!=(const ObjectPtr<T>& other) const
        {
            return !(*this == other);
        }

        virtual bool operator==(const ObjectPtrBase& other) const override
        {
            return m_handleID == other.GetHandleID() &&
                m_handleGeneration == other.GetGeneration();
        }

        virtual bool operator!=(const ObjectPtrBase& other) const override
        {
            return !(*this == other);
        }

        // === nullptr 비교 (null 핸들 검사) ===
        virtual bool operator==(std::nullptr_t) const override
        {
            return m_handleID == UINT32_MAX;  // null 핸들
        }

        virtual bool operator!=(std::nullptr_t) const override
        {
            return m_handleID != UINT32_MAX;
        }

        bool IsSameObject(const ObjectPtr<T>& other) const
        {
            // 같은 핸들이면 같은 객체
            if (m_handleID != other.m_handleID ||
                m_handleGeneration != other.m_handleGeneration)
                return false;

            return IsValid() && other.IsValid();
        }

        virtual bool IsSameObject(const ObjectPtrBase& other) const override
        {
            if (m_handleID != other.GetHandleID() ||
                m_handleGeneration != other.GetGeneration())
                return false;

            return IsValid() &&
                ObjectManager::Get().IsValidHandle(
                    other.GetHandleID(),
                    other.GetGeneration(),
                    other.GetBase()
                );
        }

        explicit operator bool() const { return IsValid(); }

        virtual bool IsValid() const override
        {
            return ObjectManager::Get().IsValidHandle(m_handleID, m_handleGeneration, m_ptr);
        }

        virtual uint32_t GetHandleID() const override { return m_handleID; }
        virtual uint32_t GetGeneration() const override { return m_handleGeneration; }
    };
}

#include "Object.inl"