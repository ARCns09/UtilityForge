#pragma once

#include "core/process/ProcessRequest.h"

#include <QObject>

#include <map>
#include <memory>

namespace utilityforge::core {

class ProcessRunner final : public QObject
{
    Q_OBJECT

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    [[nodiscard]] ProcessId start(const ProcessRequest &request);
    [[nodiscard]] bool cancel(const ProcessId &processId);
    void cancelAll();
    [[nodiscard]] int activeProcessCount() const;

signals:
    void processStarted(const utilityforge::core::ProcessId &processId);
    void standardOutputReady(const utilityforge::core::ProcessId &processId,
                             const QByteArray &data);
    void standardErrorReady(const utilityforge::core::ProcessId &processId,
                            const QByteArray &data);
    void processFinished(const utilityforge::core::ProcessId &processId,
                         const utilityforge::core::ProcessResult &result);

private:
    struct Execution;

    [[nodiscard]] std::optional<AppError> validateRequest(const ProcessRequest &request) const;
    [[nodiscard]] std::optional<AppError> validateExpectedOutput(
        const ProcessRequest &request) const;
    [[nodiscard]] QProcessEnvironment sanitizedEnvironment(
        const QProcessEnvironment &requestedEnvironment) const;
    void handleStarted(const ProcessId &processId);
    void handleOutput(const ProcessId &processId, bool isStandardError);
    void handleProcessError(const ProcessId &processId, QProcess::ProcessError processError);
    void handleFinished(const ProcessId &processId, int exitCode, QProcess::ExitStatus exitStatus);
    void handleStartTimeout(const ProcessId &processId);
    void handleExecutionTimeout(const ProcessId &processId);
    void handleTerminationTimeout(const ProcessId &processId);
    void finalize(const ProcessId &processId, ProcessResult result);
    void removeExecution(const ProcessId &processId);

    std::map<ProcessId, std::unique_ptr<Execution>> executions_;
};

} // namespace utilityforge::core
