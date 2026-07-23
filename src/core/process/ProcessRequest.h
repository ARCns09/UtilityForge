#pragma once

#include "core/errors/AppError.h"

#include <QByteArray>
#include <QMetaType>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QUuid>

#include <optional>

namespace utilityforge::core {

using ProcessId = QUuid;

enum class ExpectedOutputKind {
    None,
    RegularFile,
    Directory
};

struct ProcessRequest {
    QString programPath;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
    QString expectedOutputPath;
    ExpectedOutputKind expectedOutputKind{ExpectedOutputKind::None};
    qsizetype maximumOutputBytes{64 * 1024};
    int startTimeoutMs{5'000};
    int executionTimeoutMs{0};
    int terminationGraceMs{1'500};
};

struct ProcessResult {
    int exitCode{-1};
    QProcess::ExitStatus exitStatus{QProcess::NormalExit};
    QByteArray standardOutput;
    QByteArray standardError;
    bool standardOutputTruncated{false};
    bool standardErrorTruncated{false};
    bool wasCancelled{false};
    std::optional<AppError> error;

    [[nodiscard]] bool succeeded() const
    {
        return !wasCancelled && !error.has_value() && exitStatus == QProcess::NormalExit
            && exitCode == 0;
    }
};

} // namespace utilityforge::core

Q_DECLARE_METATYPE(utilityforge::core::ProcessResult)
