#include "core/tasks/CancellationToken.h"

#include <utility>

namespace utilityforge::core {

CancellationToken::CancellationToken(std::shared_ptr<CancellationState> state)
    : state_(std::move(state))
{
}

bool CancellationToken::isCancellationRequested() const
{
    return state_ && state_->isRequested.load(std::memory_order_acquire);
}

bool CancellationToken::isValid() const
{
    return static_cast<bool>(state_);
}

CancellationSource::CancellationSource()
    : state_(std::make_shared<CancellationState>())
{
}

CancellationToken CancellationSource::token() const
{
    return CancellationToken(state_);
}

bool CancellationSource::requestCancellation()
{
    bool expected = false;
    return state_->isRequested.compare_exchange_strong(
        expected, true, std::memory_order_acq_rel, std::memory_order_acquire);
}

bool CancellationSource::isCancellationRequested() const
{
    return state_->isRequested.load(std::memory_order_acquire);
}

} // namespace utilityforge::core
