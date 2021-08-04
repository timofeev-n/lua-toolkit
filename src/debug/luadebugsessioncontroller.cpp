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


LuaDebugSessionController::LuaDebugSessionController()
{}

LuaDebugSessionController::~LuaDebugSessionController() {

}


void LuaDebugSessionController::SetSession(ComPtr<DebugSession> session) {
	_sessionRef = std::move(session);
}

Task<> LuaDebugSessionController::ConfigurationDone() {

	Assert(_startMode != StartMode::Unknown);

	/*co_await _luaSchedulerRef.acquire();

	EnableDebug();*/

	this->Start(_startMode).detach();

	return Task<>::makeResolved();
}

Task<> LuaDebugSessionController::Disconnect(){
	co_return;
}

Task<std::vector<Dap::Breakpoint>> LuaDebugSessionController::SetBreakpoints(Dap::SetBreakpointsArguments arg) {

	std::vector<Dap::Breakpoint> breakpoints;

	for (const auto& srcBp : arg.breakpoints) {

		const auto bpId = ++_bpId;

		Dap::Breakpoint& bp = breakpoints.emplace_back();
		bp.id = bpId;
		bp.verified = true;
		// bp.message = "Not implemented yet";
		// bp.source = std::move(bp.source);
		bp.line = bp.line;

		_sourceBreakpoints.emplace_back(bpId, srcBp);
	}

	return Task<std::vector<Dap::Breakpoint>>::makeResolved(std::move(breakpoints));
}

Task<std::vector<Dap::Thread>> LuaDebugSessionController::GetThreads() {

	std::vector<Dap::Thread> threads;

	threads.emplace_back(1, "Default");

	return Task<std::vector<Dap::Thread>>::makeResolved(std::move(threads));
}


void LuaDebugSessionController::EnableDebug() {

	lua_State* const l = GetLua();

	lua_pushlightuserdata(l, this);
	lua_setfield(l, LUA_GLOBALSINDEX, "__lua_DebuggerSession");

	lua_sethook(GetLua(), [](lua_State* l, lua_Debug* ar) noexcept {

		lua_getfield(l, LUA_GLOBALSINDEX, "__lua_DebuggerSession");
		LuaDebugSessionController& self  = *reinterpret_cast<LuaDebugSessionController*>(lua_touserdata (l, -1));
		lua_pop(l, 1);
		
		lua_getinfo(l, "nSl", ar);

		self.ExecuteDebugger(l, ar);

	}
	, LUA_MASKRET | LUA_MASKCALL | LUA_MASKLINE, 0);
}

void LuaDebugSessionController::DisableDebug() {
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

	std::string_view what;
	if (ar->event == LUA_HOOKCALL) {

	}
	else if (ar->event == LUA_HOOKRET) {

	}
	else if (ar->event == LUA_HOOKLINE) {

		auto bp = std::find_if(_sourceBreakpoints.begin(), _sourceBreakpoints.end(), [ar](const SourceBp& bp) {
			return bp.Bp().line == ar->currentline;
		});

		if (bp == _sourceBreakpoints.end()) {
			return;
		}


		auto session = _sessionRef.acquire();
		Assert(session);

		Dap::StoppedEventBody ev;
		ev.reason = "breakpoint";
		ev.hitBreakpointIds.emplace().push_back(bp->Id());
		ev.threadId = 1;
		ev.allThreadsStopped = true;

		auto stackTraceProvider = Com::createInstance<LuaStackTraceProvider>(l, ar);

		session->StopExecution(std::move(ev), stackTraceProvider);


			// std::cout << Core::Format::format("BREAK AT ({})", bp->Bp().line) << std::endl;

	/*	auto task = Async::run([](LuaDebugSessionController& self, unsigned bpId, lua_Debug* ar) -> Task<> {

			auto stream = self._messageStreamRef.acquire();
			Assert(stream);

			auto bp = std::find_if(self._sourceBreakpoints.begin(), self._sourceBreakpoints.end(), [bpId](const SourceBp& bp) {
				return bp.Id() == bpId;
			});

			Assert(bp != self._sourceBreakpoints.end());

		

			auto evValue = runtimeValueCopy(std::move(ev));

			co_await stream->SendDapMessage(evValue);

			while (true) {
				co_await 100ms;
			}

		}, RuntimeCore::instance().poolScheduler(), std::ref(*this), bp->Id(), ar);

		task.detach();*/

		//Async::wait(task);
	}

	// std::cout << Core::Format::format("[{}] on ({}):{}. n:({})\n", what, ar->source, ar->currentline, ar->name ? ar->name : "-no-name-");
}

/* -------------------------------------------------------------------------- */

LuaDebugSessionController::SourceBp::SourceBp(unsigned id, Runtime::Dap::SourceBreakpoint bp): _id(id), _bp(bp)
{}

unsigned LuaDebugSessionController::SourceBp::Id() const {
	return _id;
}

const Dap::SourceBreakpoint& LuaDebugSessionController::SourceBp::Bp() const {
	return (_bp);
}


} // namespace Lua::Debug
