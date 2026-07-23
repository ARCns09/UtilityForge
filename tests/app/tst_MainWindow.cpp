#include "app/MainWindow.h"

#include "core/notifications/NotificationCenter.h"
#include "core/process/ExternalToolRegistry.h"
#include "core/process/ProcessRunner.h"
#include "core/settings/AppPaths.h"
#include "core/settings/SettingsService.h"
#include "core/tasks/TaskManager.h"

#include <QAbstractButton>
#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

using utilityforge::app::MainWindow;
using utilityforge::core::AppPaths;
using utilityforge::core::ExternalToolRegistry;
using utilityforge::core::NotificationCenter;
using utilityforge::core::NotificationKind;
using utilityforge::core::ProcessRunner;
using utilityforge::core::SettingsService;
using utilityforge::core::TaskManager;

class MainWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void startupHasOnlyDropZoneAndDoesNotProbeTools();
    void notificationAppearsInsideWindow();
    void closeCancelsQueuedWork();
};

void MainWindowTest::startupHasOnlyDropZoneAndDoesNotProbeTools()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));
    QVERIFY(!paths.ensureDirectories().has_value());
    SettingsService settings(paths);
    QVERIFY(!settings.initialize().has_value());
    TaskManager taskManager(2);
    ProcessRunner processRunner;
    ExternalToolRegistry toolRegistry(&processRunner);
    NotificationCenter notificationCenter;
    QSignalSpy processSpy(&processRunner, &ProcessRunner::processStarted);

    MainWindow window(
        &paths, &settings, &taskManager, &toolRegistry, &notificationCenter);
    window.show();
    QCoreApplication::processEvents();

    QListWidget *const sidebar =
        window.findChild<QListWidget *>(QStringLiteral("moduleSidebar"));
    QVERIFY(sidebar);
    QCOMPARE(sidebar->count(), 1);
    QCOMPARE(sidebar->item(0)->text(), QStringLiteral("DropZone"));
    QCOMPARE(processSpy.count(), 0);
}

void MainWindowTest::notificationAppearsInsideWindow()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));
    QVERIFY(!paths.ensureDirectories().has_value());
    SettingsService settings(paths);
    QVERIFY(!settings.initialize().has_value());
    TaskManager taskManager(2);
    ProcessRunner processRunner;
    ExternalToolRegistry toolRegistry(&processRunner);
    NotificationCenter notificationCenter;
    MainWindow window(
        &paths, &settings, &taskManager, &toolRegistry, &notificationCenter);
    window.show();

    const auto notificationId = notificationCenter.post(
        NotificationKind::Information,
        QStringLiteral("Fixture"),
        QStringLiteral("Visible notification"),
        true);
    QCoreApplication::processEvents();

    QLabel *const notificationLabel =
        window.findChild<QLabel *>(QStringLiteral("notificationLabel"));
    QVERIFY(notificationLabel);
    QVERIFY(notificationLabel->isVisible());
    QVERIFY(notificationLabel->text().contains(QStringLiteral("Visible notification")));
    QVERIFY(notificationCenter.dismiss(notificationId));
    QCoreApplication::processEvents();
    QVERIFY(!notificationLabel->isVisible());
    QVERIFY(!notificationCenter.dismiss(notificationId));
}

void MainWindowTest::closeCancelsQueuedWork()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));
    QVERIFY(!paths.ensureDirectories().has_value());
    SettingsService settings(paths);
    QVERIFY(!settings.initialize().has_value());
    TaskManager taskManager(2);
    ProcessRunner processRunner;
    ExternalToolRegistry toolRegistry(&processRunner);
    NotificationCenter notificationCenter;
    MainWindow window(
        &paths, &settings, &taskManager, &toolRegistry, &notificationCenter);
    window.show();
    const auto taskId = taskManager.createTask(QStringLiteral("Queued fixture"));

    QTimer::singleShot(0, &window, []() {
        auto *const messageBox =
            qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(messageBox);
        QAbstractButton *const yesButton = messageBox->button(QMessageBox::Yes);
        QVERIFY(yesButton);
        yesButton->click();
    });
    window.close();

    QTRY_VERIFY(!window.isVisible());
    const auto record = taskManager.task(taskId);
    QVERIFY(record.has_value());
    QCOMPARE(record->state, utilityforge::core::TaskState::Cancelled);
}

QTEST_MAIN(MainWindowTest)

#include "tst_MainWindow.moc"
