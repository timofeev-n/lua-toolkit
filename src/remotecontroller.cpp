//◦ Playrix ◦
#include "lua-toolkit/remotecontroller.h"
#include "lua-toolkit/debug/debugsession.h"

#include <runtime/com/comclass.h>
#include <runtime/network/server.h>
#include <runtime/serialization/runtimevaluebuilder.h>


#include "remoting/httpstream.h"

namespace Lua::Remoting {

using namespace Runtime;
using namespace Runtime::Async;
using namespace Runtime::Debug;

struct HandshakeResponse
{
	bool success = true;

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(success)
		)
	)
};


Runtime::Async::Task<> RemoteController::SpawnClientSession(Runtime::Network::Stream::Ptr client) {

	HttpStream streamReader;

	auto packet = co_await HttpStream::ReadHttpPacket(streamReader, *client);
	if (!packet) {
		co_return;
	}

	co_await HttpStream::SendHttpJsonPacket(runtimeValueCopy(HandshakeResponse{}), "/dap", *client);

	auto debugController = co_await this->CreateDebugSession("default");

	if (!debugController) {
		co_return;
	}

	auto debugSession = DebugSession::Create(DapMessageStream::Create(client), std::move(debugController));

	co_await debugSession->Run();
}



Task<> RemoteController::Run() {
	auto server = co_await Network::Server::listen("tcp://:8845");

	while (true) {
		auto client = co_await server->accept();
	
		if (!client) {
			co_return;
		}

		auto task = SpawnClientSession(std::move(client));
		task.detach();
	}
}

} // namespace Lua::Remoting



#if 0
class RemoteControllerImpl final : public RemoteController
{
	COMCLASS_(RemoteController)

public:

	RemoteControllerImpl() {
		_runTask = Run();
	}


private:


	Task<> Run() {
		
		auto server = co_await Network::Server::listen("tcp://:8845");

		auto client = co_await server->accept();
	
		if (!client) {
			co_return;
		}

		auto task = ClientSession(std::move(client));
		task.detach();
	}

	




	std::vector<DebugLocation> GetDebugLocations() const override {
		return {};
	}


	Task<> _runTask;

};




RemoteController::Ptr RemoteController::Create() {
	return Com::createInstance<RemoteControllerImpl, RemoteController>();
}
}


#endif

