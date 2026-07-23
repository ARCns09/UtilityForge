#pragma once

#include "core/errors/AppError.h"

#include <QString>

#include <optional>

namespace utilityforge::core {

class AppPaths final
{
public:
    [[nodiscard]] static AppPaths forApplication(const QString &applicationId);
    [[nodiscard]] static AppPaths forTestRoot(const QString &rootPath,
                                              const QString &applicationId);

    [[nodiscard]] const QString &applicationId() const;
    [[nodiscard]] const QString &configDirectory() const;
    [[nodiscard]] const QString &dataDirectory() const;
    [[nodiscard]] const QString &cacheDirectory() const;
    [[nodiscard]] const QString &stateDirectory() const;
    [[nodiscard]] const QString &runtimeDirectory() const;
    [[nodiscard]] QString settingsFilePath() const;

    [[nodiscard]] std::optional<AppError> ensureDirectories() const;

private:
    AppPaths(QString applicationId,
             QString configDirectory,
             QString dataDirectory,
             QString cacheDirectory,
             QString stateDirectory,
             QString runtimeDirectory);

    QString applicationId_;
    QString configDirectory_;
    QString dataDirectory_;
    QString cacheDirectory_;
    QString stateDirectory_;
    QString runtimeDirectory_;
};

} // namespace utilityforge::core
