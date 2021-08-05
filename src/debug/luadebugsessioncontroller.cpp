//◦ Playrix ◦
#include "luastacktraceprovider.h"
#include "lua-toolkit/debug/debugsessioncontroller.h"
#include "lua-toolkit/debug/luadebug.h"
#include <runtime/serialization/runtimevaluebuilder.h>
#include <runtime/com/comclass.h>
#include <runtime/runtime/runtime.h>
#include <runtime/threading/event.h>
#include <runtime/threading/lock.h>

namespace Lua::Debug {

using namespace Runtime;
using namespace Runtime::Async;
using namespace Runtime::Debug;

using namespace std::chrono_literals;

namespace {

std::tuple<int, int> GetCurrentLineAndStackDepth(lua_State* l, lua_Debug* ar) {

	int stackDepth = 0;

	lua_getstack(l, stackDepth, ar);
	lua_getinfo(l, "l", ar);

	const int line = ar->currentline;

	while (lua_getstack(l, stackDepth + 1, ar) != 0) {
		++stackDepth;
	}

	return {line, stackDepth};
}

} // namespace


/* -------------------------------------------------------------------------- */
class LuaDebugSessionController::StepPredicate final : public LuaDebugSessionController::DebugStepPredicate
{
public:

	StepPredicate(lua_State* l, lua_Debug* ar, bool allowStepIn = false): _allowStepIn(allowStepIn) {
		std::tie(_initialLine, _initialStackDepth) = GetCurrentLineAndStackDepth(l, ar);
	}


	std::optional<Runtime::Dap::StoppedEventBody> GetStopped(lua_State* l, lua_Debug* ar) override {

		if (ar->event != LUA_HOOKLINE) {
			return std::nullopt;
		}

		const auto [currentLine, currentStackDepth] = GetCurrentLineAndStackDepth(l, ar);

		const bool stop = 
			_allowStepIn ? _initialLine != currentLine :
			currentStackDepth < _initialStackDepth || (_initialStackDepth == currentStackDepth && _initialLine != currentLine)
			;

		if (stop) {
			Dap::StoppedEventBody ev("step", "Debug step");
			ev.threadId = 1;
			ev.allThreadsStopped = true;

			return ev;
		}
		
		return std::nullopt;
	}

private:

	int _initialLine = -1;
	int _initialStackDepth = -1;
	const bool _allowStepIn;
};


class LuaDebugSessionController::StepOutPredicate final : public LuaDebugSessionController::DebugStepPredicate
{
public:

	StepOutPredicate(int stackDepth): _initialStackDepth(stackDepth)
	{}

