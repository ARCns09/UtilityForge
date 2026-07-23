#include "core/settings/AppPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <array>
#include <utility>

namespace utilityforge::core {
namespace {

QString stateBaseDirectory()
{
    const QString configuredState = qEnvironmentVariable("XDG_STATE_HOME");
    if (QDir::isAbsolutePath(configuredState)) {
        return QDir::cleanPath(configuredState);
    }

    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(home).filePath(QStringLiteral(".local/state"));
}

QString childPath(const QString &parent, const QString &child)
{
    return QDir::cleanPath(QDir(parent).filePath(child));
}

QString applicationRuntimeDirectory(const QString &applicationId)
{
    const QString runtimeBase =
        QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (QDir::isAbsolutePath(runtimeBase)) {
        return childPath(runtimeBase, applicationId);
    }

    const QString cacheBase =
        QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    return childPath(childPath(cacheBase, applicationId), QStringLiteral("runtime"));
}

} // namespace

AppPaths AppPaths::forApplication(const QString &applicationId)
{
    return {
        applicationId,
        childPath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
                  applicationId),
        childPath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                  applicationId),
        childPath(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation),
                  applicationId),
        childPath(stateBaseDirectory(), applicationId),
        applicationRuntimeDirectory(applicationId),
    };
}

AppPaths AppPaths::forTestRoot(const QString &rootPath, const QString &applicationId)
{
    const QString cleanedRoot = QDir::cleanPath(rootPath);
    return {
        applicationId,
        childPath(cleanedRoot, QStringLiteral("config/") + applicationId),
        childPath(cleanedRoot, QStringLiteral("data/") + applicationId),
        childPath(cleanedRoot, QStringLiteral("cache/") + applicationId),
        childPath(cleanedRoot, QStringLiteral("state/") + applicationId),
        childPath(cleanedRoot, QStringLiteral("runtime/") + applicationId),
    };
}

AppPaths::AppPaths(QString applicationId,
                   QString configDirectory,
                   QString dataDirectory,
                   QString cacheDirectory,
                   QString stateDirectory,
                   QString runtimeDirectory)
    : applicationId_(std::move(applicationId))
    , configDirectory_(std::move(configDirectory))
    , dataDirectory_(std::move(dataDirectory))
    , cacheDirectory_(std::move(cacheDirectory))
    , stateDirectory_(std::move(stateDirectory))
    , runtimeDirectory_(std::move(runtimeDirectory))
{
}

const QString &AppPaths::applicationId() const
{
    return applicationId_;
}

const QString &AppPaths::configDirectory() const
{
    return configDirectory_;
}

const QString &AppPaths::dataDirectory() const
{
    return dataDirectory_;
}

const QString &AppPaths::cacheDirectory() const
{
    return cacheDirectory_;
}

const QString &AppPaths::stateDirectory() const
{
    return stateDirectory_;
}

const QString &AppPaths::runtimeDirectory() const
{
    return runtimeDirectory_;
}

QString AppPaths::settingsFilePath() const
{
    return QDir(configDirectory_).filePath(QStringLiteral("settings.ini"));
}

std::optional<AppError> AppPaths::ensureDirectories() const
{
    const std::array<QString, 5> directories = {
        configDirectory_,
        dataDirectory_,
        cacheDirectory_,
        stateDirectory_,
        runtimeDirectory_,
    };

    for (const QString &directory : directories) {
        if (directory.isEmpty() || !QDir::isAbsolutePath(directory)) {
            return AppError{
                .category = ErrorCategory::Filesystem,
                .severity = ErrorSeverity::Critical,
                .code = QStringLiteral("paths.invalid_directory"),
                .userMessage = QStringLiteral("An application storage path is invalid."),
                .technicalDetails = directory,
                .taskId = {},
                .recoveryActions = {},
            };
        }

        QDir dir;
        if (!dir.mkpath(directory)) {
            return AppError{
                .category = ErrorCategory::Filesystem,
                .severity = ErrorSeverity::Critical,
                .code = QStringLiteral("paths.create_failed"),
                .userMessage =
                    QStringLiteral("An application storage directory could not be created."),
                .technicalDetails = directory,
                .taskId = {},
                .recoveryActions = {},
            };
        }

        const QFileInfo info(directory);
        if (!info.isDir() || !info.isWritable()) {
            return AppError{
                .category = ErrorCategory::Filesystem,
                .severity = ErrorSeverity::Critical,
                .code = QStringLiteral("paths.not_writable"),
                .userMessage = QStringLiteral("An application storage directory is not writable."),
                .technicalDetails = directory,
                .taskId = {},
                .recoveryActions = {},
            };
        }
    }

    return std::nullopt;
}

} // namespace utilityforge::core
