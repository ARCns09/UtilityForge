#include "core/errors/ErrorFormatter.h"

namespace utilityforge::core {

QString ErrorFormatter::userSummary(const AppError &error)
{
    if (!error.isValid()) {
        return QStringLiteral("An unknown error occurred.");
    }

    return error.userMessage;
}

QString ErrorFormatter::technicalSummary(const AppError &error)
{
    if (!error.isValid()) {
        return QStringLiteral("internal.invalid_error");
    }

    QString summary = error.code;
    if (!error.technicalDetails.isEmpty()) {
        summary += QStringLiteral(": ");
        summary += error.technicalDetails;
    }
    return summary;
}

AppError ErrorFormatter::processStartError(const QString &details)
{
    return {
        .category = ErrorCategory::ProcessStart,
        .severity = ErrorSeverity::Error,
        .code = QStringLiteral("process.start_failed"),
        .userMessage = QStringLiteral("The external tool could not be started."),
        .technicalDetails = details,
        .taskId = {},
        .recoveryActions = {QStringLiteral("Check the tool path and permissions.")},
    };
}

AppError ErrorFormatter::processFailure(int exitCode, const QString &details)
{
    return {
        .category = ErrorCategory::ProcessFailure,
        .severity = ErrorSeverity::Error,
        .code = QStringLiteral("process.nonzero_exit"),
        .userMessage = QStringLiteral("The external tool reported a failure."),
        .technicalDetails =
            QStringLiteral("Exit code %1. %2").arg(exitCode).arg(details),
        .taskId = {},
        .recoveryActions = {QStringLiteral("Review the task details and try again.")},
    };
}

} // namespace utilityforge::core
