#include "signal_editor/adapters/qt/file_list_panel.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QVBoxLayout>

#include <algorithm>

namespace signal_editor::adapters::qt {

#define tr qt_file_list_panel_tr

namespace {
QString qt_file_list_panel_tr(const char* source_text,
                              const char* disambiguation = nullptr,
                              int n = -1) {
    return QCoreApplication::translate("FileListPanel", source_text,
                                       disambiguation, n);
}

QString format_label(const FileListPanel::FileItem& item) {
    return item.dirty ? QStringLiteral("%1 *").arg(item.display_name) : item.display_name;
}
}  // namespace

FileListPanel::FileListPanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("PanelCard"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    title_label_ = new QLabel(this);
    title_label_->setObjectName(QStringLiteral("PanelTitle"));
    title_label_->setProperty("uiFontRole", QStringLiteral("panel-title"));
    root->addWidget(title_label_);

    summary_label_ = new QLabel(this);
    summary_label_->setObjectName(QStringLiteral("PanelSummary"));
    summary_label_->setWordWrap(true);
    root->addWidget(summary_label_);

    detail_label_ = new QLabel(this);
    detail_label_->setObjectName(QStringLiteral("PanelDetail"));
    detail_label_->setWordWrap(true);
    root->addWidget(detail_label_);

    list_ = new QListWidget(this);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setEditTriggers(QAbstractItemView::DoubleClicked |
                           QAbstractItemView::EditKeyPressed);
    list_->setAlternatingRowColors(true);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(list_, 1);

    connect(list_, &QListWidget::currentRowChanged,
            this, &FileListPanel::onCurrentRowChanged);
    connect(list_, &QListWidget::itemChanged,
            this, &FileListPanel::onItemChanged);
    connect(list_, &QListWidget::customContextMenuRequested,
            this, &FileListPanel::onCustomContextMenuRequested);

    refresh_typography();
    retranslate_ui();
    refresh_summary(-1);
}

void FileListPanel::set_files(const std::vector<FileItem>& files) {
    files_ = files;
    const int previous = list_->currentRow();
    suppress_item_changed_ = true;
    list_->clear();
    for (const auto& file : files_) {
        auto* item = new QListWidgetItem(format_label(file), list_);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setToolTip(file.full_path);
    }

    if (previous >= 0 && previous < list_->count()) {
        list_->setCurrentRow(previous);
    } else if (list_->count() > 0) {
        list_->setCurrentRow(0);
    } else {
        refresh_summary(-1);
    }
    suppress_item_changed_ = false;
}

void FileListPanel::select(int index) {
    if (index >= 0 && index < list_->count()) {
        list_->setCurrentRow(index);
    }
}

int FileListPanel::current_index() const {
    return list_->currentRow();
}

void FileListPanel::begin_rename_current() {
    auto* item = list_->currentItem();
    if (item != nullptr) {
        list_->editItem(item);
    }
}

void FileListPanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslate_ui();
        refresh_summary(list_ != nullptr ? list_->currentRow() : -1);
    } else if (event->type() == QEvent::FontChange ||
               event->type() == QEvent::ApplicationFontChange) {
        refresh_typography();
    }
    QWidget::changeEvent(event);
}

void FileListPanel::onCurrentRowChanged(int row) {
    refresh_summary(row);
    emit selectionChanged(row);
}

void FileListPanel::onItemChanged(QListWidgetItem* item) {
    if (suppress_item_changed_ || item == nullptr) {
        return;
    }
    emit renameRequested(list_->row(item), item->text().trimmed());
}

void FileListPanel::onCustomContextMenuRequested(const QPoint& pos) {
    auto* item = list_->itemAt(pos);
    if (item == nullptr) {
        return;
    }

    const int row = list_->row(item);
    list_->setCurrentRow(row);

    QMenu menu(this);
    QAction* rename_action = menu.addAction(tr("Rename"));
    QAction* details_action = menu.addAction(tr("Details"));
    QAction* remove_action = menu.addAction(tr("Remove file from workspace"));
    QAction* chosen = menu.exec(list_->viewport()->mapToGlobal(pos));
    if (chosen == rename_action) {
        begin_rename_current();
    } else if (chosen == details_action) {
        emit detailsRequested(row);
    } else if (chosen == remove_action) {
        emit removeRequested(row);
    }
}

void FileListPanel::retranslate_ui() {
    if (title_label_ != nullptr) {
        title_label_->setText(tr("Files"));
        title_label_->setToolTip(tr("Lists every file currently loaded in the workspace."));
    }
    if (summary_label_ != nullptr) {
        summary_label_->setToolTip(tr("Quick summary of loaded files and pending modifications."));
    }
    if (detail_label_ != nullptr) {
        detail_label_->setToolTip(tr("Shows details for the currently selected workspace file."));
    }
    if (list_ != nullptr) {
        list_->setToolTip(tr("Select a file to make it active. Double-click a name to rename it."));
    }
}

void FileListPanel::refresh_typography() {
    if (title_label_ == nullptr) {
        return;
    }
    QFont title_font = font();
    title_font.setPointSizeF(title_font.pointSizeF() + 1.5);
    title_font.setBold(true);
    title_label_->setFont(title_font);
}

void FileListPanel::refresh_summary(int current_row) {
    const int dirty_count = static_cast<int>(std::count_if(
        files_.begin(),
        files_.end(),
        [](const FileItem& item) { return item.dirty; }));

    if (files_.empty()) {
        summary_label_->setText(tr("No CSV loaded yet"));
        detail_label_->setText(tr("Open files or drop them into the workspace."));
        return;
    }

    summary_label_->setText(
        tr("%1 file(s) in workspace, %2 modified")
            .arg(files_.size())
            .arg(dirty_count));

    if (current_row >= 0 && current_row < static_cast<int>(files_.size())) {
        const auto& item = files_[static_cast<std::size_t>(current_row)];
        detail_label_->setText(
            tr("Active file: %1\n%2")
                .arg(item.display_name, item.full_path));
    } else {
        detail_label_->setText(tr("Select a file to inspect its source path and workspace status."));
    }
}

#undef tr

}  // namespace signal_editor::adapters::qt
