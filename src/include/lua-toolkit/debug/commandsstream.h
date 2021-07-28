//◦ Playrix ◦
#pragma once
#include <runtime/async/task.h>
#include <runtime/io/asyncreader.h>
#include <runtime/serialization/runtimevalue.h>

namespace Lua::Debug {

/**
* 
*/
struct ABSTRACT_TYPE CommandsStream: virtual Runtime::IRefCounted
{
	using Ptr = Runtime::ComPtr<CommandsStream>;

	static CommandsStream::Ptr Create(Runtime::ComPtr<Runtime::Io::AsyncReader> stream);


	virtual Runtime::Async::Task<Runtime::RuntimeReadonlyDictionary::Ptr> GetFrontendCommand() = 0;

	virtual Runtime::Async::Task<> SendFrontendCommand(Runtime::RuntimeReadonlyDictionary::Ptr command) = 0;
};

} // namespace Lua::Debug
