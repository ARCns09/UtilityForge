#include "core/errors/AppError.h"

namespace utilityforge::core {

bool AppError::isValid() const
{
    return category != ErrorCategory::None && !code.isEmpty() && !userMessage.isEmpty();
}

} // namespace utilityforge::core
