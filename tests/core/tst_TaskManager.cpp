#include "core/tasks/TaskManager.h"

#include "core/errors/AppError.h"

#include <QSignalSpy>
#include <QTest>

using utilityforge::core::TaskManager;
using utilityforge::core::TaskState;
using utilityforge::core::AppError;
using utilityforge::core::ErrorCategory;
using utilityforge::core::ErrorSeverity;

class TaskManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void validLifecycleReachesSuccess();
    void cancellationWinsCompletionRace();
    void concurrencyLimitKeepsTaskQueued();
    void failureRequiresStructuredError();
};

void TaskManagerTest::validLifecycleReachesSuccess()
{
    TaskManager manager(2);
    QSignalSpy updateSpy(&manager, &TaskManager::taskUpdated);
    const auto taskId = manager.createTask(QStringLiteral("Fixture task"));

    QVERIFY(!taskId.isNull());
    QVERIFY(manager.transitionTask(taskId, TaskState::Preparing));
    QVERIFY(manager.transitionTask(taskId, TaskState::Running));
    QVERIFY(manager.transitionTask(taskId, TaskState::Publishing));
    QVERIFY(manager.transitionTask(taskId, TaskState::Succeeded));

    const auto record = manager.task(taskId);
    QVERIFY(record.has_value());
    QCOMPARE(record->state, TaskState::Succeeded);
    QCOMPARE(record->progress.percentage, 100);
    QCOMPARE(manager.activeTaskCount(), 0);
    QCOMPARE(updateSpy.count(), 4);
}

void TaskManagerTest::cancellationWinsCompletionRace()
{
    TaskManager manager(2);
    const auto taskId = manager.createTask(QStringLiteral("Cancellable task"));
    const auto token = manager.cancellationToken(taskId);

    QVERIFY(manager.transitionTask(taskId, TaskState::Preparing));
    QVERIFY(manager.transitionTask(taskId, TaskState::Running));
    QVERIFY(manager.requestCancellation(taskId));
    QVERIFY(token.isCancellationRequested());
    QVERIFY(!manager.requestCancellation(taskId));
    QVERIFY(!manager.transitionTask(taskId, TaskState::Publishing));
    QVERIFY(!manager.transitionTask(taskId, TaskState::Succeeded));
    QVERIFY(manager.transitionTask(taskId, TaskState::Cancelled));
}

void TaskManagerTest::concurrencyLimitKeepsTaskQueued()
{
    TaskManager manager(1);
    const auto firstTask = manager.createTask(QStringLiteral("First"));
    const auto secondTask = manager.createTask(QStringLiteral("Second"));

    QVERIFY(manager.transitionTask(firstTask, TaskState::Preparing));
    QVERIFY(!manager.transitionTask(secondTask, TaskState::Preparing));
    QCOMPARE(manager.queuedTaskCount(), 1);
    QCOMPARE(manager.runningTaskCount(), 1);
}

void TaskManagerTest::failureRequiresStructuredError()
{
    TaskManager manager(1);
    const auto taskId = manager.createTask(QStringLiteral("Failure"));
    const AppError error{
        .category = ErrorCategory::Internal,
        .severity = ErrorSeverity::Error,
        .code = QStringLiteral("fixture.failure"),
        .userMessage = QStringLiteral("Fixture failed."),
        .technicalDetails = {},
        .taskId = {},
        .recoveryActions = {},
    };

    QVERIFY(!manager.failTask(taskId, error));
    QVERIFY(!manager.transitionTask(taskId, TaskState::Failed));
    QVERIFY(manager.transitionTask(taskId, TaskState::Preparing));
    QVERIFY(manager.failTask(taskId, error));

    const auto record = manager.task(taskId);
    QVERIFY(record.has_value());
    QCOMPARE(record->state, TaskState::Failed);
    QVERIFY(record->error.has_value());
    QCOMPARE(record->error->taskId, taskId.toString(QUuid::WithoutBraces));
}

QTEST_GUILESS_MAIN(TaskManagerTest)

#include "tst_TaskManager.moc"
