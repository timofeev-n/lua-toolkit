//◦ Playrix ◦
#pragma once

#include <runtime/com/ianything.h>
#include <runtime/meta/classinfo.h>
#include <runtime/network/stream.h>
#include <lua-toolkit/debug/debugsessioncontroller.h>

#include <string>
#include <vector>


namespace Lua::Remoting {

class ABSTRACT_TYPE RemoteController : public Runtime::IRefCounted
{
public:

	using Ptr = Runtime::ComPtr<RemoteController>;

	struct DebugLocation
	{
		std::string id;
		std::string description;

#pragma region Class info
		CLASS_INFO(
			CLASS_FIELDS(
				CLASS_FIELD(id),
				CLASS_FIELD(description)
			)
		)
#pragma endregion
	};

#pragma region Class info
		CLASS_INFO(
			CLASS_METHODS(
				CLASS_METHOD(RemoteController, GetDebugLocations)
			)
		)
#pragma endregion


	Runtime::Async::Task<> Run();

	virtual ~RemoteController() = default;

	virtual std::vector<DebugLocation> GetDebugLocations() const = 0;

	virtual Runtime::Async::Task<Runtime::Debug::DebugSessionController::Ptr> CreateDebugSession(std::string id) = 0;

private:

	Runtime::Async::Task<> SpawnClientSession(Runtime::Network::Stream::Ptr client);
};


} // namespace Lua::Remoting
