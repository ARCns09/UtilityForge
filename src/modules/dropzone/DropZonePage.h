#pragma once

#include <QWidget>

class QLabel;
class QStandardItemModel;
class QTableView;
class QToolBar;

namespace utilityforge::core {
class TaskManager;
}

namespace utilityforge::dropzone {

class DropZonePage final : public QWidget
{
    Q_OBJECT

public:
    explicit DropZonePage(core::TaskManager *taskManager, QWidget *parent = nullptr);

private:
    void updateTaskStatus(int activeTasks, int queuedTasks);

    core::TaskManager *taskManager_;
    QToolBar *toolBar_;
    QTableView *queueView_;
    QStandardItemModel *queueModel_;
    QLabel *emptyStateLabel_;
    QLabel *taskStatusLabel_;
};

} // namespace utilityforge::dropzone
