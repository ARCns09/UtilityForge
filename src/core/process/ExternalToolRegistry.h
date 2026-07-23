#pragma once

#include "core/process/ProcessRunner.h"

#include <QHash>
#include <QMetaType>
#include <QObject>

#include <optional>

namespace utilityforge::core {

enum class ExternalTool {
    Ffmpeg,
    Ffprobe,
    Bsdtar
};

enum class ToolAvailability {
    Unknown,
    Checking,
    Available,
    Missing,
    Unsupported
};

struct ToolStatus {
    ExternalTool tool{ExternalTool::Ffmpeg};
    ToolAvailability availability{ToolAvailability::Unknown};
    QString executablePath;
    QString versionSummary;
    std::optional<AppError> error;
};

[[nodiscard]] QString externalToolDisplayName(ExternalTool tool);
[[nodiscard]] QString toolAvailabilityDisplayName(ToolAvailability availability);

class ExternalToolRegistry final : public QObject
{
    Q_OBJECT

public:
    explicit ExternalToolRegistry(ProcessRunner *processRunner, QObject *parent = nullptr);

    [[nodiscard]] ToolStatus status(ExternalTool tool) const;
    void checkTool(ExternalTool tool);
    void refreshTool(ExternalTool tool);
    void refreshAll();
    [[nodiscard]] bool setExecutableOverride(ExternalTool tool, const QString &absolutePath);

signals:
    void toolStatusChanged(const utilityforge::core::ToolStatus &status);

private:
    [[nodiscard]] QString executableName(ExternalTool tool) const;
    [[nodiscard]] QStringList probeArguments(ExternalTool tool) const;
    [[nodiscard]] bool probeIdentityMatches(ExternalTool tool, const QByteArray &output) const;
    void setStatus(const ToolStatus &status);
    void handleProcessFinished(const ProcessId &processId, const ProcessResult &result);

    ProcessRunner *processRunner_;
    QHash<ExternalTool, ToolStatus> statuses_;
    QHash<ExternalTool, QString> executableOverrides_;
    QHash<ProcessId, ExternalTool> pendingProbes_;
};

} // namespace utilityforge::core

Q_DECLARE_METATYPE(utilityforge::core::ExternalTool)
Q_DECLARE_METATYPE(utilityforge::core::ToolAvailability)
Q_DECLARE_METATYPE(utilityforge::core::ToolStatus)
