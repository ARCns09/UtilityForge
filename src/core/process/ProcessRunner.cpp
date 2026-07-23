#include "core/process/ProcessRunner.h"

#include "core/errors/ErrorFormatter.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QThread>

#include <algorithm>

namespace utilityforge::core {
namespace {

constexpr qsizetype kMinimumOutputBytes = 1;
constexpr qsizetype kMaximumOutputBytes = 1024 * 1024;
constexpr int kMaximumTimeoutMs = 60'000;

QByteArray appendBounded(QByteArray &destination,
                         const QByteArray &source,
                         qsizetype maximumBytes,
                         bool &wasTruncated)
{
    const qsizetype remainingBytes = std::max<qsizetype>(0, maximumBytes - destination.size());
    const qsizetype acceptedBytes = std::min(remainingBytes, source.size());
    const QByteArray accepted = source.first(acceptedBytes);
    destination.append(accepted);
    if (accepted.size() < source.size()) {
        wasTruncated = true;
    }
    return accepted;
}

AppError validationError(const QString &code, const QString &details)
{
    return {
        .category = ErrorCategory::Validation,
        .severity = ErrorSeverity::Error,
        .code = code,
        .userMessage = QStringLiteral("The external process request is invalid."),
        .technicalDetails = details,
        .taskId = {},
        .recoveryActions = {},
    };
}

AppError timeoutError(const QString &code, const QString &message)
{
    return {
        .category = ErrorCategory::ProcessFailure,
        .severity = ErrorSeverity::Error,
        .code = code,
        .userMessage = message,
        .technicalDetails = {},
        .taskId = {},
        .recoveryActions = {QStringLiteral("Review the tool diagnostics and try again.")},
    };
}

AppError outputError(const QString &code, const QString &message, const QString &path)
{
    return {
        .category = ErrorCategory::ProcessFailure,
        .severity = ErrorSeverity::Error,
        .code = code,
        .userMessage = message,
        .technicalDetails = path,
        .taskId = {},
        .recoveryActions = {},
    };
}

} // namespace

struct ProcessRunner::Execution {
    ProcessId id;
    ProcessRequest request;
    std::unique_ptr<QProcess> process;
    std::unique_ptr<QTimer> startTimer;
    std::unique_ptr<QTimer> executionTimer;
    std::unique_ptr<QTimer> terminationTimer;
    QByteArray standardOutput;
    QByteArray standardError;
    bool standardOutputTruncated{false};
    bool standardErrorTruncated{false};
    bool cancellationRequested{false};
    bool isFinalized{false};
    std::optional<AppError> forcedError;
};

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<ProcessId>();
    qRegisterMetaType<ProcessResult>();
}

ProcessRunner::~ProcessRunner()
{
    for (auto &[processId, execution] : executions_) {
        static_cast<void>(processId);
        if (execution->process->state() != QProcess::NotRunning) {
            execution->process->kill();
        }
    }
}

ProcessId ProcessRunner::start(const ProcessRequest &request)
{
    if (thread() != QThread::currentThread()) {
        return {};
    }

    const ProcessId processId = QUuid::createUuid();
    if (const auto error = validateRequest(request); error.has_value()) {
        ProcessResult result;
        result.error = error;
        QTimer::singleShot(0, this, [this, processId, result]() {
            emit processFinished(processId, result);
        });
        return processId;
    }

    auto execution = std::make_unique<Execution>();
    execution->id = processId;
    execution->request = request;
    execution->process = std::make_unique<QProcess>();
    execution->startTimer = std::make_unique<QTimer>();
    execution->executionTimer = std::make_unique<QTimer>();
    execution->terminationTimer = std::make_unique<QTimer>();

    execution->startTimer->setSingleShot(true);
    execution->executionTimer->setSingleShot(true);
    execution->terminationTimer->setSingleShot(true);

    QProcess *const process = execution->process.get();
    QTimer *const startTimer = execution->startTimer.get();
    QTimer *const executionTimer = execution->executionTimer.get();
    QTimer *const terminationTimer = execution->terminationTimer.get();

    connect(process, &QProcess::started, this, [this, processId]() {
        handleStarted(processId);
    });
    connect(process, &QProcess::readyReadStandardOutput, this, [this, processId]() {
        handleOutput(processId, false);
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, processId]() {
        handleOutput(processId, true);
    });
    connect(
        process,
        &QProcess::errorOccurred,
        this,
        [this, processId](QProcess::ProcessError error) {
            handleProcessError(processId, error);
        });
    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, processId](int exitCode, QProcess::ExitStatus exitStatus) {
                handleFinished(processId, exitCode, exitStatus);
            });
    connect(startTimer, &QTimer::timeout, this, [this, processId]() {
        handleStartTimeout(processId);
    });
    connect(executionTimer, &QTimer::timeout, this, [this, processId]() {
        handleExecutionTimeout(processId);
    });
    connect(terminationTimer, &QTimer::timeout, this, [this, processId]() {
        handleTerminationTimeout(processId);
    });

    process->setProgram(QFileInfo(request.programPath).absoluteFilePath());
    process->setArguments(request.arguments);
    if (!request.workingDirectory.isEmpty()) {
        process->setWorkingDirectory(QDir::cleanPath(request.workingDirectory));
    }
    process->setProcessEnvironment(sanitizedEnvironment(request.environment));
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->setInputChannelMode(QProcess::ManagedInputChannel);

    executions_.emplace(processId, std::move(execution));
    startTimer->start(request.startTimeoutMs);
    process->start(QIODevice::ReadOnly);
    return processId;
}

