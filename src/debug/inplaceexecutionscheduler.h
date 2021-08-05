//◦ Playrix ◦//◦ Playrix ◦
#include <runtime/async/scheduler.h>
#include <runtime/threading/event.h>
#include <runtime/utils/disposable.h>
#include <runtime/com/comclass.h>


namespace Runtime::Async {

class InplaceExecutionScheduler final : public Async::Scheduler, public Disposable
{
	COMCLASS_(Async::Scheduler, Disposable)

public:

	InplaceExecutionScheduler();

	void Execute();

private:

	void scheduleInvocation(Invocation invocation) noexcept override;

	void waitAnyActivity() noexcept override;

	void dispose() override;


	std::vector<Invocation> _invocations;
	std::mutex _mutex;
	Threading::Event _signal{Threading::Event::ResetMode::Auto};
	bool _isClosed = false;
};


}


