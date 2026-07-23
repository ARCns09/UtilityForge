#include "core/settings/SettingsService.h"

#include <QSettings>

#include <algorithm>

namespace utilityforge::core {
namespace {

constexpr int kCurrentSchemaVersion = 1;
constexpr int kDefaultConcurrentTasks = 2;
constexpr int kMinimumConcurrentTasks = 1;
constexpr int kMaximumConcurrentTasks = 4;

const QString kSchemaVersionKey = QStringLiteral("meta/schemaVersion");
const QString kWindowGeometryKey = QStringLiteral("window/geometry");
const QString kWindowStateKey = QStringLiteral("window/state");
const QString kLastOutputDirectoryKey = QStringLiteral("dropzone/lastOutputDirectory");
const QString kMaxConcurrentTasksKey = QStringLiteral("tasks/maxConcurrentTasks");

} // namespace

SettingsService::SettingsService(const AppPaths &paths)
    : settings_(
          std::make_unique<QSettings>(paths.settingsFilePath(), QSettings::IniFormat))
{
    settings_->setFallbacksEnabled(false);
}

SettingsService::~SettingsService() = default;

std::optional<AppError> SettingsService::initialize()
{
    const int storedVersion = settings_->value(kSchemaVersionKey, 0).toInt();
    if (storedVersion > kCurrentSchemaVersion) {
        return AppError{
            .category = ErrorCategory::UnsupportedInput,
            .severity = ErrorSeverity::Critical,
            .code = QStringLiteral("settings.schema_too_new"),
            .userMessage =
                QStringLiteral("The settings were created by a newer UtilityForge version."),
            .technicalDetails = QString::number(storedVersion),
            .taskId = {},
            .recoveryActions = {},
        };
    }

    if (storedVersion < kCurrentSchemaVersion) {
        settings_->setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }

    return sync();
}

std::optional<AppError> SettingsService::sync()
{
    settings_->sync();
    return statusError();
}

QByteArray SettingsService::windowGeometry() const
{
    return settings_->value(kWindowGeometryKey).toByteArray();
}

void SettingsService::setWindowGeometry(const QByteArray &geometry)
{
    settings_->setValue(kWindowGeometryKey, geometry);
}

QByteArray SettingsService::windowState() const
{
    return settings_->value(kWindowStateKey).toByteArray();
}

void SettingsService::setWindowState(const QByteArray &state)
{
    settings_->setValue(kWindowStateKey, state);
}

QString SettingsService::lastOutputDirectory() const
{
    return settings_->value(kLastOutputDirectoryKey).toString();
}

void SettingsService::setLastOutputDirectory(const QString &directory)
{
    settings_->setValue(kLastOutputDirectoryKey, directory);
}

int SettingsService::maxConcurrentTasks() const
{
    const int storedValue =
        settings_->value(kMaxConcurrentTasksKey, kDefaultConcurrentTasks).toInt();
    return std::clamp(storedValue, kMinimumConcurrentTasks, kMaximumConcurrentTasks);
}

void SettingsService::setMaxConcurrentTasks(int taskCount)
{
    settings_->setValue(
        kMaxConcurrentTasksKey,
        std::clamp(taskCount, kMinimumConcurrentTasks, kMaximumConcurrentTasks));
}

std::optional<AppError> SettingsService::statusError() const
{
    if (settings_->status() == QSettings::NoError) {
        return std::nullopt;
    }

    const QString code = settings_->status() == QSettings::AccessError
        ? QStringLiteral("settings.access_error")
        : QStringLiteral("settings.format_error");
    return AppError{
        .category = ErrorCategory::Filesystem,
        .severity = ErrorSeverity::Error,
        .code = code,
        .userMessage = QStringLiteral("The application settings could not be saved."),
        .technicalDetails = settings_->fileName(),
        .taskId = {},
        .recoveryActions = {},
    };
}

} // namespace utilityforge::core
