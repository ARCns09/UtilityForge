#pragma once

#include "core/errors/AppError.h"
#include "core/settings/AppPaths.h"

#include <QByteArray>
#include <QString>

#include <memory>
#include <optional>

class QSettings;

namespace utilityforge::core {

class SettingsService final
{
public:
    explicit SettingsService(const AppPaths &paths);
    ~SettingsService();

    SettingsService(const SettingsService &) = delete;
    SettingsService &operator=(const SettingsService &) = delete;
    SettingsService(SettingsService &&) = delete;
    SettingsService &operator=(SettingsService &&) = delete;

    [[nodiscard]] std::optional<AppError> initialize();
    [[nodiscard]] std::optional<AppError> sync();

    [[nodiscard]] QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);
    [[nodiscard]] QByteArray windowState() const;
    void setWindowState(const QByteArray &state);

    [[nodiscard]] QString lastOutputDirectory() const;
    void setLastOutputDirectory(const QString &directory);

    [[nodiscard]] int maxConcurrentTasks() const;
    void setMaxConcurrentTasks(int taskCount);

private:
    [[nodiscard]] std::optional<AppError> statusError() const;

    std::unique_ptr<QSettings> settings_;
};

} // namespace utilityforge::core
