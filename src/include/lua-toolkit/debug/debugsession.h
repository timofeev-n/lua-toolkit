//◦ Playrix ◦
#pragma once
#include <lua-toolkit/debug/adapterprotocol.h>
#include <lua-toolkit/debug/dapmessagestream.h>
#include <lua-toolkit/debug/debugsessioncontroller.h>
#include <lua-toolkit/debug/stacktraceprovider.h>

#include <variant>

namespace Runtime::Debug {


enum class ContinueExecutionMode
{
	Continue,
	Step,
	StepIn,
	StepOut,
	Stopped
};


struct ABSTRACT_TYPE DebugSession : IRefCounted
{
	using Ptr = ComPtr<DebugSession>;



	static DebugSession::Ptr Create(DapMessageStream::Ptr, DebugSessionController::Ptr);

	virtual Async::Task<> Run();

	virtual bool PauseIsRequested() const = 0;

	virtual DapMessageStream& GetCommandsStream() const = 0;

	virtual ContinueExecutionMode StopExecution(Dap::StoppedEventBody ev, StackTraceProvider::Ptr) = 0;

};

} // namespace Runtime::Debug
