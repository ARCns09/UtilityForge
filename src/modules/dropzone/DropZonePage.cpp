#include "modules/dropzone/DropZonePage.h"

#include "core/tasks/TaskManager.h"

#include <QAction>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QSize>
#include <QStandardItemModel>
#include <QStyle>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

namespace utilityforge::dropzone {

DropZonePage::DropZonePage(core::TaskManager *taskManager, QWidget *parent)
    : QWidget(parent)
    , taskManager_(taskManager)
    , toolBar_(new QToolBar(this))
    , queueView_(new QTableView(this))
    , queueModel_(new QStandardItemModel(0, 6, this))
    , emptyStateLabel_(new QLabel(this))
    , taskStatusLabel_(new QLabel(this))
{
    Q_ASSERT(taskManager_);
    setObjectName(QStringLiteral("dropZonePage"));
    setAcceptDrops(false);

    toolBar_->setObjectName(QStringLiteral("dropZoneToolBar"));
    toolBar_->setMovable(false);
    toolBar_->setFloatable(false);
    toolBar_->setIconSize(QSize(16, 16));
    toolBar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction *const addFilesAction = toolBar_->addAction(
        QIcon::fromTheme(
            QStringLiteral("document-open"),
            style()->standardIcon(QStyle::SP_DialogOpenButton)),
        tr("Add Files"));
    addFilesAction->setObjectName(QStringLiteral("addFilesAction"));
    addFilesAction->setEnabled(false);
    addFilesAction->setToolTip(tr("File intake is planned for Phase 2."));

    QAction *const addFolderAction = toolBar_->addAction(
        QIcon::fromTheme(
            QStringLiteral("folder-open"),
            style()->standardIcon(QStyle::SP_DirOpenIcon)),
        tr("Add Folder"));
    addFolderAction->setObjectName(QStringLiteral("addFolderAction"));
    addFolderAction->setEnabled(false);
    addFolderAction->setToolTip(tr("Folder intake is planned for Phase 2."));

    toolBar_->addSeparator();
    QAction *const cancelAction = toolBar_->addAction(
        QIcon::fromTheme(
            QStringLiteral("process-stop"),
            style()->standardIcon(QStyle::SP_DialogCancelButton)),
        tr("Cancel Selected"));
    cancelAction->setObjectName(QStringLiteral("cancelSelectedAction"));
    cancelAction->setEnabled(false);
    cancelAction->setToolTip(tr("There is no queued work to cancel."));

    queueModel_->setHorizontalHeaderLabels({
        tr("Input"),
        tr("Operation"),
        tr("State"),
        tr("Progress"),
        tr("Output"),
        tr("Details"),
    });
    queueView_->setObjectName(QStringLiteral("dropQueueView"));
    queueView_->setModel(queueModel_);
    queueView_->setAlternatingRowColors(true);
    queueView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queueView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    queueView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    queueView_->setSortingEnabled(false);
    queueView_->verticalHeader()->setVisible(false);
    queueView_->horizontalHeader()->setStretchLastSection(true);
    queueView_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    queueView_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    emptyStateLabel_->setObjectName(QStringLiteral("dropZoneEmptyState"));
    emptyStateLabel_->setText(
        tr("No files are queued.\n"
           "File intake and processing are not enabled in the Phase 1 shell."));
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setTextFormat(Qt::PlainText);
    emptyStateLabel_->setWordWrap(true);
    emptyStateLabel_->setMinimumHeight(96);

    taskStatusLabel_->setObjectName(QStringLiteral("dropZoneTaskStatus"));
    taskStatusLabel_->setTextFormat(Qt::PlainText);

    auto *const layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);
    layout->addWidget(toolBar_);
    layout->addWidget(emptyStateLabel_);
    layout->addWidget(queueView_, 1);
    layout->addWidget(taskStatusLabel_);

    connect(taskManager_,
            &core::TaskManager::countsChanged,
            this,
            &DropZonePage::updateTaskStatus);
    updateTaskStatus(taskManager_->activeTaskCount(), taskManager_->queuedTaskCount());
}

void DropZonePage::updateTaskStatus(int activeTasks, int queuedTasks)
{
    taskStatusLabel_->setText(
        tr("%1 active · %2 queued").arg(activeTasks).arg(queuedTasks));
}

} // namespace utilityforge::dropzone
