#pragma once

#include "core/errors/AppError.h"
#include "core/tasks/TaskTypes.h"

#include <QDateTime>
#include <QMetaType>

#include <optional>

namespace utilityforge::core {

struct TaskRecord {
    TaskId id;
    QString label;
    TaskState state{TaskState::Queued};
    TaskProgress progress;
    QDateTime createdAt;
    std::optional<AppError> error;
};

} // namespace utilityforge::core

Q_DECLARE_METATYPE(utilityforge::core::TaskRecord)
