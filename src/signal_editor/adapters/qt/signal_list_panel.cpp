#include "signal_editor/adapters/qt/signal_list_panel.h"

#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <QAbstractItemView>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace myprj::signal_editor::adapters::qt {

SignalListPanel::SignalListPanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("PanelCard"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* title = new QLabel(QStringLiteral("Signals"), this);
    title->setObjectName(QStringLiteral("PanelTitle"));
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() + 1.5);
    tf.setBold(true);
    title->setFont(tf);
    root->addWidget(title);

    summary_label_ = new QLabel(QStringLiteral("No active signal library"), this);
    summary_label_->setObjectName(QStringLiteral("PanelSummary"));
    summary_label_->setWordWrap(true);
    root->addWidget(summary_label_);

    detail_label_ = new QLabel(QStringLiteral("Load a workspace file to inspect and rename signals."), this);
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
    add_button_ = new QPushButton(QStringLiteral("+ New"), this);
    remove_button_ = new QPushButton(QStringLiteral("- Remove"), this);
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
            item->setToolTip(QStringLiteral("%1 samples").arg(static_cast<qulonglong>(s.size())));
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

void SignalListPanel::refresh_summary() {
    if (library_ == nullptr) {
        summary_label_->setText(QStringLiteral("No active signal library"));
        detail_label_->setText(QStringLiteral("Load a workspace file to inspect and rename signals."));
        remove_button_->setEnabled(false);
        return;
    }

    summary_label_->setText(QStringLiteral("%1 signal(s) ready for editing").arg(library_->size()));

    const int row = list_->currentRow();
    if (row >= 0 && row < static_cast<int>(library_->size())) {
        const auto& signal = library_->at(static_cast<std::size_t>(row));
        const QString interpolation = signal.interpolation() == Signal::InterpolationMode::Step
            ? QStringLiteral("step")
            : QStringLiteral("linear");
        detail_label_->setText(
            QStringLiteral("Selected: %1\n%2 samples, %3 interpolation")
                .arg(QString::fromStdString(signal.name()))
                .arg(signal.size())
                .arg(interpolation));
        remove_button_->setEnabled(true);
        return;
    }

    detail_label_->setText(QStringLiteral("Select a signal to edit its shape, interpolation, and samples."));
    remove_button_->setEnabled(false);
}

}  // namespace myprj::signal_editor::adapters::qt
