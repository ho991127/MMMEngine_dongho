#pragma once
#include "Object.h"
#include "rttr/type"
#include <vector>

namespace MMMEngine
{
	class Transform;
	class Component;
	class GameObject : public Object
	{
	private:
		RTTR_ENABLE(MMMEngine::Object)
		RTTR_REGISTRATION_FRIEND
		ObjectPtr<Transform> m_transform;
		std::vector<ObjectPtr<Component>> m_components;
		friend class App;
		friend class ObjectManager;
		friend class Scene;
	protected:
		GameObject() = default;
		GameObject(std::string name);
	public:
		virtual ~GameObject() = default;
	};
}