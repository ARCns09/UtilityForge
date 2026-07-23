#pragma once

#include "core/tasks/CancellationToken.h"
#include "core/tasks/TaskRecord.h"

#include <QHash>
#include <QList>
#include <QObject>

#include <optional>

namespace utilityforge::core {

class TaskManager final : public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(int maxConcurrentTasks, QObject *parent = nullptr);

    [[nodiscard]] TaskId createTask(const QString &label);
    [[nodiscard]] bool transitionTask(const TaskId &taskId, TaskState nextState);
    [[nodiscard]] bool updateProgress(const TaskId &taskId, const TaskProgress &progress);
    [[nodiscard]] bool failTask(const TaskId &taskId, const AppError &error);
    [[nodiscard]] bool requestCancellation(const TaskId &taskId);
    void cancelAll();

    [[nodiscard]] std::optional<TaskRecord> task(const TaskId &taskId) const;
    [[nodiscard]] QList<TaskRecord> tasks() const;
    [[nodiscard]] CancellationToken cancellationToken(const TaskId &taskId) const;

    [[nodiscard]] int activeTaskCount() const;
    [[nodiscard]] int queuedTaskCount() const;
    [[nodiscard]] int runningTaskCount() const;
    [[nodiscard]] int maxConcurrentTasks() const;
    void setMaxConcurrentTasks(int maxConcurrentTasks);

signals:
    void taskAdded(const utilityforge::core::TaskRecord &task);
    void taskUpdated(const utilityforge::core::TaskRecord &task);
    void taskCancellationRequested(const utilityforge::core::TaskId &taskId);
    void countsChanged(int activeTasks, int queuedTasks);

private:
    struct TaskEntry {
        TaskRecord record;
        CancellationSource cancellationSource;
    };

    [[nodiscard]] bool isOnOwningThread() const;
    [[nodiscard]] bool isTransitionAllowed(TaskState current, TaskState next) const;
    void emitTaskChange(const TaskRecord &record);

    QHash<TaskId, TaskEntry> tasks_;
    int maxConcurrentTasks_;
};

} // namespace utilityforge::core
