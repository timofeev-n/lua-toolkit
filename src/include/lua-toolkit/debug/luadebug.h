//◦ Playrix ◦
#pragma once
#include <lua-toolkit/debug/debugsessioncontroller.h>
#include <lua-toolkit/debug/debugsession.h>
#include <runtime/async/scheduler.h>
#include <runtime/com/weakcomptr.h>

#if ! __has_include(<lua.h>)
#error lua not configured to be used with current project
#endif

extern "C" {
#include <lua.h>
}

namespace Lua::Debug {

// DebugSessionController::Ptr CreateLuaDebugController(lua_State* lua, Runtime::Async::Scheduler::Ptr scheduler);

class LuaDebugSessionController : public Runtime::Debug::DebugSessionController
{
	CLASS_INFO(
		CLASS_BASE(Runtime::Debug::DebugSessionController)
	)

public:

	enum class StartMode
	{
		Unknown,
		Launch,
		Attach
	};


	LuaDebugSessionController();

	~LuaDebugSessionController();

protected:

	Runtime::Async::Task<> ConfigureLaunch(Runtime::RuntimeValue::Ptr config) override;

	Runtime::Async::Task<> ConfigureAttach(Runtime::RuntimeValue::Ptr config) override;

	Runtime::Async::Task<> Disconnect() override;

	void EnableDebug();

	void DisableDebug();

	virtual lua_State* GetLua() const = 0;

	virtual Runtime::Async::Task<> Start(StartMode) = 0;

private:

	void SetSession(Runtime::ComPtr<Runtime::Debug::DebugSession> session) override final;

	Runtime::Async::Task<> ConfigurationDone() override final ;

	Runtime::Async::Task<std::vector<Runtime::Dap::Breakpoint>> SetBreakpoints(Runtime::Dap::SetBreakpointsArguments) override final;

	Runtime::Async::Task<std::vector<Runtime::Dap::Thread>> GetThreads() override final;

	void ExecuteDebugger(lua_State*, lua_Debug*);


	class SourceBp
	{
	public:
		SourceBp(unsigned, Runtime::Dap::SourceBreakpoint);

		unsigned Id() const;

		const Runtime::Dap::SourceBreakpoint& Bp() const;

	private:

		unsigned _id;
		Runtime::Dap::SourceBreakpoint _bp;
	};


	StartMode _startMode = StartMode::Unknown;
	std::vector<SourceBp> _sourceBreakpoints;
	unsigned _bpId = 0;

	Runtime::WeakComPtr<Runtime::Debug::DebugSession> _sessionRef;
};


}
