//#pragma once
//#include "rttr/type.h"
//#include "json/json.hpp"
//
//using namespace nlohmann::json_abi_v3_12_0;
//
//#include <type_traits>
//
//using namespace rttr;
//
//namespace MMMEngine
//{
//	class Object;
//    class ObjectSerializer
//    {
//    private:
//        // 복원 컨텍스트
//        std::unordered_map<std::string, variant> m_guidToHandleMap;
//
//        struct PendingReference
//        {
//            variant ownerHandle;        // ObjectPtr<Object>
//            rttr::property prop;
//            std::string targetGUID;
//
//            // 컨테이너 내부 참조용
//            bool isContainerElement = false;
//            size_t containerIndex = 0;
//        };
//
//        std::vector<PendingReference> m_pendingReferences;
//
//        // === 직렬화 헬퍼 ===
//        std::optional<json> SerializeVariant(const variant& var);
//        std::optional<json> SerializeObjectPtr(const variant& var);
//        std::optional<json> SerializeSequentialContainer(const variant& var);
//
//        // === 역직렬화 헬퍼 ===
//        bool DeserializeVariant(const json& j, variant& var);
//        bool DeserializeObjectPtrProperty(const json& j, const variant& ownerHandle, rttr::property prop);
//        bool DeserializeSequentialContainer(const json& j, variant& containerVar, const variant& ownerHandle, rttr::property prop);
//
//    public:
//        json Serialize(const variant& objectHandle);
//
//        bool Deserialize(const json& j, const variant& objectHandle);
//
//        // 복원 컨텍스트 관리
//        void BeginRestoration();
//        void RegisterRestoredObject(const std::string& guid, ObjectPtrBase* handle);
//        void ResolvePendingReferences();
//        void EndRestoration();
//    };
//}
