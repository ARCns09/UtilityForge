#pragma once

#include "core/errors/AppError.h"

namespace utilityforge::core {

class ErrorFormatter final
{
public:
    [[nodiscard]] static QString userSummary(const AppError &error);
    [[nodiscard]] static QString technicalSummary(const AppError &error);
    [[nodiscard]] static AppError processStartError(const QString &details);
    [[nodiscard]] static AppError processFailure(int exitCode, const QString &details);
};

} // namespace utilityforge::core
