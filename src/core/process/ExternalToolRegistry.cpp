#include "core/process/ExternalToolRegistry.h"

#include "core/errors/ErrorFormatter.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <utility>

namespace utilityforge::core {
namespace {

QList<ExternalTool> supportedTools()
{
    return {ExternalTool::Ffmpeg, ExternalTool::Ffprobe, ExternalTool::Bsdtar};
}

ToolStatus unknownStatus(ExternalTool tool)
{
    return {
        .tool = tool,
        .availability = ToolAvailability::Unknown,
        .executablePath = {},
        .versionSummary = {},
        .error = std::nullopt,
    };
}

AppError missingToolError(const QString &toolName, const QString &path)
{
    return {
        .category = ErrorCategory::MissingCapability,
        .severity = ErrorSeverity::Warning,
        .code = QStringLiteral("tool.missing"),
        .userMessage = QStringLiteral("%1 is not available.").arg(toolName),
        .technicalDetails = path,
        .taskId = {},
        .recoveryActions = {
            QStringLiteral("Install the tool through your distribution and refresh diagnostics."),
        },
    };
}

} // namespace

QString externalToolDisplayName(ExternalTool tool)
{
    switch (tool) {
    case ExternalTool::Ffmpeg:
        return QStringLiteral("FFmpeg");
    case ExternalTool::Ffprobe:
        return QStringLiteral("FFprobe");
    case ExternalTool::Bsdtar:
        return QStringLiteral("bsdtar");
    }
    return QStringLiteral("Unknown tool");
}

QString toolAvailabilityDisplayName(ToolAvailability availability)
{
    switch (availability) {
    case ToolAvailability::Unknown:
        return QStringLiteral("Not checked");
    case ToolAvailability::Checking:
        return QStringLiteral("Checking");
    case ToolAvailability::Available:
        return QStringLiteral("Available");
    case ToolAvailability::Missing:
        return QStringLiteral("Missing");
    case ToolAvailability::Unsupported:
        return QStringLiteral("Unsupported");
    }
    return QStringLiteral("Unknown");
}

ExternalToolRegistry::ExternalToolRegistry(ProcessRunner *processRunner, QObject *parent)
    : QObject(parent)
    , processRunner_(processRunner)
{
    Q_ASSERT(processRunner_);
    qRegisterMetaType<ToolStatus>();

    for (const ExternalTool tool : supportedTools()) {
        statuses_.insert(tool, unknownStatus(tool));
    }

    connect(processRunner_,
            &ProcessRunner::processFinished,
            this,
            &ExternalToolRegistry::handleProcessFinished);
}

ToolStatus ExternalToolRegistry::status(ExternalTool tool) const
{
    return statuses_.value(tool, unknownStatus(tool));
}

void ExternalToolRegistry::checkTool(ExternalTool tool)
{
    const ToolStatus currentStatus = status(tool);
    if (currentStatus.availability == ToolAvailability::Checking
        || currentStatus.availability == ToolAvailability::Available) {
        return;
    }

    const QString overridePath = executableOverrides_.value(tool);
    const QString resolvedPath = overridePath.isEmpty()
        ? QStandardPaths::findExecutable(executableName(tool))
        : QDir::cleanPath(overridePath);
    const QFileInfo executableInfo(resolvedPath);
    if (resolvedPath.isEmpty() || !QDir::isAbsolutePath(resolvedPath)
        || !executableInfo.isFile() || !executableInfo.isExecutable()) {
        setStatus(ToolStatus{
            .tool = tool,
            .availability = ToolAvailability::Missing,
            .executablePath = resolvedPath,
            .versionSummary = {},
            .error = missingToolError(externalToolDisplayName(tool), resolvedPath),
        });
        return;
    }

    setStatus(ToolStatus{
        .tool = tool,
        .availability = ToolAvailability::Checking,
        .executablePath = executableInfo.absoluteFilePath(),
        .versionSummary = {},
        .error = std::nullopt,
    });

    ProcessRequest request{
        .programPath = executableInfo.absoluteFilePath(),
        .arguments = probeArguments(tool),
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 32 * 1024,
        .startTimeoutMs = 3'000,
        .executionTimeoutMs = 5'000,
        .terminationGraceMs = 500,
    };
    const ProcessId processId = processRunner_->start(request);
    if (processId.isNull()) {
        setStatus(ToolStatus{
            .tool = tool,
            .availability = ToolAvailability::Unsupported,
            .executablePath = executableInfo.absoluteFilePath(),
            .versionSummary = {},
            .error = ErrorFormatter::processStartError(
                QStringLiteral("ProcessRunner rejected the probe thread.")),
        });
        return;
    }
    pendingProbes_.insert(processId, tool);
}

void ExternalToolRegistry::refreshTool(ExternalTool tool)
{
    if (status(tool).availability == ToolAvailability::Checking) {
        return;
    }
    setStatus(unknownStatus(tool));
    checkTool(tool);
}

void ExternalToolRegistry::refreshAll()
{
    for (const ExternalTool tool : supportedTools()) {
        refreshTool(tool);
    }
}

bool ExternalToolRegistry::setExecutableOverride(ExternalTool tool,
                                                 const QString &absolutePath)
{
    if (!absolutePath.isEmpty() && !QDir::isAbsolutePath(absolutePath)) {
        return false;
    }

    if (absolutePath.isEmpty()) {
        executableOverrides_.remove(tool);
    } else {
        executableOverrides_.insert(tool, QDir::cleanPath(absolutePath));
    }
    setStatus(unknownStatus(tool));
    return true;
}

QString ExternalToolRegistry::executableName(ExternalTool tool) const
{
    switch (tool) {
    case ExternalTool::Ffmpeg:
        return QStringLiteral("ffmpeg");
    case ExternalTool::Ffprobe:
        return QStringLiteral("ffprobe");
    case ExternalTool::Bsdtar:
        return QStringLiteral("bsdtar");
    }
    return {};
}

QStringList ExternalToolRegistry::probeArguments(ExternalTool tool) const
{
    if (tool == ExternalTool::Bsdtar) {
        return {QStringLiteral("--version")};
    }
    return {QStringLiteral("-version")};
}

bool ExternalToolRegistry::probeIdentityMatches(ExternalTool tool,
                                                const QByteArray &output) const
{
    const QByteArray lowerOutput = output.toLower();
    switch (tool) {
    case ExternalTool::Ffmpeg:
        return lowerOutput.contains("ffmpeg version");
    case ExternalTool::Ffprobe:
        return lowerOutput.contains("ffprobe version");
    case ExternalTool::Bsdtar:
        return lowerOutput.contains("bsdtar") || lowerOutput.contains("libarchive");
    }
    return false;
}

void ExternalToolRegistry::setStatus(const ToolStatus &status)
{
    statuses_.insert(status.tool, status);
    emit toolStatusChanged(status);
}

void ExternalToolRegistry::handleProcessFinished(const ProcessId &processId,
                                                 const ProcessResult &result)
{
    const auto pendingIterator = pendingProbes_.find(processId);
    if (pendingIterator == pendingProbes_.end()) {
        return;
    }

    const ExternalTool tool = pendingIterator.value();
    pendingProbes_.erase(pendingIterator);
    const ToolStatus currentStatus = status(tool);
    const QByteArray combinedOutput = result.standardOutput + '\n' + result.standardError;
    if (!result.succeeded() || !probeIdentityMatches(tool, combinedOutput)) {
        AppError error = result.error.value_or(AppError{
            .category = ErrorCategory::MissingCapability,
            .severity = ErrorSeverity::Warning,
            .code = QStringLiteral("tool.probe_unsupported"),
            .userMessage =
                QStringLiteral("%1 did not pass its capability probe.")
                    .arg(externalToolDisplayName(tool)),
            .technicalDetails = QString::fromUtf8(combinedOutput.left(512)).simplified(),
            .taskId = {},
            .recoveryActions = {},
        });
        setStatus(ToolStatus{
            .tool = tool,
            .availability = ToolAvailability::Unsupported,
            .executablePath = currentStatus.executablePath,
            .versionSummary = {},
            .error = std::move(error),
        });
        return;
    }

    const QString versionSummary =
        QString::fromUtf8(combinedOutput).section(QLatin1Char('\n'), 0, 0).simplified();
    setStatus(ToolStatus{
        .tool = tool,
        .availability = ToolAvailability::Available,
        .executablePath = currentStatus.executablePath,
        .versionSummary = versionSummary,
        .error = std::nullopt,
    });
}

} // namespace utilityforge::core