bool ProcessRunner::cancel(const ProcessId &processId)
{
    if (thread() != QThread::currentThread()) {
        return false;
    }

    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized
        || executionIterator->second->cancellationRequested) {
        return false;
    }

    Execution &execution = *executionIterator->second;
    execution.cancellationRequested = true;
    execution.startTimer->stop();
    execution.executionTimer->stop();

    if (execution.process->state() == QProcess::NotRunning) {
        ProcessResult result;
        result.wasCancelled = true;
        finalize(processId, std::move(result));
        return true;
    }

    execution.process->terminate();
    execution.terminationTimer->start(execution.request.terminationGraceMs);
    return true;
}

void ProcessRunner::cancelAll()
{
    if (thread() != QThread::currentThread()) {
        return;
    }

    QList<ProcessId> processIds;
    processIds.reserve(static_cast<qsizetype>(executions_.size()));
    for (const auto &[processId, execution] : executions_) {
        if (!execution->isFinalized) {
            processIds.append(processId);
        }
    }
    for (const ProcessId &processId : processIds) {
        static_cast<void>(cancel(processId));
    }
}

int ProcessRunner::activeProcessCount() const
{
    return static_cast<int>(std::count_if(
        executions_.cbegin(), executions_.cend(), [](const auto &entry) {
            return !entry.second->isFinalized;
        }));
}

std::optional<AppError> ProcessRunner::validateRequest(const ProcessRequest &request) const
{
    if (request.programPath.isEmpty() || !QDir::isAbsolutePath(request.programPath)) {
        return validationError(QStringLiteral("process.program_not_absolute"),
                               request.programPath);
    }

    const QFileInfo programInfo(request.programPath);
    if (!programInfo.exists() || !programInfo.isFile() || !programInfo.isExecutable()) {
        return validationError(QStringLiteral("process.program_not_executable"),
                               request.programPath);
    }

    if (!request.workingDirectory.isEmpty()) {
        const QFileInfo workingDirectoryInfo(request.workingDirectory);
        if (!QDir::isAbsolutePath(request.workingDirectory)
            || !workingDirectoryInfo.isDir()) {
            return validationError(QStringLiteral("process.working_directory_invalid"),
                                   request.workingDirectory);
        }
    }

    if (request.expectedOutputKind == ExpectedOutputKind::None
        && !request.expectedOutputPath.isEmpty()) {
        return validationError(
            QStringLiteral("process.unexpected_output_contract"),
            request.expectedOutputPath);
    }
    if (request.expectedOutputKind != ExpectedOutputKind::None
        && (request.expectedOutputPath.isEmpty()
            || !QDir::isAbsolutePath(request.expectedOutputPath))) {
        return validationError(
            QStringLiteral("process.output_path_invalid"),
            request.expectedOutputPath);
    }

    if (request.maximumOutputBytes < kMinimumOutputBytes
        || request.maximumOutputBytes > kMaximumOutputBytes) {
        return validationError(QStringLiteral("process.output_limit_invalid"),
                               QString::number(request.maximumOutputBytes));
    }

    if (request.startTimeoutMs <= 0 || request.startTimeoutMs > kMaximumTimeoutMs
        || request.executionTimeoutMs < 0
        || request.executionTimeoutMs > kMaximumTimeoutMs
        || request.terminationGraceMs < 0
        || request.terminationGraceMs > kMaximumTimeoutMs) {
        return validationError(QStringLiteral("process.timeout_invalid"), {});
    }

    return std::nullopt;
}

std::optional<AppError> ProcessRunner::validateExpectedOutput(
    const ProcessRequest &request) const
{
    if (request.expectedOutputKind == ExpectedOutputKind::None) {
        return std::nullopt;
    }

    const QFileInfo outputInfo(request.expectedOutputPath);
    if (!outputInfo.exists()) {
        return outputError(
            QStringLiteral("process.output_missing"),
            QStringLiteral("The external tool did not create the expected output."),
            request.expectedOutputPath);
    }

    const bool typeMatches = request.expectedOutputKind == ExpectedOutputKind::RegularFile
        ? outputInfo.isFile()
        : outputInfo.isDir();
    if (!typeMatches) {
        return outputError(
            QStringLiteral("process.output_type_invalid"),
            QStringLiteral("The external tool created an unexpected output type."),
            request.expectedOutputPath);
    }

    return std::nullopt;
}

