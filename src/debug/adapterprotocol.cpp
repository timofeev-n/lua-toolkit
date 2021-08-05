//◦ Playrix ◦
#include "lua-toolkit/debug/adapterprotocol.h"
#include <runtime/utils/strings.h>

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
StoppedEventBody::StoppedEventBody(std::string_view eventReason, std::optional<unsigned> eventThreadId): reason(eventReason), threadId(eventThreadId)
{}

StoppedEventBody::StoppedEventBody(std::string_view eventReason, std::string_view eventDescription, std::optional<unsigned> eventThreadId)
	: reason(eventReason)
	, description(eventDescription)
	, threadId(eventThreadId)
{}

/* -------------------------------------------------------------------------- */
Source::Source(std::string_view sourcePath): path(sourcePath)
{}

bool operator == (const Source& left, const Source& right) {
	return left == right.path;
}

bool operator == (const Source& left, std::string_view path) {

	if (left.path.empty() && path.empty()) {
		return true;
	}

	if (left.path.empty() || path.empty()) {
		return false;
	}

#if ENGINE_OS_WINDOWS
	return Strings::icaseEqual(left.path, path);
#else
	return left.path == path;
#endif
}

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
