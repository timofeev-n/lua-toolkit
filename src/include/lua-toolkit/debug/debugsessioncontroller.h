//◦ Playrix ◦
#pragma once
#include <lua-toolkit/debug/adapterprotocol.h>
#include <runtime/com/comptr.h>
#include <runtime/async/task.h>

namespace Runtime::Debug {

/**
* 
*/
struct ABSTRACT_TYPE DebugSessionController : IRefCounted
{
	using Ptr = ComPtr<DebugSessionController>;

	virtual void SetSession(ComPtr<struct DebugSession> session) = 0;

	virtual Async::Task<> ConfigureLaunch(RuntimeValue::Ptr) = 0;

	virtual Async::Task<> ConfigureAttach(RuntimeValue::Ptr) = 0;

	virtual Async::Task<> ConfigurationDone() = 0;

	virtual Async::Task<> Disconnect() = 0;

	virtual Async::Task<std::vector<Dap::Breakpoint>> SetBreakpoints(Dap::SetBreakpointsArguments) = 0;

	virtual Async::Task<std::vector<Dap::Breakpoint>> SetFunctionBreakpoints(Dap::SetFunctionBreakpointsArguments) = 0;

	virtual Async::Task<std::vector<Dap::Thread>> GetThreads() = 0;
};

} // namespace Runtime::Debug
