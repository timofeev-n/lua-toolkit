//◦ Playrix ◦
#include "lua-toolkit/debug/adapterprotocol.h"

namespace Runtime::Dap {

/* -------------------------------------------------------------------------- */

ProtocolMessage::ProtocolMessage(unsigned messageSeq, std::string messageType): seq(messageSeq), type(std::move(messageType))
{}



/* -------------------------------------------------------------------------- */
EventMessage::EventMessage (unsigned messageSeq, std::string_view eventType_)
	: ProtocolMessage(messageSeq, std::string{ProtocolMessage::MessageEvent})
	, eventType(eventType_)
{}


/* -------------------------------------------------------------------------- */

ResponseMessage::ResponseMessage(unsigned messageSeq, const RequestMessage& request)
	: ProtocolMessage(messageSeq, std::string{ProtocolMessage::MessageResponse})
	, request_seq(request.seq)
	, command(request.command)
{}

ResponseMessage& ResponseMessage::SetError(std::string_view errorMessage) {
	success = false;
	if (!errorMessage.empty()) {
		message.emplace(errorMessage.data(), errorMessage.size());
	}

	return *this;
}

/* -------------------------------------------------------------------------- */
Source::Source(std::string_view sourcePath): path(sourcePath)
{}


/* -------------------------------------------------------------------------- */
Thread::Thread(unsigned threadId, std::string_view threadName): id(threadId), name(threadName)
{}


/* -------------------------------------------------------------------------- */
StackFrame::StackFrame(unsigned frameId, std::string_view frameName): id(frameId), name(frameName)
{}


/* -------------------------------------------------------------------------- */

Variable::Variable(unsigned refId, std::string_view variableName, std::string_view variableValue)
	: variablesReference(refId)
	, name(variableName)
	, value(variableValue)
{}

/* -------------------------------------------------------------------------- */
Scope::Scope(std::string_view scopeName, unsigned varRef): name(scopeName), variablesReference(varRef)
{}

} // namespace Runtime::Dap
