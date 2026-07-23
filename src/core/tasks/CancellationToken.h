#pragma once

#include <atomic>
#include <memory>

namespace utilityforge::core {

struct CancellationState final {
    std::atomic_bool isRequested{false};
};

class CancellationToken final
{
public:
    CancellationToken() = default;

    [[nodiscard]] bool isCancellationRequested() const;
    [[nodiscard]] bool isValid() const;

private:
    explicit CancellationToken(std::shared_ptr<CancellationState> state);

    std::shared_ptr<CancellationState> state_;

    friend class CancellationSource;
};

class CancellationSource final
{
public:
    CancellationSource();

    [[nodiscard]] CancellationToken token() const;
    [[nodiscard]] bool requestCancellation();
    [[nodiscard]] bool isCancellationRequested() const;

private:
    std::shared_ptr<CancellationState> state_;
};

} // namespace utilityforge::core
