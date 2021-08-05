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

#include <memory>
#include <mutex>
#include <optional>

namespace Lua::Debug {

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


	class SourceBp
	{
	public:
		SourceBp() noexcept = default;

		SourceBp(unsigned bpId, unsigned sourceId, Runtime::Dap::SourceBreakpoint) noexcept;

		unsigned Id() const;

		unsigned SourceId() const;

		const Runtime::Dap::SourceBreakpoint& Bp() const;

		const Runtime::Dap::Source& GetSource(const LuaDebugSessionController&) const;

	private:
		unsigned _id;
		unsigned _sourceId;
		Runtime::Dap::SourceBreakpoint _bp;
	};


	class FunctionBp
	{
	public:
		FunctionBp() noexcept = default;

		FunctionBp(unsigned, Runtime::Dap::FunctionBreakpoint) noexcept;

		unsigned Id() const;

		const Runtime::Dap::FunctionBreakpoint& Bp() const;

	private:
		unsigned _id;
		Runtime::Dap::FunctionBreakpoint _bp;
	};


	class SourceEntry
	{
	public:
		SourceEntry() noexcept = default;

		SourceEntry(unsigned, Runtime::Dap::Source&&) noexcept;

		unsigned Id() const;

		const Runtime::Dap::Source& GetSource() const;

	private:

		unsigned _id;
		Runtime::Dap::Source _sourceInfo;
	};

	
	struct ABSTRACT_TYPE DebugStepPredicate
	{
		using Ptr = std::unique_ptr<DebugStepPredicate>;

		virtual std::optional<Runtime::Dap::StoppedEventBody> GetStopped(lua_State*, lua_Debug*) = 0;
	};

	class StepPredicate;

	class StepOutPredicate;



	void SetSession(Runtime::ComPtr<Runtime::Debug::DebugSession> session) override final;

	unsigned GetSourceId(Runtime::Dap::Source& source);

	const Runtime::Dap::Source& GetSource(unsigned sourceId) const;

	Runtime::Async::Task<> ConfigurationDone() override final ;

	Runtime::Async::Task<std::vector<Runtime::Dap::Breakpoint>> SetBreakpoints(Runtime::Dap::SetBreakpointsArguments) override final;

	Runtime::Async::Task<std::vector<Runtime::Dap::Breakpoint>> SetFunctionBreakpoints(Runtime::Dap::SetFunctionBreakpointsArguments) override final;

	Runtime::Async::Task<std::vector<Runtime::Dap::Thread>> GetThreads() override final;

	void ExecuteDebugger(lua_State*, lua_Debug*);

	std::optional<Runtime::Dap::StoppedEventBody> CheckBreakpoints(lua_State*, lua_Debug*);


	StartMode _startMode = StartMode::Unknown;
	Runtime::WeakComPtr<Runtime::Debug::DebugSession> _sessionRef;
	std::vector<SourceBp> _sourceBreakpoints;
	std::vector<FunctionBp> _functionBreakpoints;
	std::vector<SourceEntry> _sources;
	DebugStepPredicate::Ptr _debugStepPredicate;

	unsigned _bpId = 0;
	unsigned _srcId = 0;
	bool _isActive = false;
	std::mutex _mutex;
};


}
