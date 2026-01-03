#include "Object.h"
#include "rttr/registration"
#include "rttr/detail/policies/ctor_policies.h"
#include "ObjectManager.h"
#include <cassert>

uint64_t MMMEngine::Object::s_nextInstanceID = 1;


RTTR_REGISTRATION
{
	using namespace rttr;
	using namespace MMMEngine;

	registration::class_<Object>("Object")
		.property("Name", &Object::GetName, &Object::SetName)
		.property("GUID", &Object::GetGUID, &Object::SetGUID)
		.property_readonly("InstanceID", &Object::GetInstanceID)
		.property_readonly("isDestroyed", &Object::IsDestroyed);

	registration::class_<ObjectPtrBase>("ObjectPtr")
		.method("IsValid", &ObjectPtrBase::IsValid)
		.method("Get", &ObjectPtrBase::GetRaw)
		.method("GetHandleID", &ObjectPtrBase::GetHandleID)
		.method("GetGeneration", &ObjectPtrBase::GetGeneration);

	registration::class_<ObjectPtr<Object>>("ObjectPtr<Object>")
		.constructor<>(
			[]() {
				return Object::CreateInstance<Object>();
			}, registration::protected_access);
}

MMMEngine::Object::Object() : m_instanceID(s_nextInstanceID++)
{
#ifdef _DEBUG
	if (!ObjectManager::Get()->IsCreatingObject())
	{
		assert(false && "Object는 ObjectManager/CreateInstance로만 생성할 수 있습니다.");
		std::abort();
	}
#endif
	m_name = "<Unnamed> [ Instance ID : " + std::to_string(m_instanceID) + " ]";
	m_guid = GUID::NewGuid();
}

MMMEngine::Object::~Object()
{
#ifdef _DEBUG
	assert(ObjectManager::Get()->IsDestroyingObject() && "Object는 ObjectManager/Destroy로만 파괴할 수 있습니다.");
#endif
}

