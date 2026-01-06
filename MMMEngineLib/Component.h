#pragma once
#include "Object.h"
#include "rttr/type"

namespace MMMEngine
{
	class Component : public Object
	{
	private:
		RTTR_ENABLE(MMMEngine::Object)
		RTTR_REGISTRATION_FRIEND
		friend class App;
		friend class ObjectManager;
		friend class GameObject;

		ObjectPtr<GameObject> m_owner;
	protected:
		Component() = default;
	public:
		virtual ~Component() = default;
	};
}