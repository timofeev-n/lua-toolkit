//◦ Playrix ◦
#pragma once
#include <lua-toolkit/debug/adapterprotocol.h>
#include <runtime/com/comptr.h>


namespace Runtime::Debug {

struct StackTraceProvider : IRefCounted
{
	using Ptr = ComPtr<StackTraceProvider>;

	virtual Dap::StackTraceResponseBody GetStackTrace(Dap::StackTraceArguments) = 0;

	virtual std::vector<Dap::Scope> GetScopes(unsigned stackFrameId) = 0;

	virtual std::vector<Dap::Variable> GetVariables(Dap::VariablesArguments) = 0;
};


} // namespace Runtime::Debug
