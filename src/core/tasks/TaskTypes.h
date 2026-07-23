#pragma once

#include <QMetaType>
#include <QString>
#include <QUuid>

namespace utilityforge::core {

using TaskId = QUuid;

enum class TaskState {
    Queued,
    Preparing,
    Running,
    Publishing,
    Cancelling,
    Succeeded,
    Failed,
    Cancelled
};

struct TaskProgress {
    int percentage{-1};
    QString phase;
};

[[nodiscard]] inline bool isTerminalTaskState(TaskState state)
{
    return state == TaskState::Succeeded || state == TaskState::Failed
        || state == TaskState::Cancelled;
}

[[nodiscard]] inline QString taskStateDisplayName(TaskState state)
{
    switch (state) {
    case TaskState::Queued:
        return QStringLiteral("Queued");
    case TaskState::Preparing:
        return QStringLiteral("Preparing");
    case TaskState::Running:
        return QStringLiteral("Running");
    case TaskState::Publishing:
        return QStringLiteral("Publishing");
    case TaskState::Cancelling:
        return QStringLiteral("Cancelling");
    case TaskState::Succeeded:
        return QStringLiteral("Succeeded");
    case TaskState::Failed:
        return QStringLiteral("Failed");
    case TaskState::Cancelled:
        return QStringLiteral("Cancelled");
    }

    return QStringLiteral("Unknown");
}

} // namespace utilityforge::core

Q_DECLARE_METATYPE(utilityforge::core::TaskProgress)
Q_DECLARE_METATYPE(utilityforge::core::TaskState)
