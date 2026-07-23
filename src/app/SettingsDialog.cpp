#include "app/SettingsDialog.h"

#include "core/process/ExternalToolRegistry.h"
#include "core/settings/AppPaths.h"
#include "core/settings/SettingsService.h"
#include "core/tasks/TaskManager.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace utilityforge::app {
namespace {

constexpr int kToolColumn = 0;
constexpr int kStatusColumn = 1;
constexpr int kVersionColumn = 2;

int rowForTool(core::ExternalTool tool)
{
    switch (tool) {
    case core::ExternalTool::Ffmpeg:
        return 0;
    case core::ExternalTool::Ffprobe:
        return 1;
    case core::ExternalTool::Bsdtar:
        return 2;
    }
    return -1;
}

QLabel *pathLabel(const QString &path, QWidget *parent)
{
    auto *const label = new QLabel(path, parent);
    label->setTextFormat(Qt::PlainText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setWordWrap(true);
    return label;
}

} // namespace

SettingsDialog::SettingsDialog(const core::AppPaths *paths,
                               core::SettingsService *settings,
                               core::TaskManager *taskManager,
                               core::ExternalToolRegistry *toolRegistry,
                               QWidget *parent)
    : QDialog(parent)
    , paths_(paths)
    , settings_(settings)
    , taskManager_(taskManager)
    , toolRegistry_(toolRegistry)
    , concurrencySpinBox_(new QSpinBox(this))
    , toolTable_(new QTableWidget(3, 3, this))
    , saveStatusLabel_(new QLabel(this))
{
    Q_ASSERT(paths_);
    Q_ASSERT(settings_);
    Q_ASSERT(taskManager_);
    Q_ASSERT(toolRegistry_);

    setObjectName(QStringLiteral("settingsDialog"));
    setWindowTitle(tr("Settings"));
    setModal(true);
    resize(600, 420);

    concurrencySpinBox_->setObjectName(QStringLiteral("concurrencySpinBox"));
    concurrencySpinBox_->setRange(1, 4);
    concurrencySpinBox_->setValue(settings_->maxConcurrentTasks());
    concurrencySpinBox_->setToolTip(tr("Maximum number of heavyweight tasks."));

    auto *const preferencesGroup = new QGroupBox(tr("Task preferences"), this);
    auto *const preferencesLayout = new QFormLayout(preferencesGroup);
    preferencesLayout->addRow(tr("Concurrent tasks:"), concurrencySpinBox_);

    auto *const pathsGroup = new QGroupBox(tr("XDG storage"), this);
    auto *const pathsLayout = new QFormLayout(pathsGroup);
    pathsLayout->addRow(tr("Configuration:"), pathLabel(paths_->configDirectory(), pathsGroup));
    pathsLayout->addRow(tr("Data:"), pathLabel(paths_->dataDirectory(), pathsGroup));
    pathsLayout->addRow(tr("Cache:"), pathLabel(paths_->cacheDirectory(), pathsGroup));
    pathsLayout->addRow(tr("State:"), pathLabel(paths_->stateDirectory(), pathsGroup));

    toolTable_->setObjectName(QStringLiteral("toolDiagnosticsTable"));
    toolTable_->setHorizontalHeaderLabels({tr("Tool"), tr("Status"), tr("Version")});
    toolTable_->verticalHeader()->setVisible(false);
    toolTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    toolTable_->setSelectionMode(QAbstractItemView::NoSelection);
    toolTable_->horizontalHeader()->setStretchLastSection(true);
    toolTable_->horizontalHeader()->setSectionResizeMode(
        kToolColumn, QHeaderView::ResizeToContents);
    toolTable_->horizontalHeader()->setSectionResizeMode(kStatusColumn,
                                                        QHeaderView::ResizeToContents);

    const QList<core::ExternalTool> tools = {
        core::ExternalTool::Ffmpeg,
        core::ExternalTool::Ffprobe,
        core::ExternalTool::Bsdtar,
    };
    for (const core::ExternalTool tool : tools) {
        const int row = rowForTool(tool);
        toolTable_->setItem(
            row, kToolColumn, new QTableWidgetItem(core::externalToolDisplayName(tool)));
        toolTable_->setItem(row, kStatusColumn, new QTableWidgetItem);
        toolTable_->setItem(row, kVersionColumn, new QTableWidgetItem);
        updateToolStatus(toolRegistry_->status(tool));
    }

    auto *const diagnosticsGroup = new QGroupBox(tr("External tool diagnostics"), this);
    auto *const diagnosticsLayout = new QVBoxLayout(diagnosticsGroup);
    diagnosticsLayout->addWidget(toolTable_);
    auto *const checkToolsButton = new QPushButton(tr("Check tools"), diagnosticsGroup);
    checkToolsButton->setObjectName(QStringLiteral("checkToolsButton"));
    diagnosticsLayout->addWidget(checkToolsButton, 0, Qt::AlignRight);

    saveStatusLabel_->setObjectName(QStringLiteral("settingsSaveStatus"));
    saveStatusLabel_->setTextFormat(Qt::PlainText);
    saveStatusLabel_->setWordWrap(true);

    auto *const buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto *const layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(preferencesGroup);
    layout->addWidget(pathsGroup);
    layout->addWidget(diagnosticsGroup, 1);
    layout->addWidget(saveStatusLabel_);
    layout->addWidget(buttonBox);

    connect(checkToolsButton,
            &QPushButton::clicked,
            toolRegistry_,
            &core::ExternalToolRegistry::refreshAll);
    connect(toolRegistry_,
            &core::ExternalToolRegistry::toolStatusChanged,
            this,
            &SettingsDialog::updateToolStatus);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::handleAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::handleAccepted()
{
    const int taskCount = concurrencySpinBox_->value();
    settings_->setMaxConcurrentTasks(taskCount);
    taskManager_->setMaxConcurrentTasks(taskCount);
    if (const auto error = settings_->sync(); error.has_value()) {
        saveStatusLabel_->setText(error->userMessage);
        return;
    }
    accept();
}

void SettingsDialog::updateToolStatus(const core::ToolStatus &status)
{
    const int row = rowForTool(status.tool);
    if (row < 0) {
        return;
    }

    QTableWidgetItem *const statusItem = toolTable_->item(row, kStatusColumn);
    QTableWidgetItem *const versionItem = toolTable_->item(row, kVersionColumn);
    statusItem->setText(core::toolAvailabilityDisplayName(status.availability));
    statusItem->setToolTip(status.executablePath);
    versionItem->setText(status.versionSummary);
}

} // namespace utilityforge::app