QProcessEnvironment ProcessRunner::sanitizedEnvironment(
    const QProcessEnvironment &requestedEnvironment) const
{
    QProcessEnvironment environment = requestedEnvironment.isEmpty()
        ? QProcessEnvironment::systemEnvironment()
        : requestedEnvironment;

    const QStringList injectionVariables = {
        QStringLiteral("LD_PRELOAD"),
        QStringLiteral("LD_AUDIT"),
        QStringLiteral("DYLD_INSERT_LIBRARIES"),
        QStringLiteral("DYLD_LIBRARY_PATH"),
        QStringLiteral("QT_PLUGIN_PATH"),
        QStringLiteral("QT_QPA_PLATFORM_PLUGIN_PATH"),
    };
    for (const QString &variable : injectionVariables) {
        environment.remove(variable);
    }
    return environment;
}

void ProcessRunner::handleStarted(const ProcessId &processId)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    Execution &execution = *executionIterator->second;
    execution.startTimer->stop();
    execution.process->closeWriteChannel();
    if (execution.request.executionTimeoutMs > 0) {
        execution.executionTimer->start(execution.request.executionTimeoutMs);
    }
    emit processStarted(processId);
}

void ProcessRunner::handleOutput(const ProcessId &processId, bool isStandardError)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    Execution &execution = *executionIterator->second;
    const QByteArray source = isStandardError ? execution.process->readAllStandardError()
                                              : execution.process->readAllStandardOutput();
    QByteArray &destination =
        isStandardError ? execution.standardError : execution.standardOutput;
    bool &wasTruncated = isStandardError ? execution.standardErrorTruncated
                                         : execution.standardOutputTruncated;
    const QByteArray accepted =
        appendBounded(destination, source, execution.request.maximumOutputBytes, wasTruncated);
    if (accepted.isEmpty()) {
        return;
    }

    if (isStandardError) {
        emit standardErrorReady(processId, accepted);
    } else {
        emit standardOutputReady(processId, accepted);
    }
}

void ProcessRunner::handleProcessError(const ProcessId &processId,
                                       QProcess::ProcessError processError)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    Execution &execution = *executionIterator->second;
    if (processError == QProcess::FailedToStart) {
        ProcessResult result;
        result.error = ErrorFormatter::processStartError(execution.process->errorString());
        finalize(processId, std::move(result));
        return;
    }

    if (processError != QProcess::UnknownError && !execution.cancellationRequested) {
        execution.forcedError = ErrorFormatter::processFailure(
            -1, execution.process->errorString());
    }
}

void ProcessRunner::handleFinished(const ProcessId &processId,
                                   int exitCode,
                                   QProcess::ExitStatus exitStatus)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    handleOutput(processId, false);
    handleOutput(processId, true);

    Execution &execution = *executionIterator->second;
    ProcessResult result{
        .exitCode = exitCode,
        .exitStatus = exitStatus,
        .standardOutput = execution.standardOutput,
        .standardError = execution.standardError,
        .standardOutputTruncated = execution.standardOutputTruncated,
        .standardErrorTruncated = execution.standardErrorTruncated,
        .wasCancelled = execution.cancellationRequested,
        .error = execution.forcedError,
    };

    if (!result.wasCancelled && !result.error.has_value()
        && (exitStatus != QProcess::NormalExit || exitCode != 0)) {
        result.error = ErrorFormatter::processFailure(
            exitCode, QString::fromUtf8(result.standardError));
    }
    if (!result.wasCancelled && !result.error.has_value()) {
        result.error = validateExpectedOutput(execution.request);
    }
    finalize(processId, std::move(result));
}

void ProcessRunner::handleStartTimeout(const ProcessId &processId)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    Execution &execution = *executionIterator->second;
    execution.forcedError = timeoutError(
        QStringLiteral("process.start_timeout"),
        QStringLiteral("The external tool did not start in time."));
    execution.process->kill();
    ProcessResult result;
    result.error = execution.forcedError;
    finalize(processId, std::move(result));
}

void ProcessRunner::handleExecutionTimeout(const ProcessId &processId)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    Execution &execution = *executionIterator->second;
    execution.forcedError = timeoutError(
        QStringLiteral("process.execution_timeout"),
        QStringLiteral("The external tool did not finish in time."));
    execution.process->kill();
}

void ProcessRunner::handleTerminationTimeout(const ProcessId &processId)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator == executions_.end() || executionIterator->second->isFinalized) {
        return;
    }

    Execution &execution = *executionIterator->second;
    if (execution.process->state() != QProcess::NotRunning) {
        execution.process->kill();
    }
}

void ProcessRunner::finalize(const ProcessId &processId, ProcessResult result)
{
    const auto executionIterator = executions_.find(processId);
    if (executionIterator != executions_.end()) {
        Execution &execution = *executionIterator->second;
        if (execution.isFinalized) {
            return;
        }
        execution.isFinalized = true;
        execution.startTimer->stop();
        execution.executionTimer->stop();
        execution.terminationTimer->stop();
    }

    emit processFinished(processId, result);
    QTimer::singleShot(0, this, [this, processId]() {
        removeExecution(processId);
    });
}

void ProcessRunner::removeExecution(const ProcessId &processId)
{
    executions_.erase(processId);
}

} // namespace utilityforge::core
