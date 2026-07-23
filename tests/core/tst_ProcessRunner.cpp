#include "core/process/ProcessRunner.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

using utilityforge::core::ProcessId;
using utilityforge::core::ProcessRequest;
using utilityforge::core::ProcessResult;
using utilityforge::core::ProcessRunner;
using utilityforge::core::ExpectedOutputKind;

class ProcessRunnerTest final : public QObject
{
    Q_OBJECT

private slots:
    void argumentsAreTransportedLiterally();
    void nonZeroExitCreatesStructuredError();
    void cancellationIsIdempotent();
    void relativeExecutableIsRejected();
    void eventLoopRemainsResponsive();
    void missingExpectedOutputIsFailure();
    void expectedOutputIsValidated();
    void capturedOutputIsBounded();
};

void ProcessRunnerTest::argumentsAreTransportedLiterally()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    const QStringList arguments = {
        QStringLiteral("plain value"),
        QStringLiteral("$HOME"),
        QStringLiteral("$(touch should-not-exist)"),
        QStringLiteral("semi;colon"),
        QStringLiteral("line\nbreak"),
        QStringLiteral("यूटिलिटी"),
    };
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = arguments,
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 1'500,
    };

    const ProcessId processId = runner.start(request);
    QVERIFY(!processId.isNull());
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);

    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(result.succeeded());
    QList<QByteArray> outputArguments = result.standardOutput.split('\0');
    outputArguments.removeLast();
    QCOMPARE(outputArguments.size(), arguments.size());
    for (qsizetype index = 0; index < arguments.size(); ++index) {
        QCOMPARE(QString::fromUtf8(outputArguments.at(index)), arguments.at(index));
    }
}

void ProcessRunnerTest::nonZeroExitCreatesStructuredError()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = {QStringLiteral("--exit-code"), QStringLiteral("7")},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 1'500,
    };

    static_cast<void>(runner.start(request));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);

    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(!result.succeeded());
    QVERIFY(result.error.has_value());
    QCOMPARE(result.error->code, QStringLiteral("process.nonzero_exit"));
}

void ProcessRunnerTest::cancellationIsIdempotent()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    ProcessId processId;
    connect(
        &runner,
        &ProcessRunner::processStarted,
        &runner,
        [&runner, &processId](const auto &id) {
            if (id == processId) {
                QVERIFY(runner.cancel(id));
                QVERIFY(!runner.cancel(id));
            }
        });
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = {QStringLiteral("--delay-ms"), QStringLiteral("10000")},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 3'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 100,
    };

    processId = runner.start(request);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);

    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(result.wasCancelled);
    QVERIFY(!result.succeeded());
}

void ProcessRunnerTest::relativeExecutableIsRejected()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    const ProcessRequest request{
        .programPath = QStringLiteral("not-an-absolute-program"),
        .arguments = {},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 1'500,
    };

    static_cast<void>(runner.start(request));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 1'000);
    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(result.error.has_value());
    QCOMPARE(result.error->code, QStringLiteral("process.program_not_absolute"));
}

void ProcessRunnerTest::eventLoopRemainsResponsive()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    bool eventWasProcessed = false;
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = {QStringLiteral("--delay-ms"), QStringLiteral("50")},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 1'000,
        .terminationGraceMs = 1'500,
    };

    static_cast<void>(runner.start(request));
    QTimer::singleShot(0, &runner, [&eventWasProcessed]() {
        eventWasProcessed = true;
    });

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);
    QVERIFY(eventWasProcessed);
    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(result.succeeded());
}

void ProcessRunnerTest::missingExpectedOutputIsFailure()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    const QString missingOutput =
        temporaryDirectory.filePath(QStringLiteral("missing.bin"));
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = {},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = missingOutput,
        .expectedOutputKind = ExpectedOutputKind::RegularFile,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 1'500,
    };

    static_cast<void>(runner.start(request));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);
    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(!result.succeeded());
    QVERIFY(result.error.has_value());
    QCOMPARE(result.error->code, QStringLiteral("process.output_missing"));
}

void ProcessRunnerTest::expectedOutputIsValidated()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    const QString outputPath =
        temporaryDirectory.filePath(QStringLiteral("created.bin"));
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = {QStringLiteral("--create-output"), outputPath},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = outputPath,
        .expectedOutputKind = ExpectedOutputKind::RegularFile,
        .maximumOutputBytes = 64 * 1024,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 1'500,
    };

    static_cast<void>(runner.start(request));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);
    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(result.succeeded());
}

void ProcessRunnerTest::capturedOutputIsBounded()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::processFinished);
    const ProcessRequest request{
        .programPath = QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH),
        .arguments = {QStringLiteral("--stdout-bytes"), QStringLiteral("128")},
        .workingDirectory = {},
        .environment = {},
        .expectedOutputPath = {},
        .expectedOutputKind = ExpectedOutputKind::None,
        .maximumOutputBytes = 16,
        .startTimeoutMs = 5'000,
        .executionTimeoutMs = 0,
        .terminationGraceMs = 1'500,
    };

    static_cast<void>(runner.start(request));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3'000);
    const ProcessResult result =
        qvariant_cast<ProcessResult>(finishedSpy.takeFirst().at(1));
    QVERIFY(result.succeeded());
    QCOMPARE(result.standardOutput.size(), 16);
    QVERIFY(result.standardOutputTruncated);
}

QTEST_GUILESS_MAIN(ProcessRunnerTest)

#include "tst_ProcessRunner.moc"
