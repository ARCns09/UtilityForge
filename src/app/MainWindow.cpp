#include "app/MainWindow.h"

#include "app/SettingsDialog.h"
#include "core/notifications/NotificationCenter.h"
#include "core/settings/AppPaths.h"
#include "core/settings/SettingsService.h"
#include "core/tasks/TaskManager.h"
#include "modules/dropzone/DropZonePage.h"

#include <QAction>
#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace utilityforge::app {

MainWindow::MainWindow(const core::AppPaths *paths,
                       core::SettingsService *settings,
                       core::TaskManager *taskManager,
                       core::ExternalToolRegistry *toolRegistry,
                       core::NotificationCenter *notificationCenter,
                       QWidget *parent)
    : QMainWindow(parent)
    , paths_(paths)
    , settings_(settings)
    , taskManager_(taskManager)
    , toolRegistry_(toolRegistry)
    , notificationCenter_(notificationCenter)
    , sidebar_(new QListWidget(this))
    , dropZonePage_(new dropzone::DropZonePage(taskManager, this))
    , notificationBanner_(new QFrame(this))
    , notificationLabel_(new QLabel(notificationBanner_))
    , notificationDismissButton_(new QToolButton(notificationBanner_))
    , taskStatusLabel_(new QLabel(this))
{
    Q_ASSERT(paths_);
    Q_ASSERT(settings_);
    Q_ASSERT(taskManager_);
    Q_ASSERT(toolRegistry_);
    Q_ASSERT(notificationCenter_);

    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(tr("UtilityForge"));
    resize(900, 580);
    setMinimumSize(720, 460);

    sidebar_->setObjectName(QStringLiteral("moduleSidebar"));
    sidebar_->setFixedWidth(148);
    sidebar_->setSpacing(1);
    sidebar_->setUniformItemSizes(true);
    sidebar_->setSelectionMode(QAbstractItemView::SingleSelection);
    auto *const dropZoneItem = new QListWidgetItem(
        QIcon::fromTheme(
            QStringLiteral("document-send"),
            style()->standardIcon(QStyle::SP_FileDialogDetailedView)),
        tr("DropZone"),
        sidebar_);
    dropZoneItem->setData(Qt::UserRole, QStringLiteral("dropzone"));
    sidebar_->setCurrentItem(dropZoneItem);

    notificationBanner_->setObjectName(QStringLiteral("notificationBanner"));
    notificationBanner_->setVisible(false);
    auto *const notificationLayout = new QHBoxLayout(notificationBanner_);
    notificationLayout->setContentsMargins(8, 4, 4, 4);
    notificationLabel_->setObjectName(QStringLiteral("notificationLabel"));
    notificationLabel_->setTextFormat(Qt::PlainText);
    notificationLabel_->setWordWrap(true);
    notificationDismissButton_->setObjectName(QStringLiteral("dismissNotificationButton"));
    notificationDismissButton_->setAutoRaise(true);
    notificationDismissButton_->setIcon(
        QIcon::fromTheme(
            QStringLiteral("window-close"),
            style()->standardIcon(QStyle::SP_TitleBarCloseButton)));
    notificationDismissButton_->setToolTip(tr("Dismiss notification"));
    notificationLayout->addWidget(notificationLabel_, 1);
    notificationLayout->addWidget(notificationDismissButton_);

    auto *const pageStack = new QStackedWidget(this);
    pageStack->setObjectName(QStringLiteral("moduleStack"));
    pageStack->addWidget(dropZonePage_);

    auto *const content = new QWidget(this);
    auto *const contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(notificationBanner_);
    contentLayout->addWidget(pageStack, 1);

    auto *const centralWidget = new QWidget(this);
    auto *const centralLayout = new QHBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(sidebar_);
    centralLayout->addWidget(content, 1);
    setCentralWidget(centralWidget);

    taskStatusLabel_->setObjectName(QStringLiteral("mainTaskStatus"));
    statusBar()->addPermanentWidget(taskStatusLabel_);
    statusBar()->showMessage(tr("Ready"));
    updateStatusBar(taskManager_->activeTaskCount(), taskManager_->queuedTaskCount());

    createMenus();

    connect(notificationCenter_,
            &core::NotificationCenter::notificationPosted,
            this,
            &MainWindow::showNotification);
    connect(notificationCenter_,
            &core::NotificationCenter::notificationDismissed,
            this,
            &MainWindow::hideNotification);
    connect(notificationDismissButton_, &QToolButton::clicked, this, [this]() {
        static_cast<void>(notificationCenter_->dismiss(visibleNotificationId_));
    });
    connect(taskManager_,
            &core::TaskManager::countsChanged,
            this,
            &MainWindow::updateStatusBar);
    connect(taskManager_,
            &core::TaskManager::countsChanged,
            this,
            [this](int activeTasks, int) {
                if (isWaitingForCancellation_ && activeTasks == 0) {
                    isWaitingForCancellation_ = false;
                    close();
                }
            });

    const QByteArray geometry = settings_->windowGeometry();
    if (!geometry.isEmpty()) {
        static_cast<void>(restoreGeometry(geometry));
    }
    const QByteArray state = settings_->windowState();
    if (!state.isEmpty()) {
        static_cast<void>(restoreState(state));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (taskManager_->activeTaskCount() > 0) {
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            tr("Active tasks"),
            tr("Cancel all active tasks and exit when cancellation finishes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            event->ignore();
            return;
        }

        isWaitingForCancellation_ = true;
        taskManager_->cancelAll();
        if (taskManager_->activeTaskCount() > 0) {
            statusBar()->showMessage(tr("Cancelling active tasks…"));
            event->ignore();
            return;
        }
    }

    settings_->setWindowGeometry(saveGeometry());
    settings_->setWindowState(saveState());
    if (const auto error = settings_->sync(); error.has_value()) {
        static_cast<void>(notificationCenter_->postError(*error));
    }
    event->accept();
}

void MainWindow::createMenus()
{
    QMenu *const fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *const quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *const editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *const settingsAction = editMenu->addAction(tr("&Settings…"));
    settingsAction->setObjectName(QStringLiteral("settingsAction"));
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    QMenu *const helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *const aboutAction = helpMenu->addAction(tr("&About UtilityForge"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(paths_, settings_, taskManager_, toolRegistry_, this);
    static_cast<void>(dialog.exec());
}

void MainWindow::showAbout()
{
    QMessageBox messageBox(this);
    messageBox.setWindowTitle(tr("About UtilityForge"));
    messageBox.setTextFormat(Qt::PlainText);
    messageBox.setText(
        tr("UtilityForge\n"
           "A native Linux utility shell built with C++20 and Qt 6 Widgets."));
    messageBox.setIconPixmap(
        style()->standardIcon(QStyle::SP_ComputerIcon).pixmap(QSize(48, 48)));
    static_cast<void>(messageBox.exec());
}

void MainWindow::showNotification(const core::Notification &notification)
{
    visibleNotificationId_ = notification.id;
    notificationLabel_->setText(
        QStringLiteral("%1 — %2").arg(notification.title, notification.message));
    notificationBanner_->setVisible(true);
}

void MainWindow::hideNotification(const QUuid &notificationId)
{
    if (notificationId != visibleNotificationId_) {
        return;
    }
    visibleNotificationId_ = {};
    notificationBanner_->setVisible(false);
}

void MainWindow::updateStatusBar(int activeTasks, int queuedTasks)
{
    taskStatusLabel_->setText(
        tr("%1 active · %2 queued").arg(activeTasks).arg(queuedTasks));
}

} // namespace utilityforge::app
