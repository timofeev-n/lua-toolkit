//◦ Playrix ◦
#include "lua-toolkit/debug/debugsession.h"
#include "inplaceexecutionscheduler.h"

#include <runtime/com/comclass.h>
#include <runtime/serialization/json.h>
#include <runtime/serialization/runtimevaluebuilder.h>
#include <runtime/utils/strings.h>
#include <runtime/runtime/runtime.h>

namespace Runtime::Debug {

using namespace Runtime::Async;



/**
*
*/
class DebugSessionImpl final : public DebugSession
{
	COMCLASS_(DebugSession)

public:
	DebugSessionImpl(DapMessageStream::Ptr commandsStream, DebugSessionController::Ptr controller): _commandsStream(std::move(commandsStream)), _controller(std::move(controller))
	{
		Assert(_commandsStream);
		Assert(_controller);

		// _controller->SetMessageStream(_commandsStream);
	}

private:

	struct StoppedExectionState
	{
		Scheduler::Ptr scheduler;
		StackTraceProvider::Ptr stackTraceProvider;

		StoppedExectionState(Scheduler::Ptr scheduler_, StackTraceProvider::Ptr stackTraceProvider_): scheduler(std::move(scheduler_)), stackTraceProvider(std::move(stackTraceProvider_))
		{}
	};


	Task<> Run() override {

		_controller->SetSession(Com::Acquire{static_cast<DebugSession*>(this)});

		co_await RuntimeCore::instance().poolScheduler();

		do {
			const ReadonlyDynamicObject message {co_await _commandsStream->GetDapMessage()};

			if (!message) {
				break;
			}

			const std::string messageType = message.Get<std::string>("type");

			if (messageType == Dap::ProtocolMessage::MessageRequest) {

				auto request = RuntimeValueCast<Dap::RequestMessage>(message);

				Dap::GenericResponseMessage<> response(NextSeqId(), request);

				try
				{
					if (Strings::icaseEqual(request.command, "launch"))
					{
						co_await _controller->ConfigureLaunch(request.arguments);
					}
					else if (Strings::icaseEqual(request.command, "attach"))
					{
						co_await _controller->ConfigureAttach(request.arguments);
					}
					else if (Strings::icaseEqual(request.command, "configurationDone"))
					{
						co_await _controller->ConfigurationDone();
					}
					else if (Strings::icaseEqual(request.command, "disconnect"))
					{
						_isClosed = true;
						co_await _controller->Disconnect();
					}
					else if (Strings::icaseEqual(request.command, "setBreakpoints"))
					{
						auto args = RuntimeValueCast<Dap::SetBreakpointsArguments>(request.arguments);

						Dap::SetBreakpointsResponseBody body;
						body.breakpoints = co_await _controller->SetBreakpoints(std::move(args));
						response.body = runtimeValueCopy(std::move(body));
					}
					else if (Strings::icaseEqual(request.command, "setFunctionBreakpoints"))
					{

					}
					else if (Strings::icaseEqual(request.command, "threads"))
					{
						Dap::ThreadsResponseBody body;
						body.threads = co_await _controller->GetThreads();
						response.body = runtimeValueCopy(std::move(body));
					}
					else if (Strings::icaseEqual(request.command, "stackTrace"))
					{
						Assert(_stoppedState);

						auto args = RuntimeValueCast<Dap::StackTraceArguments>(request.arguments);

						auto stack = co_await Async::run([](StackTraceProvider& stackTraceProvider, Dap::StackTraceArguments args) -> Dap::StackTraceResponseBody {

							return stackTraceProvider.GetStackTrace(args);

						}, _stoppedState->scheduler, std::ref(*_stoppedState->stackTraceProvider), std::move(args));

						response.body = runtimeValueCopy(std::move(stack));
					}
					else if (Strings::icaseEqual(request.command, "scopes"))
					{
						Assert(_stoppedState);

						const auto args = RuntimeValueCast<Dap::ScopesArguments>(request.arguments);

						Dap::ScopesResponseBody body;

						body.scopes = co_await Async::run([](StackTraceProvider& stackTraceProvider, unsigned frameId) -> std::vector<Dap::Scope> {

							return stackTraceProvider.GetScopes(frameId);

						}, _stoppedState->scheduler, std::ref(*_stoppedState->stackTraceProvider), args.frameId);

						response.body = runtimeValueCopy(std::move(body));
					}
					else if (Strings::icaseEqual(request.command, "variables"))
					{
						Assert(_stoppedState);

						const auto args = RuntimeValueCast<Dap::VariablesArguments>(request.arguments);

						Dap::VariablesResponseBody body;

						body.variables = co_await Async::run([](StackTraceProvider& stackTraceProvider, Dap::VariablesArguments args) -> std::vector<Dap::Variable> {

							return stackTraceProvider.GetVariables(args);

						}, _stoppedState->scheduler, std::ref(*_stoppedState->stackTraceProvider), args);

						response.body = runtimeValueCopy(std::move(body));
					}
					else if (Strings::icaseEqual(request.command, "pause"))
					{

					}
					else if (Strings::icaseEqual(request.command, "continue"))
					{

					}
					else if (Strings::icaseEqual(request.command, "next"))
					{

					}
					else if (Strings::icaseEqual(request.command, "stepOut"))
					{

					}
				}
				catch (const std::exception& exception) {
					_isClosed = true;
					response.SetError(exception.what());
				}

				co_await _commandsStream->SendDapMessage(runtimeValueRef(response));
			}
			else {
			}
		}
		while (!_isClosed);
	}

	bool PauseIsRequested() const override {
		return false;
	}

	DapMessageStream& GetCommandsStream() const override {
		Assert(_commandsStream);

		return *_commandsStream;
	}


	ExecutionMode StopExecution(Dap::StoppedEventBody ev, StackTraceProvider::Ptr stackTraceProvider) override {
		Assert(!_stoppedState);
		Assert(stackTraceProvider);

		auto scheduler = Com::createInstance<Async::InplaceExecutionScheduler>();

		_stoppedState.emplace(scheduler, std::move(stackTraceProvider));

		Dap::GenericEventMessage<Dap::StoppedEventBody> eventMessage(NextSeqId(), "stopped");
		eventMessage.body = std::move(ev);

		_commandsStream->SendDapMessage(runtimeValueCopy(std::move(eventMessage))).detach();

		scheduler->Execute();

		return ExecutionMode::Continue;
	}

	unsigned NextSeqId() {
		return _seqId.fetch_add(1);
	}


	DapMessageStream::Ptr _commandsStream;
	DebugSessionController::Ptr _controller;
	std::atomic<unsigned> _seqId{1ui32};
	bool _isClosed = false;

	std::optional<StoppedExectionState> _stoppedState;
};


/* -------------------------------------------------------------------------- */
DebugSession::Ptr DebugSession::Create(DapMessageStream::Ptr commandsStream, DebugSessionController::Ptr controller) {
	return Com::createInstance<DebugSessionImpl, DebugSession>(std::move(commandsStream), std::move(controller));
}

}
