//◦ Playrix ◦
#pragma once
#include <runtime/io/asyncwriter.h>
#include <runtime/io/asyncreader.h>
#include <runtime/memory/bytesbuffer.h>
#include <runtime/remoting/httpparser.h>
#include <runtime/serialization/runtimevalue.h>

namespace Runtime {

class HttpStream
{
public:

	struct Packet
	{
		HttpParser headers;
		std::string_view body;

		Packet();

		Packet(HttpParser, std::string_view);

		explicit operator bool() const;
	};

	void AppendBytes(ReadOnlyBuffer bytes);

	Packet GetNextPacket();

	static Async::Task<Packet> ReadHttpPacket(HttpStream& httpStream, Io::AsyncReader& bytesStream);

	static Async::Task<> SendHttpJsonPacket(RuntimeValue::Ptr, std::string_view path, Io::AsyncWriter& stream);

private:

	void PopLastPacketBytes();

	BytesBuffer _inboundBuffer;
	size_t _lastPacketSize = 0;
};


}
