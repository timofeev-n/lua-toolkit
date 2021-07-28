//◦ Playrix ◦
#pragma once
#include <lua-toolkit/debug/commandsstream.h>
#include <lua-toolkit/debug/debugsessioncontroller.h>

namespace Lua::Debug {

struct ABSTRACT_TYPE DebugSession : Runtime::IRefCounted
{
	using Ptr = Runtime::ComPtr<DebugSession>;

	static DebugSession::Ptr Create(CommandsStream::Ptr, DebugSessionController::Ptr);

	virtual Runtime::Async::Task<> Run();

	virtual CommandsStream& GetCommandsStream() const = 0;

};

}
