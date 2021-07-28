//◦ Playrix ◦
#include <runtime/com/comptr.h>
#include <lua-toolkit/debug/adapterprotocol.h>


/**
* 
*/
namespace Lua::Debug {

struct ABSTRACT_TYPE DebugSessionController : Runtime::IRefCounted
{
	using Ptr = Runtime::ComPtr<DebugSessionController>;

	virtual Runtime::Async::Task<> Startup() = 0;

	virtual Runtime::Async::Task<> Disconnect() = 0;

	virtual Runtime::Async::Task<std::vector<Runtime::Dap::Breakpoint>> SetBreakpoints(Runtime::Dap::SetBreakpointsArguments) = 0;
};

} // namespace Lua::Debug
