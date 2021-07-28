//◦ Playrix ◦
#include "lua-toolkit/debug/debugsession.h"

#include <runtime/com/comclass.h>
#include <runtime/serialization/json.h>
#include <runtime/serialization/runtimevaluebuilder.h>
#include <runtime/utils/strings.h>

namespace Lua::Debug {

using namespace Runtime;
using namespace Runtime::Async;

/**
* 
*/
struct RuntimeDictionaryWrapper
{
	RuntimeReadonlyDictionary::Ptr dict;

	RuntimeDictionaryWrapper(RuntimeValue::Ptr value): dict(std::move(value))
	{}

	explicit operator bool () const {
		return static_cast<bool>(dict);
	}

	operator RuntimeValue::Ptr () const {
		return dict;
	}

	bool has(std::string_view key) const {
		return dict && dict->hasKey(key);
	}

	std::string getString(std::string_view key) const {
		Assert(dict);
		auto value = dict->value(key);
		Assert2(value, Core::Format::format("Named field ({}) does not exists", key));
		
		auto strValue = value->as<const RuntimeStringValue*>();
		Assert2(strValue, Core::Format::format("Field ({}) is not a string", key));

		return strValue->getUtf8();
	}
};

/**
*
*/
class DebugSessionImpl final : public DebugSession
{
	COMCLASS_(DebugSession)

public:
	DebugSessionImpl(CommandsStream::Ptr commandsStream, DebugSessionController::Ptr controller): _commandsStream(std::move(commandsStream)), _controller(std::move(controller))
	{
		Assert(_commandsStream);
		Assert(_controller);
	}

private:

	Task<> Run() override {

		do {

			const RuntimeDictionaryWrapper message {co_await _commandsStream->GetFrontendCommand()};
			if (!message) {
				break;
			}

			const std::string messageType = message.getString("type");

			if (messageType == Dap::ProtocolMessage::MessageRequest) {

				auto request = RuntimeValueCast<Dap::RequestMessage>(message);

				Dap::GenericResponse response(NextSeqId(), request);

				try {
					if (Strings::icaseEqual(request.command, "launch")) {

					}
					else if (Strings::icaseEqual(request.command, "disconnect")) {
						_isClosed = true;
						co_await _controller->Disconnect();
					}
					else if (Strings::icaseEqual(request.command, "setBreakpoints")) {

						auto args = RuntimeValueCast<Dap::SetBreakpointsArguments>(request.arguments);

						Dap::SetBreakpointsResponseBody body;
						body.breakpoints = co_await _controller->SetBreakpoints(std::move(args));

						response.body = runtimeValueCopy(std::move(body));// runtimeValueCopy(std::move(breakpoints));

					}
				}
				catch (const std::exception& exception) {
					_isClosed = true;
					response.SetError(exception.what());
				}

				co_await _commandsStream->SendFrontendCommand(runtimeValueRef(response));
			}
			else {
			}
		}
		while (!_isClosed);
	}

	CommandsStream& GetCommandsStream() const override {
		Assert(_commandsStream);

		return *_commandsStream;
	}

	unsigned NextSeqId() {
		return _seqId.fetch_add(1);
	}


	CommandsStream::Ptr _commandsStream;
	DebugSessionController::Ptr _controller;
	std::atomic<unsigned> _seqId{1ui32};
	bool _isClosed = false;
};


/* -------------------------------------------------------------------------- */
DebugSession::Ptr DebugSession::Create(CommandsStream::Ptr commandsStream, DebugSessionController::Ptr controller) {
	return Com::createInstance<DebugSessionImpl, DebugSession>(std::move(commandsStream), std::move(controller));
}

}
