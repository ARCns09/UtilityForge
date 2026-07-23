#pragma once

#include <QMainWindow>
#include <QUuid>

class QLabel;
class QListWidget;
class QToolButton;

namespace utilityforge::core {
class AppPaths;
class ExternalToolRegistry;
class NotificationCenter;
class SettingsService;
class TaskManager;
struct Notification;
}

namespace utilityforge::dropzone {
class DropZonePage;
}

namespace utilityforge::app {

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const core::AppPaths *paths,
               core::SettingsService *settings,
               core::TaskManager *taskManager,
               core::ExternalToolRegistry *toolRegistry,
               core::NotificationCenter *notificationCenter,
               QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createMenus();
    void openSettings();
    void showAbout();
    void showNotification(const core::Notification &notification);
    void hideNotification(const QUuid &notificationId);
    void updateStatusBar(int activeTasks, int queuedTasks);

    const core::AppPaths *paths_;
    core::SettingsService *settings_;
    core::TaskManager *taskManager_;
    core::ExternalToolRegistry *toolRegistry_;
    core::NotificationCenter *notificationCenter_;
    QListWidget *sidebar_;
    dropzone::DropZonePage *dropZonePage_;
    QWidget *notificationBanner_;
    QLabel *notificationLabel_;
    QToolButton *notificationDismissButton_;
    QLabel *taskStatusLabel_;
    QUuid visibleNotificationId_;
    bool isWaitingForCancellation_{false};
};

} // namespace utilityforge::app
