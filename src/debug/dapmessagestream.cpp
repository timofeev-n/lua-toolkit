//◦ Playrix ◦
#include "lua-toolkit/debug/dapmessagestream.h"
#include "remoting/httpstream.h"

#include <runtime/com/comclass.h>
#include <runtime/serialization/json.h>

namespace Runtime::Debug {

using namespace Runtime::Async;

/**
* 
*/
class CommandsStreamImpl final : public DapMessageStream
{
	COMCLASS_(DapMessageStream)

public:
	CommandsStreamImpl(ComPtr<Io::AsyncReader> stream): _bytesStream(std::move(stream))
	{
		Assert(_bytesStream);
	}

private:

	Task<RuntimeReadonlyDictionary::Ptr> GetDapMessage() override {

		HttpStream::Packet packet = co_await HttpStream::ReadHttpPacket(_httpStream, *_bytesStream);

		if (!packet) {
			co_return nothing;
		}

		if (IsDebuggerPresent()) {
			OutputDebugStringA("\nREQUEST:\n");
			OutputDebugStringA(std::string(packet.body).c_str());
			OutputDebugStringA("\n");
		}

		co_return *Serialization::JsonParseString(packet.body);
	}

	Task<> SendDapMessage(Runtime::RuntimeReadonlyDictionary::Ptr command) override {
		Assert(_bytesStream);

		if (Io::AsyncWriter* const asyncWriter = _bytesStream->as<Io::AsyncWriter*>()) {
			co_await HttpStream::SendHttpJsonPacket(std::move(command), "/dap", *asyncWriter);
		}
		else {
			Halt("Currently can send commands only throug Io::AsyncWriter API");
		}
	}

	ComPtr<Io::AsyncReader> _bytesStream;
	HttpStream _httpStream;
};


/* -------------------------------------------------------------------------- */
DapMessageStream::Ptr DapMessageStream::Create(ComPtr<Io::AsyncReader> stream) {
	return Com::createInstance<CommandsStreamImpl, DapMessageStream>(std::move(stream));
}

} // namespace Runtime::Debug
