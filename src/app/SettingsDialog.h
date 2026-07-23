#pragma once

#include <QDialog>

class QLabel;
class QSpinBox;
class QTableWidget;

namespace utilityforge::core {
class AppPaths;
class ExternalToolRegistry;
class SettingsService;
class TaskManager;
struct ToolStatus;
}

namespace utilityforge::app {

class SettingsDialog final : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(const core::AppPaths *paths,
                   core::SettingsService *settings,
                   core::TaskManager *taskManager,
                   core::ExternalToolRegistry *toolRegistry,
                   QWidget *parent = nullptr);

private:
    void handleAccepted();
    void updateToolStatus(const core::ToolStatus &status);

    const core::AppPaths *paths_;
    core::SettingsService *settings_;
    core::TaskManager *taskManager_;
    core::ExternalToolRegistry *toolRegistry_;
    QSpinBox *concurrencySpinBox_;
    QTableWidget *toolTable_;
    QLabel *saveStatusLabel_;
};

} // namespace utilityforge::app
