//◦ Playrix ◦
#include "luastacktraceprovider.h"


namespace Lua::Debug {

using namespace Runtime;
using namespace Runtime::Debug;
using namespace Core;

LuaStackTraceProvider::StackFrameEntry::StackFrameEntry(LuaStackTraceProvider& provider, unsigned frameId, int level)
	: _id(frameId)
	, _level(level)
	, _frameInfo(_id, {})
{
	lua_Debug* const ar = provider._ar;

	lua_getinfo(provider._lua, "nl", ar);

	if (strcmp(ar->what, "main") == 0) {
		_frameInfo.name = "Global";
	}
	else if (strcmp(ar->what, "Lua") == 0) {
		_frameInfo.name = ar->name ? ar->name : "GLOBAL_SCOPE";
	}
	else if (strcmp(ar->what, "C") == 0) {
		_frameInfo.name = "CFunc";
	}

	if (ar->source) {
		Dap::Source& source = _frameInfo.source.emplace(ar->source);
		_frameInfo.line = ar->currentline;
	}
}


unsigned LuaStackTraceProvider::StackFrameEntry::Id() const {
	return _id;
}


int LuaStackTraceProvider::StackFrameEntry::Level() const {
	return _level;
}


const Dap::StackFrame& LuaStackTraceProvider::StackFrameEntry::GetFrameInfo() const {
	return _frameInfo;
}


std::vector<Dap::Scope> LuaStackTraceProvider::StackFrameEntry::GetScopes(LuaStackTraceProvider& provider) {

	if (_scopes.empty()) {

		const auto GetScope = [&](std::string_view name) -> Dap::Scope& {

			auto scp = std::find_if(_scopes.begin(), _scopes.end(), [name](const Dap::Scope& s) { return s.name == name; });

			if (scp != _scopes.end()) {
				return *scp;
			}

			const unsigned varRef = provider.NewVariable(*this);

			return _scopes.emplace_back(name, varRef);
		};


		lua_Debug* const ar = provider._ar;

		if (lua_getstack(provider._lua, _level, ar) == 0) {
			LOG_WARN("Fail to activate stack frame");
			return {};
		}

		Dap::Scope& locals = GetScope("Locals");
		locals.presentationHint.emplace("locals");

		auto& localsVar = provider.GetVariableEntry(locals.variablesReference);

		int n = 0;
		do
		{
			const char* const name = lua_getlocal(provider._lua, ar, ++n);
			if (name == nullptr) {
				break;
			}

			localsVar.AddChild(provider, name, n);

			lua_pop(provider._lua, 1);
		}
		while (true);

	}


	return _scopes;
}

/* -------------------------------------------------------------------------- */
LuaStackTraceProvider::VariableEntry::VariableEntry(unsigned referenceId, VariableEntry& parentVariable, std::string_view name, std::optional<int> indexOnFrame)
	: _referenceId(referenceId)
	, _name(name)
	, _parentVariableId(parentVariable.Id())
	, _indexOnFrame(indexOnFrame)
{
}


LuaStackTraceProvider::VariableEntry::VariableEntry(unsigned referenceId, VariableEntry& parentVariable, int index)
	: _referenceId(referenceId)
	, _name(std::to_string(index))
	, _indexedKey(index)
	, _parentVariableId(parentVariable.Id())
{}


LuaStackTraceProvider::VariableEntry::VariableEntry(unsigned referenceId, StackFrameEntry& parentFrame)
	: _referenceId(referenceId)
	, _parentFrame(parentFrame.Id())
{}


unsigned LuaStackTraceProvider::VariableEntry::Id() const {
	return _referenceId;
}


const std::string& LuaStackTraceProvider::VariableEntry::GetName() const {
	return _variableInfo ? _variableInfo->name : this->_name;
}


bool LuaStackTraceProvider::VariableEntry::IsIndexed() const {
	return static_cast<bool>(_indexedKey);
}


int LuaStackTraceProvider::VariableEntry::GetIndex() const {
	Assert(IsIndexed());
	return *_indexedKey;
}


const Dap::Variable& LuaStackTraceProvider::VariableEntry::GetVariable(LuaStackTraceProvider& provider) {

	lua_State* l = provider._lua;

	const auto top = lua_gettop(l);

	SCOPE_Leave {
		lua_settop(l, top);
	};

	if (!_variableInfo) {
		Dap::Variable& variable = _variableInfo.emplace(_referenceId, std::move(_name));
		variable.presentationHint.emplace().kind = "property";

		if (!_parentFrame) {
		
			Assert(_parentVariableId);
		
			auto& parent = provider.GetVariableEntry(*_parentVariableId);
			if (parent.PushChildValue(provider, *this)) {

				const int valueType = lua_type(l, -1);

				if (valueType == LUA_TNIL) {
					variable.value = "nil";
					variable.type = "nil";
				}
				else if (valueType == LUA_TBOOLEAN) {
					const auto value = lua_toboolean(l, -1);
					variable.value = value ? "True" : "False";
					variable.type = "boolean";
				}
				else if (valueType == LUA_TLIGHTUSERDATA) {

				}
				else if (valueType == LUA_TNUMBER) {
					const auto value = lua_tonumber(l, -1);
					variable.value = std::to_string(value);
					variable.type = "number";
				}
				else if (valueType == LUA_TSTRING) {
					size_t len;
					const char* const value = lua_tolstring(l, -1, &len);
					variable.value.assign(value, len);
					variable.type = "string";
				}
				else if (valueType == LUA_TTABLE) {

					bool isArray = true;

					lua_pushnil(l);
					while (lua_next(l, -2) != 0) {

						constexpr int KeyIndex = -2;

						const int keyType = lua_type(l, KeyIndex);

						if (keyType == LUA_TNUMBER) {
							const auto index = lua_tonumber(l, KeyIndex);
							this->AddChild(provider, index);
						}
						else if (keyType == LUA_TSTRING) {
							isArray = false;
							size_t len;
							const char* const value = lua_tolstring(l, KeyIndex, &len);
							this->AddChild(provider, std::string_view{value, len});
						}
						else {
							// Unsupported index type
						}
						
						lua_pop(l, 1);
					}

					if (isArray) {
						variable.type = "array";
						variable.indexedVariables = static_cast<unsigned>(_children.size());
					}
					else {
						variable.type = "object";
						variable.namedVariables = static_cast<unsigned>(_children.size());
					}
				}
				else if (valueType == LUA_TFUNCTION) {

				}
				else if (valueType == LUA_TUSERDATA) {

				}
				else if (valueType == LUA_TTHREAD) {

				}
			}
			else {
				variable.value = "[unevaluated]";
			}
		}


		if (!_children.empty()) {
			variable.namedVariables = static_cast<unsigned>(_children.size());
		}
		else {
			variable.variablesReference = 0;
		}
	}

	return *_variableInfo;
}


void LuaStackTraceProvider::VariableEntry::AddChild(LuaStackTraceProvider& provider, std::string_view name, std::optional<int> indexOnFrame) {
	const auto childId = provider.NewVariable(*this, name, indexOnFrame);
	_children.push_back(childId);
}

void LuaStackTraceProvider::VariableEntry::AddChild(LuaStackTraceProvider& provider, int index) {
	const auto childId = provider.NewVariable(*this, index);
	_children.push_back(childId);
}


std::vector<Dap::Variable> LuaStackTraceProvider::VariableEntry::GetChildren(LuaStackTraceProvider& provider) {

	std::vector<Dap::Variable> variables;
	variables.reserve(_children.size());

	std::transform(_children.begin(), _children.end(), std::back_inserter(variables), [&provider](unsigned variableId) {
		return provider.GetVariableEntry(variableId).GetVariable(provider);
	});

	return variables;
}


bool LuaStackTraceProvider::VariableEntry::PushChildValue(LuaStackTraceProvider& provider, const VariableEntry& child) const {

	Assert(!child._parentFrame);

	auto l = provider._lua;
	auto ar = provider._ar;
	
	if (_parentFrame) {
		Assert(child._indexOnFrame);

		auto& frame = provider.GetStackFrameEntry(*_parentFrame);
		lua_getstack(l, frame.Level(), ar);
		return lua_getlocal(l, ar, *child._indexOnFrame) != nullptr;
	}
	else {
		Assert(this->_parentVariableId);

		auto& parentVariable = provider.GetVariableEntry(*this->_parentVariableId);
		parentVariable.PushChildValue(provider, *this);

		Assert(lua_type(l, -1) == LUA_TTABLE);

		if (child.IsIndexed()) {
			lua_pushinteger(l, child.GetIndex());
		}
		else {
			lua_pushstring(l, child.GetName().c_str());
		}

		lua_gettable(l, -2);
		lua_remove(l, -2);

		return true;
	}

	return false;
}


/* -------------------------------------------------------------------------- */
LuaStackTraceProvider::LuaStackTraceProvider(lua_State* luaState, lua_Debug* ar): _lua(luaState), _ar(ar) {
}


Dap::StackTraceResponseBody LuaStackTraceProvider::GetStackTrace(Dap::StackTraceArguments args) {

	if (_stackFrames.empty()) {
		unsigned frameId = 0;
		
		for (int level = 0; lua_getstack(_lua, level, _ar) != 0; ++level) {
			_stackFrames.emplace_back(*this, ++frameId, level);
		}
	}

	Dap::StackTraceResponseBody response;
	response.stackFrames.reserve(_stackFrames.size());
	std::transform(_stackFrames.begin(), _stackFrames.end(), std::back_inserter(response.stackFrames), [](const StackFrameEntry& f) -> Dap::StackFrame {
		return f.GetFrameInfo();
	});


	return response;
}


std::vector<Dap::Scope> LuaStackTraceProvider::GetScopes(unsigned frameId) {
	auto& frame = GetStackFrameEntry(frameId);
	return frame.GetScopes(*this);
}


std::vector<Dap::Variable> LuaStackTraceProvider::GetVariables(Runtime::Dap::VariablesArguments arg) {
	auto& variableEntry = GetVariableEntry(arg.variablesReference);
	return variableEntry.GetChildren(*this);
}


LuaStackTraceProvider::StackFrameEntry& LuaStackTraceProvider::GetStackFrameEntry(unsigned frameId) {
	auto frame = std::find_if(_stackFrames.begin(), _stackFrames.end(), [frameId](const StackFrameEntry& entry) { return entry.Id() == frameId; });
	Assert2(frame != _stackFrames.end(), "Invalid frame id");
	return *frame;
}


unsigned LuaStackTraceProvider::NewVariable(StackFrameEntry& parentStack) {
	return _variables.emplace_back(++_variableRefId, parentStack).Id();
}


unsigned LuaStackTraceProvider::NewVariable(VariableEntry& parentVariable, std::string_view name, std::optional<int> indexOnFrame) {
	return _variables.emplace_back(++_variableRefId, parentVariable, name, indexOnFrame).Id();
}


unsigned LuaStackTraceProvider::NewVariable(VariableEntry& parentVariable, int index) {
	return _variables.emplace_back(++_variableRefId, parentVariable, index).Id();
}


LuaStackTraceProvider::VariableEntry& LuaStackTraceProvider::GetVariableEntry(unsigned variableId) {

	auto variable = std::find_if(_variables.begin(), _variables.end(), [variableId](const VariableEntry& v) { return v.Id() == variableId;});
	Assert(variable != _variables.end());

	return *variable;

}

} // namespace Lua::Debug
