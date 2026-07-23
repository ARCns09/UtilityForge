#pragma once

#include <QString>
#include <QStringList>

namespace utilityforge::core {

enum class ErrorCategory {
    None,
    Validation,
    UnsupportedInput,
    MissingCapability,
    Filesystem,
    ProcessStart,
    ProcessFailure,
    CancellationCleanup,
    Internal
};

enum class ErrorSeverity {
    Information,
    Warning,
    Error,
    Critical
};

struct AppError {
    ErrorCategory category{ErrorCategory::None};
    ErrorSeverity severity{ErrorSeverity::Error};
    QString code;
    QString userMessage;
    QString technicalDetails;
    QString taskId;
    QStringList recoveryActions;

    [[nodiscard]] bool isValid() const;
};

} // namespace utilityforge::core
