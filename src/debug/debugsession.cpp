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
	DebugSessionImpl(DapMessageStream::Ptr commandsStream, DebugSessionController::Ptr controller): _messageStream(std::move(commandsStream)), _controller(std::move(controller))
	{
		Assert(_messageStream);
		Assert(_controller);

		// _controller->SetMessageStream(_messageStream);
	}

private:

	struct StoppedExectionState
	{
		Scheduler::Ptr scheduler;
		StackTraceProvider::Ptr stackTraceProvider;
		std::optional<ContinueExecutionMode> continueMode;

		StoppedExectionState(Scheduler::Ptr scheduler_, StackTraceProvider::Ptr stackTraceProvider_): scheduler(std::move(scheduler_)), stackTraceProvider(std::move(stackTraceProvider_))
		{}
	};


	Task<> Run() override {

		_controller->SetSession(Com::Acquire{static_cast<DebugSession*>(this)});

		co_await RuntimeCore::instance().poolScheduler();

		do {
			const ReadonlyDynamicObject message {co_await _messageStream->GetDapMessage()};

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

						if (_stoppedState) {
							_stoppedState->continueMode = ContinueExecutionMode::Stopped;
							_stoppedState->scheduler->as<Disposable&>().dispose();
						}

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
						auto args = RuntimeValueCast<Dap::SetFunctionBreakpointsArguments>(request.arguments);

						Dap::SetBreakpointsResponseBody body;
						body.breakpoints = co_await _controller->SetFunctionBreakpoints(std::move(args));
						response.body = runtimeValueCopy(std::move(body));
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
						Assert(_stoppedState);
						Assert(!_stoppedState->continueMode);

						_stoppedState->continueMode = ContinueExecutionMode::Continue;
						_stoppedState->scheduler->as<Disposable&>().dispose();
					}
					else if (Strings::icaseEqual(request.command, "next"))
					{
						Assert(_stoppedState);
						Assert(!_stoppedState->continueMode);

						_stoppedState->continueMode = ContinueExecutionMode::Step;
						_stoppedState->scheduler->as<Disposable&>().dispose();
					}
					else if (Strings::icaseEqual(request.command, "stepIn"))
					{
						Assert(_stoppedState);
						Assert(!_stoppedState->continueMode);

						_stoppedState->continueMode = ContinueExecutionMode::StepIn;
						_stoppedState->scheduler->as<Disposable&>().dispose();
					}
					else if (Strings::icaseEqual(request.command, "stepOut"))
					{
						Assert(_stoppedState);
						Assert(!_stoppedState->continueMode);

						_stoppedState->continueMode = ContinueExecutionMode::StepOut;
						_stoppedState->scheduler->as<Disposable&>().dispose();
					}
				}
				catch (const std::exception& exception) {
					_isClosed = true;
					response.SetError(exception.what());
				}

				co_await _messageStream->SendDapMessage(runtimeValueRef(response));
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
		Assert(_messageStream);

		return *_messageStream;
	}


	ContinueExecutionMode StopExecution(Dap::StoppedEventBody ev, StackTraceProvider::Ptr stackTraceProvider) override {
		SCOPE_Leave {
			_stoppedState.reset();
		};

		Assert(!_stoppedState);
		Assert(stackTraceProvider);

		auto scheduler = Com::createInstance<Async::InplaceExecutionScheduler>();

		_stoppedState.emplace(scheduler, std::move(stackTraceProvider));

		Dap::GenericEventMessage<Dap::StoppedEventBody> eventMessage(NextSeqId(), "stopped");
		eventMessage.body = std::move(ev);

		_messageStream->SendDapMessage(runtimeValueCopy(std::move(eventMessage))).detach();

		scheduler->Execute();

		Assert(_stoppedState && _stoppedState->continueMode);

		return *_stoppedState->continueMode;
	}

	unsigned NextSeqId() {
		return _seqId.fetch_add(1);
	}


	DapMessageStream::Ptr _messageStream;
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
