#include "signal_editor/adapters/qt/file_list_panel.h"

#include <QAbstractItemView>
#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>

#include <algorithm>

namespace myprj::signal_editor::adapters::qt {

namespace {
QString format_label(const FileListPanel::FileItem& item) {
    return item.dirty ? QStringLiteral("%1 *").arg(item.display_name) : item.display_name;
}
}  // namespace

FileListPanel::FileListPanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("PanelCard"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* title = new QLabel(QStringLiteral("Files"), this);
    title->setObjectName(QStringLiteral("PanelTitle"));
    QFont font = title->font();
    font.setPointSizeF(font.pointSizeF() + 1.5);
    font.setBold(true);
    title->setFont(font);
    root->addWidget(title);

    summary_label_ = new QLabel(QStringLiteral("No CSV loaded yet"), this);
    summary_label_->setObjectName(QStringLiteral("PanelSummary"));
    summary_label_->setWordWrap(true);
    root->addWidget(summary_label_);

    detail_label_ = new QLabel(QStringLiteral("Open files or drop them into the workspace."), this);
    detail_label_->setObjectName(QStringLiteral("PanelDetail"));
    detail_label_->setWordWrap(true);
    root->addWidget(detail_label_);

    list_ = new QListWidget(this);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list_->setAlternatingRowColors(true);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(list_, 1);

    connect(list_, &QListWidget::currentRowChanged,
            this, &FileListPanel::onCurrentRowChanged);
    connect(list_, &QListWidget::customContextMenuRequested,
            this, &FileListPanel::onCustomContextMenuRequested);
}

void FileListPanel::set_files(const std::vector<FileItem>& files) {
    files_ = files;
    const int previous = list_->currentRow();
    list_->clear();
    for (const auto& file : files_) {
        auto* item = new QListWidgetItem(format_label(file), list_);
        item->setToolTip(file.full_path);
    }

    if (previous >= 0 && previous < list_->count()) {
        list_->setCurrentRow(previous);
    } else if (list_->count() > 0) {
        list_->setCurrentRow(0);
    } else {
        refresh_summary(-1);
    }
}

void FileListPanel::select(int index) {
    if (index >= 0 && index < list_->count()) {
        list_->setCurrentRow(index);
    }
}

int FileListPanel::current_index() const {
    return list_->currentRow();
}

void FileListPanel::onCurrentRowChanged(int row) {
    refresh_summary(row);
    emit selectionChanged(row);
}

void FileListPanel::onCustomContextMenuRequested(const QPoint& pos) {
    auto* item = list_->itemAt(pos);
    if (item == nullptr) {
        return;
    }

    const int row = list_->row(item);
    list_->setCurrentRow(row);

    QMenu menu(this);
    QAction* details_action = menu.addAction(QStringLiteral("Details"));
    QAction* remove_action = menu.addAction(QStringLiteral("Remove file from workspace"));
    QAction* chosen = menu.exec(list_->viewport()->mapToGlobal(pos));
    if (chosen == details_action) {
        emit detailsRequested(row);
    } else if (chosen == remove_action) {
        emit removeRequested(row);
    }
}

void FileListPanel::refresh_summary(int current_row) {
    const int dirty_count = static_cast<int>(std::count_if(
        files_.begin(),
        files_.end(),
        [](const FileItem& item) { return item.dirty; }));

    if (files_.empty()) {
        summary_label_->setText(QStringLiteral("No CSV loaded yet"));
        detail_label_->setText(QStringLiteral("Open files or drop them into the workspace."));
        return;
    }

    summary_label_->setText(
        QStringLiteral("%1 file(s) in workspace, %2 modified")
            .arg(files_.size())
            .arg(dirty_count));

    if (current_row >= 0 && current_row < static_cast<int>(files_.size())) {
        const auto& item = files_[static_cast<std::size_t>(current_row)];
        detail_label_->setText(
            QStringLiteral("Active file: %1\n%2")
                .arg(item.display_name, item.full_path));
    } else {
        detail_label_->setText(QStringLiteral("Select a file to inspect its source path and workspace status."));
    }
}

}  // namespace myprj::signal_editor::adapters::qt
