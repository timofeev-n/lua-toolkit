//◦ Playrix ◦
#include <lua-toolkit/debug/stacktraceprovider.h>
#include <runtime/com/comclass.h>

extern "C" {
#include <lua.h>
}

namespace Lua::Debug {


class LuaStackTraceProvider final : public  Runtime::Debug::StackTraceProvider
{
	COMCLASS_(Runtime::Debug::StackTraceProvider)

public:

	LuaStackTraceProvider(lua_State*, lua_Debug*);

	Runtime::Dap::StackTraceResponseBody GetStackTrace(Runtime::Dap::StackTraceArguments) override;

	std::vector<Runtime::Dap::Scope> GetScopes(unsigned stackFrameId) override;

	std::vector<Runtime::Dap::Variable> GetVariables(Runtime::Dap::VariablesArguments) override;


private:

	class StackFrameEntry
	{
	public:
		StackFrameEntry(LuaStackTraceProvider& provider, unsigned frameId, int level);

		unsigned Id() const;

		int Level() const;

		const Runtime::Dap::StackFrame& GetFrameInfo() const;

		std::vector<Runtime::Dap::Scope> GetScopes(LuaStackTraceProvider& provider);

	private:
		const unsigned _id;
		const int _level;
		Runtime::Dap::StackFrame _frameInfo;
		std::vector<Runtime::Dap::Scope> _scopes;
	};


	class VariableEntry
	{
	public:

		VariableEntry(unsigned referenceId, StackFrameEntry& parentFrame);

		VariableEntry(unsigned referenceId, VariableEntry& parentVariable, std::string_view name, std::optional<int> indexOnFrame = std::nullopt);

		VariableEntry(unsigned referenceId, VariableEntry& parentVariable, int index);

		unsigned Id() const;

		const std::string& GetName() const;

		bool IsIndexed() const;

		int GetIndex() const;

		const Runtime::Dap::Variable& GetVariable(LuaStackTraceProvider&);

		void AddChild(LuaStackTraceProvider& provider, std::string_view name, std::optional<int> indexOnFrame = std::nullopt);

		void AddChild(LuaStackTraceProvider& provider, int);

		std::vector<Runtime::Dap::Variable> GetChildren(LuaStackTraceProvider& provider);


	private:

		bool PushChildValue(LuaStackTraceProvider& provider, const VariableEntry& child) const;

		const unsigned _referenceId;
		std::string _name;
		std::optional<int> _indexedKey;

		std::optional<unsigned> _parentFrame;
		std::optional<unsigned> _parentVariableId;
		std::optional<int> _indexOnFrame;


		std::optional<Runtime::Dap::Variable> _variableInfo;
		std::vector<unsigned> _children;
	};


	StackFrameEntry& GetStackFrameEntry(unsigned frameId);

	unsigned NewVariable(StackFrameEntry& parentStack);

	unsigned NewVariable(VariableEntry& parentVariable, std::string_view name, std::optional<int> indexOnFrame);

	unsigned NewVariable(VariableEntry& parentVariable, int index);

	VariableEntry& GetVariableEntry(unsigned refId);


	lua_State* const _lua;
	lua_Debug* _ar;
	std::vector<StackFrameEntry> _stackFrames;
	std::list<VariableEntry> _variables;
	unsigned _variableRefId = 0;

};


} // namespace Lua::Debug
