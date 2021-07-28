//◦ Playrix ◦
#include "lua-toolkit/debug/adapterprotocol.h"

namespace Runtime::Dap {

/* -------------------------------------------------------------------------- */

ProtocolMessage::ProtocolMessage(unsigned messageSeq, std::string messageType): seq(messageSeq), type(std::move(messageType))
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

} // namespace Runtime::Dap