	std::optional<Runtime::Dap::StoppedEventBody> GetStopped(lua_State* l, lua_Debug* ar) override {
		if (ar->event != LUA_HOOKLINE) {
			return std::nullopt;
		}

		if (const auto [currentLine, currentStackDepth] = GetCurrentLineAndStackDepth(l, ar); currentStackDepth < _initialStackDepth) {
			Dap::StoppedEventBody ev("step", "Debug step");
			ev.threadId = 1;
			ev.allThreadsStopped = true;
			return ev;
		}

		return std::nullopt;
	}

private:
	const int _initialStackDepth;
};


/* -------------------------------------------------------------------------- */
LuaDebugSessionController::LuaDebugSessionController()
{}


LuaDebugSessionController::~LuaDebugSessionController()
{}


void LuaDebugSessionController::SetSession(ComPtr<DebugSession> session) {
	_sessionRef = std::move(session);
}


unsigned LuaDebugSessionController::GetSourceId(Runtime::Dap::Source& source) {

	auto existingSource = std::find_if(_sources.begin(), _sources.end(), [&source](const SourceEntry& entry) {
		return entry.GetSource() == source;
	});

	if (existingSource != _sources.end()) {
		return existingSource->Id();
	}

	return _sources.emplace_back(++_srcId, std::move(source)).Id();
}


const Dap::Source& LuaDebugSessionController::GetSource(unsigned sourceId) const {
	
	auto source = std::find_if(_sources.begin(), _sources.end(), [&sourceId](const SourceEntry& entry) { return entry.Id() == sourceId; });
	Assert(source != _sources.end());

	return source->GetSource();
}


Task<> LuaDebugSessionController::ConfigurationDone() {

	Assert(_startMode != StartMode::Unknown);

	this->Start(_startMode).detach();

	return Task<>::makeResolved();
}


Task<> LuaDebugSessionController::Disconnect(){
	return Task<>::makeResolved();
}


Task<std::vector<Dap::Breakpoint>> LuaDebugSessionController::SetBreakpoints(Dap::SetBreakpointsArguments arg) {

	lock_(_mutex);

	const unsigned sourceId = GetSourceId(arg.source);

	{
		// This method called every time when active breakpoints set is changed. That means it will be called when breakpoint is disabled or removed.
		// Just reset all breakpoints for specified source.
		auto iter = std::remove_if(_sourceBreakpoints.begin(), _sourceBreakpoints.end(), [sourceId](const SourceBp& bp) { return bp.SourceId() == sourceId;});
		if (const size_t removedCount = std::distance(iter, _sourceBreakpoints.end()); removedCount  > 0) {
			_sourceBreakpoints.resize(_sourceBreakpoints.size() - removedCount);
		}
	}

	std::vector<Dap::Breakpoint> breakpoints;

	for (Dap::SourceBreakpoint& srcBp : arg.breakpoints) {

		Dap::Breakpoint& bp = breakpoints.emplace_back();
		bp.id = ++_bpId;
		bp.verified = true;
		// bp.message = "Not implemented yet";
		// bp.source = std::move(bp.source);
		bp.line = bp.line;

		_sourceBreakpoints.emplace_back(*bp.id, sourceId, srcBp);
	}

	return Task<std::vector<Dap::Breakpoint>>::makeResolved(std::move(breakpoints));
}


Task<std::vector<Dap::Breakpoint>> LuaDebugSessionController::SetFunctionBreakpoints(Dap::SetFunctionBreakpointsArguments arg) {
	lock_(_mutex);

	std::vector<Dap::Breakpoint> breakpoints;

	_functionBreakpoints.clear();

	for (Dap::FunctionBreakpoint& funcBp : arg.breakpoints) {

		Dap::Breakpoint& bp = breakpoints.emplace_back();
		bp.id = ++_bpId;
		bp.verified = true;

		_functionBreakpoints.emplace_back(*bp.id, std::move(funcBp));
	}

	return Task<std::vector<Dap::Breakpoint>>::makeResolved(std::move(breakpoints));
}


Task<std::vector<Dap::Thread>> LuaDebugSessionController::GetThreads() {

	std::vector<Dap::Thread> threads;

	threads.emplace_back(1, "Default");

	return Task<std::vector<Dap::Thread>>::makeResolved(std::move(threads));
}


void LuaDebugSessionController::EnableDebug() {

	_isActive = true;

	lua_State* const l = GetLua();

	lua_pushlightuserdata(l, this);
	lua_setfield(l, LUA_GLOBALSINDEX, "__lua_DebuggerSession");

	lua_sethook(GetLua(), [](lua_State* l, lua_Debug* ar) noexcept {

		lua_getfield(l, LUA_GLOBALSINDEX, "__lua_DebuggerSession");
		LuaDebugSessionController* self  = reinterpret_cast<LuaDebugSessionController*>(lua_touserdata (l, -1));
		lua_pop(l, 1);
		if (self) {
			self->ExecuteDebugger(l, ar);
		}
	}
	, LUA_MASKRET | LUA_MASKCALL | LUA_MASKLINE, 0);
}


void LuaDebugSessionController::DisableDebug() {

	//lua_State* const l = GetLua();
	//lua_pushnil(l);
	//lua_setfield(l, LUA_GLOBALSINDEX, "__lua_DebuggerSession");

	_isActive = false;
	// lua_sethook(GetLua(), nullptr, 0, 0);
}


Task<> LuaDebugSessionController::ConfigureLaunch(RuntimeValue::Ptr config) {

	Assert(_startMode == StartMode::Unknown);
	_startMode = StartMode::Launch;

	return Task<>::makeResolved();
}


Task<> LuaDebugSessionController::ConfigureAttach(RuntimeValue::Ptr config) {

	Assert(_startMode == StartMode::Unknown);
	_startMode = StartMode::Attach;

	return Task<>::makeResolved();
}


void LuaDebugSessionController::ExecuteDebugger(lua_State* l , lua_Debug* ar) {

	if (!_isActive) {
		return;
	}

	std::optional<Dap::StoppedEventBody> stoppedEvent = CheckBreakpoints(l, ar);

	if (!stoppedEvent && _debugStepPredicate) {
		stoppedEvent = _debugStepPredicate->GetStopped(l, ar);
		if (stoppedEvent) {
			_debugStepPredicate.reset();
		}
	}

	if (stoppedEvent) {
		auto session = _sessionRef.acquire();
		Assert(session);

		auto stackTraceProvider = Com::createInstance<LuaStackTraceProvider>(l, ar);
		const ContinueExecutionMode continueMode = session->StopExecution(std::move(*stoppedEvent), stackTraceProvider);

		if (continueMode == ContinueExecutionMode::Step) {
			_debugStepPredicate = std::make_unique<StepPredicate>(l, ar);
		}
		else if (continueMode == ContinueExecutionMode::StepIn) {
			_debugStepPredicate = std::make_unique<StepPredicate>(l, ar, true);
		}
		else if (continueMode == ContinueExecutionMode::StepOut) {
			if (const auto [line, stackDepth] = GetCurrentLineAndStackDepth(l, ar); stackDepth > 0) {
				_debugStepPredicate = std::make_unique<StepOutPredicate>(stackDepth);
			}
			
		}
		else if (continueMode == ContinueExecutionMode::Stopped) {
			//this->DisableDebug();
		}
	}
}


std::optional<Dap::StoppedEventBody> LuaDebugSessionController::CheckBreakpoints(lua_State* l, lua_Debug* ar) {

	lock_(_mutex);

	if (ar->event == LUA_HOOKLINE && ar->source && ar->currentline > 0) {

		lua_getinfo(l, "nSl", ar);

		auto bp = std::find_if(_sourceBreakpoints.begin(), _sourceBreakpoints.end(), [this, ar](const SourceBp& bp) {
			return bp.GetSource(*this) ==  ar->source && bp.Bp().line == ar->currentline;
		});

		if (bp == _sourceBreakpoints.end()) {
			return std::nullopt;
		}

		Dap::StoppedEventBody ev("breakpoint", "Paused on breakpoint");
		ev.hitBreakpointIds.emplace().push_back(bp->Id());
		ev.threadId = 1;
		ev.allThreadsStopped = true;

		return ev;
	}
	
	if (ar->event == LUA_HOOKCALL) {
		lua_getinfo(l, "nS", ar);
		if (!ar->name || !ar->what || strcmp(ar->what, "Lua") != 0) {
			return std::nullopt;
		}

		auto bp = std::find_if(_functionBreakpoints.begin(), _functionBreakpoints.end(), [this, ar](const FunctionBp& bp) {
			return bp.Bp().name == ar->name;
		});

		if (bp == _functionBreakpoints.end()) {
			return std::nullopt;
		}

		Dap::StoppedEventBody ev("function breakpoint", Core::Format::format("Paused on ({})", bp->Bp().name));
		ev.hitBreakpointIds.emplace().push_back(bp->Id());
		ev.threadId = 1;
		ev.allThreadsStopped = true;

		return ev;
	}

	return std::nullopt;
}

/* -------------------------------------------------------------------------- */
LuaDebugSessionController::SourceBp::SourceBp(unsigned bpId, unsigned sourceId, Runtime::Dap::SourceBreakpoint bp) noexcept
	: _id(bpId)
	, _sourceId(sourceId)
	, _bp(bp)
{}

unsigned LuaDebugSessionController::SourceBp::Id() const {
	return _id;
}

unsigned LuaDebugSessionController::SourceBp::SourceId() const {
	return _sourceId;
}

const Dap::SourceBreakpoint& LuaDebugSessionController::SourceBp::Bp() const {
	return _bp;
}

const Dap::Source& LuaDebugSessionController::SourceBp::GetSource(const LuaDebugSessionController& controller) const {
	return controller.GetSource(_sourceId);
}

/* -------------------------------------------------------------------------- */
LuaDebugSessionController::FunctionBp::FunctionBp(unsigned bpId, Dap::FunctionBreakpoint bp) noexcept: _id(bpId), _bp(bp)
{}

unsigned LuaDebugSessionController::FunctionBp::Id() const {
	return _id;
}
const Dap::FunctionBreakpoint& LuaDebugSessionController::FunctionBp::Bp() const {
	return _bp;
}

/* -------------------------------------------------------------------------- */
LuaDebugSessionController::SourceEntry::SourceEntry(unsigned id, Dap::Source&& source) noexcept : _id(id), _sourceInfo(std::move(source))
{}

unsigned LuaDebugSessionController::SourceEntry::Id() const {
	return _id;
}

const Dap::Source& LuaDebugSessionController::SourceEntry::GetSource() const {
	return _sourceInfo;
}


} // namespace Lua::Debug
