//◦ Playrix ◦
#include "lua-toolkit/debug/commandsstream.h"
#include "remoting/httpstream.h"

#include <runtime/com/comclass.h>
#include <runtime/serialization/json.h>

namespace Lua::Debug {

using namespace Runtime;
using namespace Runtime::Async;

/**
* 
*/
class CommandsStreamImpl final : public CommandsStream
{
	COMCLASS_(CommandsStream)

public:
	CommandsStreamImpl(ComPtr<Io::AsyncReader> stream): _bytesStream(std::move(stream))
	{
		Assert(_bytesStream);
	}

private:

	Task<RuntimeReadonlyDictionary::Ptr> GetFrontendCommand() override {

		HttpStream::Packet packet;

		while (!packet)
		{
			packet = _httpStream.GetNextPacket();
			if (!packet) {
				Assert(_bytesStream);
				auto bytes = co_await _bytesStream->read();
				if (!bytes) {
					co_return nothing;
				}

				_httpStream.AppendBytes(bytes.toReadOnly());
			}
		}

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

	Task<> SendFrontendCommand(Runtime::RuntimeReadonlyDictionary::Ptr command) override {
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
CommandsStream::Ptr CommandsStream::Create(Runtime::ComPtr<Runtime::Io::AsyncReader> stream) {
	return Com::createInstance<CommandsStreamImpl, CommandsStream>(std::move(stream));
}

} // namespace Lua::Debug
