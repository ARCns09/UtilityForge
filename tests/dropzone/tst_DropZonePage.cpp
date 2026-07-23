#include "core/tasks/TaskManager.h"
#include "modules/dropzone/DropZonePage.h"

#include <QAction>
#include <QLabel>
#include <QTableView>
#include <QTest>
#include <QToolBar>

using utilityforge::core::TaskManager;
using utilityforge::dropzone::DropZonePage;

class DropZonePageTest final : public QObject
{
    Q_OBJECT

private slots:
    void shellIsHonestAndNonInteractive();
    void taskStatusUsesEventDrivenCounts();
};

void DropZonePageTest::shellIsHonestAndNonInteractive()
{
    TaskManager taskManager(2);
    DropZonePage page(&taskManager);

    QVERIFY(!page.acceptDrops());
    QToolBar *const toolBar = page.findChild<QToolBar *>(QStringLiteral("dropZoneToolBar"));
    QVERIFY(toolBar);
    for (QAction *const action : toolBar->actions()) {
        if (!action->isSeparator()) {
            QVERIFY(!action->isEnabled());
        }
    }

    QTableView *const queueView =
        page.findChild<QTableView *>(QStringLiteral("dropQueueView"));
    QVERIFY(queueView);
    QCOMPARE(queueView->model()->rowCount(), 0);
    QCOMPARE(queueView->model()->columnCount(), 6);

    QLabel *const emptyState =
        page.findChild<QLabel *>(QStringLiteral("dropZoneEmptyState"));
    QVERIFY(emptyState);
    QVERIFY(emptyState->text().contains(QStringLiteral("not enabled")));
}

void DropZonePageTest::taskStatusUsesEventDrivenCounts()
{
    TaskManager taskManager(2);
    DropZonePage page(&taskManager);
    QLabel *const taskStatus =
        page.findChild<QLabel *>(QStringLiteral("dropZoneTaskStatus"));
    QVERIFY(taskStatus);
    QCOMPARE(taskStatus->text(), QStringLiteral("0 active · 0 queued"));

    static_cast<void>(taskManager.createTask(QStringLiteral("Fixture")));
    QCOMPARE(taskStatus->text(), QStringLiteral("1 active · 1 queued"));
}

QTEST_MAIN(DropZonePageTest)

#include "tst_DropZonePage.moc"
