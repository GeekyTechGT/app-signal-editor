#include "signal_editor/adapters/qt/signal_list_panel.h"

#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace signal_editor::adapters::qt {

#define tr qt_signal_list_panel_tr

namespace {
QString qt_signal_list_panel_tr(const char* source_text,
                                const char* disambiguation = nullptr,
                                int n = -1) {
    return QCoreApplication::translate("SignalListPanel", source_text,
                                       disambiguation, n);
}
}  // namespace

SignalListPanel::SignalListPanel(QWidget* parent) : QWidget(parent) {
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
    root->addWidget(list_, 1);

    auto* buttons = new QHBoxLayout();
    add_button_ = new QPushButton(this);
    remove_button_ = new QPushButton(this);
    add_button_->setCursor(::Qt::PointingHandCursor);
    remove_button_->setCursor(::Qt::PointingHandCursor);
    buttons->addWidget(add_button_);
    buttons->addWidget(remove_button_);
    root->addLayout(buttons);

    connect(list_, &QListWidget::currentRowChanged,
            this, &SignalListPanel::onCurrentRowChanged);
    connect(list_, &QListWidget::itemChanged,
            this, &SignalListPanel::onItemChanged);
    connect(add_button_, &QPushButton::clicked,
            this, &SignalListPanel::onAddClicked);
    connect(remove_button_, &QPushButton::clicked,
            this, &SignalListPanel::onRemoveClicked);

    refresh_typography();
    retranslate_ui();
    refresh_summary();
}

void SignalListPanel::set_library(const SignalLibrary* library) {
    library_ = library;
    refresh();
}

void SignalListPanel::refresh() {
    suppress_item_changed_ = true;
    const int previous = list_->currentRow();
    list_->clear();
    if (library_ != nullptr) {
        for (const auto& s : library_->items()) {
            auto* item = new QListWidgetItem(QString::fromStdString(s.name()), list_);
            item->setFlags(item->flags() | ::Qt::ItemIsEditable);
            item->setToolTip(
                tr("%1 samples. Double-click to rename; select to edit in plot and table.")
                    .arg(static_cast<qulonglong>(s.size())));
        }
    }
    if (previous >= 0 && previous < list_->count()) {
        list_->setCurrentRow(previous);
    } else if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
    suppress_item_changed_ = false;
    refresh_summary();
}

void SignalListPanel::select(int index) {
    if (index >= 0 && index < list_->count()) {
        list_->setCurrentRow(index);
    }
}

void SignalListPanel::begin_rename_current() {
    auto* item = list_->currentItem();
    if (item != nullptr) {
        list_->editItem(item);
    }
}

int SignalListPanel::current_index() const {
    return list_->currentRow();
}

void SignalListPanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslate_ui();
        refresh_summary();
    } else if (event->type() == QEvent::FontChange ||
               event->type() == QEvent::ApplicationFontChange) {
        refresh_typography();
    }
    QWidget::changeEvent(event);
}

void SignalListPanel::onCurrentRowChanged(int row) {
    Q_UNUSED(row);
    refresh_summary();
    emit selectionChanged(row);
}

void SignalListPanel::onItemChanged(QListWidgetItem* item) {
    if (suppress_item_changed_ || item == nullptr) {
        return;
    }
    const int row = list_->row(item);
    emit renameRequested(row, item->text());
}

void SignalListPanel::onAddClicked() {
    emit addRequested();
}

void SignalListPanel::onRemoveClicked() {
    emit removeRequested(list_->currentRow());
}

void SignalListPanel::retranslate_ui() {
    if (title_label_ != nullptr) {
        title_label_->setText(tr("Signals"));
        title_label_->setToolTip(tr("Signals available in the active workspace file."));
    }
    if (summary_label_ != nullptr) {
        summary_label_->setToolTip(tr("Summary of the currently loaded signal library."));
    }
    if (detail_label_ != nullptr) {
        detail_label_->setToolTip(tr("Details about the selected signal and its interpolation mode."));
    }
    if (list_ != nullptr) {
        list_->setToolTip(tr("Select a signal to edit it. Double-click a name to rename it."));
    }
    if (add_button_ != nullptr) {
        add_button_->setText(tr("+ New"));
        add_button_->setToolTip(tr("Create a new signal in the active workspace."));
        add_button_->setStatusTip(tr("Create a new signal."));
    }
    if (remove_button_ != nullptr) {
        remove_button_->setText(tr("- Remove"));
        remove_button_->setToolTip(tr("Remove the selected signal from the active workspace."));
        remove_button_->setStatusTip(tr("Remove the selected signal."));
    }
}

void SignalListPanel::refresh_typography() {
    if (title_label_ == nullptr) {
        return;
    }
    QFont title_font = font();
    title_font.setPointSizeF(title_font.pointSizeF() + 1.5);
    title_font.setBold(true);
    title_label_->setFont(title_font);
}

void SignalListPanel::refresh_summary() {
    if (library_ == nullptr) {
        summary_label_->setText(tr("No active signal library"));
        detail_label_->setText(tr("Load a workspace file to inspect and rename signals."));
        remove_button_->setEnabled(false);
        return;
    }

    summary_label_->setText(tr("%1 signal(s) ready for editing").arg(library_->size()));

    const int row = list_->currentRow();
    if (row >= 0 && row < static_cast<int>(library_->size())) {
        const auto& signal = library_->at(static_cast<std::size_t>(row));
        const QString interpolation = signal.interpolation() == Signal::InterpolationMode::Step
            ? tr("step")
            : tr("linear");
        detail_label_->setText(
            tr("Selected: %1\n%2 samples, %3 interpolation")
                .arg(QString::fromStdString(signal.name()))
                .arg(signal.size())
                .arg(interpolation));
        remove_button_->setEnabled(true);
        return;
    }

    detail_label_->setText(tr("Select a signal to edit its shape, interpolation, and samples."));
    remove_button_->setEnabled(false);
}

#undef tr

}  // namespace signal_editor::adapters::qt
