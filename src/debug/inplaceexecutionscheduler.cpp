//◦ Playrix ◦
#include "inplaceexecutionscheduler.h"
#include <runtime/threading/lock.h>


namespace Runtime::Async {


InplaceExecutionScheduler::InplaceExecutionScheduler()
{}


void InplaceExecutionScheduler::scheduleInvocation(Invocation invocation) noexcept {
	lock_(_mutex);
	_invocations.emplace_back(std::move(invocation));
	_signal.set();
}


void InplaceExecutionScheduler::waitAnyActivity() noexcept {
}


void InplaceExecutionScheduler::dispose() {
	lock_(_mutex);
	_isClosed = true;
	_signal.set();
}


void InplaceExecutionScheduler::Execute() {

	const auto getExecutionState = [this]() -> std::tuple<bool, decltype(this->_invocations) > {
		lock_(_mutex);
		return {!_isClosed, std::move(_invocations) };
	};

	while (true) {

		auto [continueExecution, invocations] = getExecutionState();

		if (!invocations.empty()) {

			const InvocationGuard guard{*this};

			for (auto& invocation : invocations) {
				Scheduler::invoke(*this, std::move(invocation));
			}
		}
		else if (!continueExecution) {
			break;
		}

		_signal.wait();
	}
}

} // namespace Runtime::Asyncs
