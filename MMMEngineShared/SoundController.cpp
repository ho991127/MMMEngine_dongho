#include "rttr/registration.h"
#include "SoundController.h"

RTTR_REGISTRATION
{
	using namespace rttr;
	using namespace MMMEngine;

	registration::class_<SoundController>("SoundController")
		(rttr::metadata("wrapper_type_name", "ObjPtr<SoundController>"));
	registration::class_<ObjPtr<SoundController>>("ObjPtr<SoundController>")
		.constructor<>(
			[]() {
				return Object::NewObject<SoundController>();
			}).method("Inject", &ObjPtr<SoundController>::Inject);
}
