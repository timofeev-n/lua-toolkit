//◦ Playrix ◦
#include "httpstream.h"
#include <runtime/io/readerwriter.h>
#include <runtime/serialization/json.h>


namespace Runtime {

/* -------------------------------------------------------------------------- */
void HttpStream::AppendBytes(ReadOnlyBuffer bytes) {
	PopLastPacketBytes();
	memcpy(_inboundBuffer.append(bytes.size()), bytes.data(), bytes.size());
}

HttpStream::Packet HttpStream::GetNextPacket() {

	PopLastPacketBytes();

	const HttpParser headers(asStringView(_inboundBuffer));
	if (!headers) {
		return {};
	}

	if (headers["Content-Length"].empty()) {

	}

	const auto contentType = headers["Content-Type"];
	const auto contentLen = headers.contentLength();
	const size_t packetSize = headers.headersLength() + contentLen;
	if (_inboundBuffer.size() < packetSize) {
		return {};
	}

	_lastPacketSize = packetSize;

	std::string_view body (reinterpret_cast<const char*>(_inboundBuffer.data()) + headers.headersLength(), contentLen);

	return {headers, body};
}

void HttpStream::PopLastPacketBytes() {
	if (_lastPacketSize == 0) {
		return;
	}
	Assert(_lastPacketSize <= _inboundBuffer.size());

	const size_t newSize = _inboundBuffer.size() - _lastPacketSize;
	if (newSize > 0) {
		memcpy(_inboundBuffer.data(), _inboundBuffer.data() + _lastPacketSize, newSize);
	}
	_inboundBuffer.resize(newSize);
	_lastPacketSize = 0;
}

Async::Task<> HttpStream::SendHttpJsonPacket(RuntimeValue::Ptr bodyValue, std::string_view path, Io::AsyncWriter& stream) {

	try {
		BytesBuffer bodyBytes;
		{
			Io::BufferWriter writer{bodyBytes};
			Serialization::JsonWrite(writer, bodyValue).rethrowIfException();
		}


		if (IsDebuggerPresent() == TRUE) {
			auto bodyView = asStringView(bodyBytes);
			OutputDebugStringA("\nRESPONSE:\n");
			OutputDebugStringA(std::string(bodyView).c_str());
			OutputDebugStringA("\n");
		}


		std::string headers = Core::Format::format("POST {} HTTP/1.1\r\n", path);
		headers = headers + 
			"Content-Type: application/json\r\n"
			"Content-Length: " + std::to_string(bodyBytes.size()) + 
			"\r\n\r\n";

		BytesBuffer packetBytes(headers.size() + bodyBytes.size());
		memcpy(packetBytes.data(), headers.data(), headers.size());
		memcpy(packetBytes.data() + headers.size(), bodyBytes.data(), bodyBytes.size());

		co_await stream.write(packetBytes.toReadOnly());
	}
	catch (const std::exception& exception) {
		Halt(exception.what());
	}
}

/* -------------------------------------------------------------------------- */
HttpStream::Packet::Packet() = default;

HttpStream::Packet::Packet(HttpParser packetHeaders, std::string_view packetBody): headers(packetHeaders), body(packetBody)
{}

HttpStream::Packet::operator bool() const {
	return static_cast<bool>(this->headers);
}

}
