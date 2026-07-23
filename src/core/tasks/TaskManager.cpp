#include "core/tasks/TaskManager.h"

#include <QThread>

#include <algorithm>
#include <utility>

namespace utilityforge::core {
namespace {

constexpr int kMinimumConcurrentTasks = 1;
constexpr int kMaximumConcurrentTasks = 4;

bool consumesConcurrencySlot(TaskState state)
{
    return state == TaskState::Preparing || state == TaskState::Running
        || state == TaskState::Publishing || state == TaskState::Cancelling;
}

} // namespace

TaskManager::TaskManager(int maxConcurrentTasks, QObject *parent)
    : QObject(parent)
    , maxConcurrentTasks_(
          std::clamp(maxConcurrentTasks, kMinimumConcurrentTasks, kMaximumConcurrentTasks))
{
    qRegisterMetaType<TaskRecord>();
    qRegisterMetaType<TaskState>();
}

TaskId TaskManager::createTask(const QString &label)
{
    if (!isOnOwningThread() || label.trimmed().isEmpty()) {
        return {};
    }

    TaskRecord record{
        .id = QUuid::createUuid(),
        .label = label.trimmed(),
        .state = TaskState::Queued,
        .progress = {},
        .createdAt = QDateTime::currentDateTimeUtc(),
        .error = std::nullopt,
    };
    tasks_.insert(record.id, TaskEntry{.record = record, .cancellationSource = {}});
    emit taskAdded(record);
    emit countsChanged(activeTaskCount(), queuedTaskCount());
    return record.id;
}

bool TaskManager::transitionTask(const TaskId &taskId, TaskState nextState)
{
    if (!isOnOwningThread()) {
        return false;
    }

    auto taskIterator = tasks_.find(taskId);
    if (taskIterator == tasks_.end()
        || nextState == TaskState::Failed
        || !isTransitionAllowed(taskIterator->record.state, nextState)) {
        return false;
    }

    if (nextState == TaskState::Preparing
        && runningTaskCount() >= maxConcurrentTasks_) {
        return false;
    }

    if (taskIterator->record.state == TaskState::Cancelling
        && nextState == TaskState::Succeeded) {
        return false;
    }

    taskIterator->record.state = nextState;
    if (isTerminalTaskState(nextState)) {
        taskIterator->record.progress.percentage =
            nextState == TaskState::Succeeded ? 100 : taskIterator->record.progress.percentage;
    }
    emitTaskChange(taskIterator->record);
    return true;
}

bool TaskManager::updateProgress(const TaskId &taskId, const TaskProgress &progress)
{
    if (!isOnOwningThread() || progress.percentage < -1 || progress.percentage > 100) {
        return false;
    }

    auto taskIterator = tasks_.find(taskId);
    if (taskIterator == tasks_.end()
        || !consumesConcurrencySlot(taskIterator->record.state)) {
        return false;
    }

    taskIterator->record.progress = progress;
    emit taskUpdated(taskIterator->record);
    return true;
}

bool TaskManager::failTask(const TaskId &taskId, const AppError &error)
{
    if (!isOnOwningThread() || !error.isValid()) {
        return false;
    }

    auto taskIterator = tasks_.find(taskId);
    if (taskIterator == tasks_.end()
        || !isTransitionAllowed(taskIterator->record.state, TaskState::Failed)) {
        return false;
    }

    AppError taskError = error;
    if (taskError.taskId.isEmpty()) {
        taskError.taskId = taskId.toString(QUuid::WithoutBraces);
    }
    taskIterator->record.error = std::move(taskError);
    taskIterator->record.state = TaskState::Failed;
    emitTaskChange(taskIterator->record);
    return true;
}

bool TaskManager::requestCancellation(const TaskId &taskId)
{
    if (!isOnOwningThread()) {
        return false;
    }

    auto taskIterator = tasks_.find(taskId);
    if (taskIterator == tasks_.end() || isTerminalTaskState(taskIterator->record.state)
        || taskIterator->record.state == TaskState::Cancelling) {
        return false;
    }

    if (!taskIterator->cancellationSource.requestCancellation()) {
        return false;
    }

    if (taskIterator->record.state == TaskState::Queued) {
        taskIterator->record.state = TaskState::Cancelled;
    } else {
        taskIterator->record.state = TaskState::Cancelling;
    }

    emit taskCancellationRequested(taskId);
    emitTaskChange(taskIterator->record);
    return true;
}

void TaskManager::cancelAll()
{
    if (!isOnOwningThread()) {
        return;
    }

    const QList<TaskId> taskIds = tasks_.keys();
    for (const TaskId &taskId : taskIds) {
        static_cast<void>(requestCancellation(taskId));
    }
}

std::optional<TaskRecord> TaskManager::task(const TaskId &taskId) const
{
    const auto taskIterator = tasks_.constFind(taskId);
    if (taskIterator == tasks_.cend()) {
        return std::nullopt;
    }
    return taskIterator->record;
}

QList<TaskRecord> TaskManager::tasks() const
{
    QList<TaskRecord> records;
    records.reserve(tasks_.size());
    for (const TaskEntry &entry : tasks_) {
        records.append(entry.record);
    }
    return records;
}

CancellationToken TaskManager::cancellationToken(const TaskId &taskId) const
{
    const auto taskIterator = tasks_.constFind(taskId);
    if (taskIterator == tasks_.cend()) {
        return {};
    }
    return taskIterator->cancellationSource.token();
}

int TaskManager::activeTaskCount() const
{
    int count = 0;
    for (const TaskEntry &entry : tasks_) {
        if (!isTerminalTaskState(entry.record.state)) {
            ++count;
        }
    }
    return count;
}

int TaskManager::queuedTaskCount() const
{
    int count = 0;
    for (const TaskEntry &entry : tasks_) {
        if (entry.record.state == TaskState::Queued) {
            ++count;
        }
    }
    return count;
}

int TaskManager::runningTaskCount() const
{
    int count = 0;
    for (const TaskEntry &entry : tasks_) {
        if (consumesConcurrencySlot(entry.record.state)) {
            ++count;
        }
    }
    return count;
}

int TaskManager::maxConcurrentTasks() const
{
    return maxConcurrentTasks_;
}

void TaskManager::setMaxConcurrentTasks(int maxConcurrentTasks)
{
    if (!isOnOwningThread()) {
        return;
    }

    maxConcurrentTasks_ =
        std::clamp(maxConcurrentTasks, kMinimumConcurrentTasks, kMaximumConcurrentTasks);
}

bool TaskManager::isOnOwningThread() const
{
    return thread() == QThread::currentThread();
}

bool TaskManager::isTransitionAllowed(TaskState current, TaskState next) const
{
    switch (current) {
    case TaskState::Queued:
        return next == TaskState::Preparing || next == TaskState::Cancelled;
    case TaskState::Preparing:
        return next == TaskState::Running || next == TaskState::Cancelling
            || next == TaskState::Failed;
    case TaskState::Running:
        return next == TaskState::Publishing || next == TaskState::Cancelling
            || next == TaskState::Failed;
    case TaskState::Publishing:
        return next == TaskState::Succeeded || next == TaskState::Cancelling
            || next == TaskState::Failed;
    case TaskState::Cancelling:
        return next == TaskState::Cancelled || next == TaskState::Failed;
    case TaskState::Succeeded:
    case TaskState::Failed:
    case TaskState::Cancelled:
        return false;
    }

    return false;
}

void TaskManager::emitTaskChange(const TaskRecord &record)
{
    emit taskUpdated(record);
    emit countsChanged(activeTaskCount(), queuedTaskCount());
}

} // namespace utilityforge::core
