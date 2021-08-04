//◦ Playrix ◦
#pragma once
#include <runtime/async/task.h>
#include <runtime/io/asyncreader.h>
#include <runtime/serialization/runtimevalue.h>

namespace Runtime::Debug {

/**
* 
*/
struct ABSTRACT_TYPE DapMessageStream: virtual IRefCounted
{
	using Ptr = ComPtr<DapMessageStream>;

	static DapMessageStream::Ptr Create(ComPtr<Io::AsyncReader> stream);


	virtual Async::Task<RuntimeReadonlyDictionary::Ptr> GetDapMessage() = 0;

	virtual Async::Task<> SendDapMessage(RuntimeReadonlyDictionary::Ptr command) = 0;
};

} // namespace Runtime::Debug
