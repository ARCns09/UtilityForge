#include "app/ApplicationBootstrap.h"

#include "app/MainWindow.h"
#include "core/errors/ErrorFormatter.h"
#include "core/notifications/NotificationCenter.h"
#include "core/process/ExternalToolRegistry.h"
#include "core/process/ProcessRunner.h"
#include "core/settings/AppPaths.h"
#include "core/settings/SettingsService.h"
#include "core/tasks/TaskManager.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyle>

#include <cstdlib>

namespace utilityforge::app {
namespace {

const QString kApplicationId = QStringLiteral("utilityforge");

int showFatalStartupError(const core::AppError &error)
{
    QMessageBox messageBox;
    messageBox.setWindowTitle(QStringLiteral("UtilityForge"));
    messageBox.setTextFormat(Qt::PlainText);
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setText(core::ErrorFormatter::userSummary(error));
    messageBox.setDetailedText(core::ErrorFormatter::technicalSummary(error));
    static_cast<void>(messageBox.exec());
    return EXIT_FAILURE;
}

} // namespace

int ApplicationBootstrap::run(QApplication &application) const
{
    QCoreApplication::setOrganizationName(QStringLiteral("UtilityForge"));
    QCoreApplication::setApplicationName(QStringLiteral("UtilityForge"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    application.setDesktopFileName(QStringLiteral("io.github.ARCns09.UtilityForge"));
    application.setWindowIcon(
        application.style()->standardIcon(QStyle::SP_ComputerIcon));

    const core::AppPaths paths = core::AppPaths::forApplication(kApplicationId);
    if (const auto error = paths.ensureDirectories(); error.has_value()) {
        return showFatalStartupError(*error);
    }

    core::SettingsService settings(paths);
    if (const auto error = settings.initialize(); error.has_value()) {
        return showFatalStartupError(*error);
    }

    core::NotificationCenter notificationCenter;
    core::TaskManager taskManager(settings.maxConcurrentTasks());
    core::ProcessRunner processRunner;
    core::ExternalToolRegistry toolRegistry(&processRunner);
    MainWindow mainWindow(
        &paths, &settings, &taskManager, &toolRegistry, &notificationCenter);

    QObject::connect(
        &application,
        &QCoreApplication::aboutToQuit,
        &processRunner,
        [&processRunner]() {
            processRunner.cancelAll();
        });

    mainWindow.show();
    return application.exec();
}

} // namespace utilityforge::app
