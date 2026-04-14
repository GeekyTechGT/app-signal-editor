#include "signal_editor/adapters/qt/signal_table_panel.h"

#include "signal_editor/core/domain/signal.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDoubleValidator>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QMetaType>

#include <cmath>
#include <stdexcept>

namespace myprj::signal_editor::adapters::qt {

namespace {
void configure_table_editor(QWidget* editor) {
    editor->setAttribute(Qt::WA_StyledBackground, true);
    editor->setAutoFillBackground(true);
    editor->setFocusPolicy(Qt::StrongFocus);
}

class SampleItemDelegate : public QStyledItemDelegate {
public:
    explicit SampleItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void set_signal(const Signal* signal) noexcept { signal_ = signal; }

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem&,
                          const QModelIndex& index) const override {
        if (index.column() == 1 && signal_ != nullptr && signal_->is_enumerated()) {
            auto* editor = new QComboBox(parent);
            configure_table_editor(editor);
            for (const auto& entry : signal_->enumeration()) {
                editor->addItem(QString::fromStdString(entry.label));
            }
            editor->setEditable(false);
            editor->setFrame(false);
            editor->setMaxVisibleItems(12);
            return editor;
        }

        auto* editor = new QLineEdit(parent);
        configure_table_editor(editor);
        editor->setAlignment(Qt::AlignRight);
        editor->setFrame(false);
        auto* validator = new QDoubleValidator(-1e12, 1e12, 6, editor);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setLocale(editor->locale());
        editor->setValidator(validator);
        return editor;
    }

    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex&) const override {
        editor->setGeometry(option.rect.adjusted(3, 3, -3, -3));
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        if (index.column() == 1 && signal_ != nullptr && signal_->is_enumerated()) {
            auto* combo = qobject_cast<QComboBox*>(editor);
            if (combo == nullptr) {
                return;
            }
            const QString label = index.data(Qt::EditRole).toString();
            const int combo_index = combo->findText(label);
            combo->setCurrentIndex(combo_index >= 0 ? combo_index : 0);
            return;
        }

        auto* line_edit = qobject_cast<QLineEdit*>(editor);
        if (line_edit == nullptr) {
            return;
        }
        line_edit->setText(line_edit->locale().toString(index.data(Qt::EditRole).toDouble(), 'f', 6));
        line_edit->selectAll();
    }

    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        if (index.column() == 1 && signal_ != nullptr && signal_->is_enumerated()) {
            auto* combo = qobject_cast<QComboBox*>(editor);
            if (combo == nullptr) {
                return;
            }
            model->setData(index, combo->currentText(), Qt::EditRole);
            return;
        }

        auto* line_edit = qobject_cast<QLineEdit*>(editor);
        if (line_edit == nullptr) {
            return;
        }
        bool ok = false;
        const double value = line_edit->locale().toDouble(line_edit->text(), &ok);
        if (!ok) {
            return;
        }
        model->setData(index, value, Qt::EditRole);
    }

    QString displayText(const QVariant& value, const QLocale& locale) const override {
        if (value.canConvert<QString>() && !value.canConvert<double>()) {
            return value.toString();
        }
        if (value.typeId() == QMetaType::QString) {
            return value.toString();
        }
        return locale.toString(value.toDouble(), 'f', 6);
    }

private:
    const Signal* signal_{nullptr};
};
}  // namespace

SignalTablePanel::SignalTablePanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("PanelCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* title_row = new QHBoxLayout();
    auto* title = new QLabel(QStringLiteral("Samples"), this);
    title->setObjectName(QStringLiteral("PanelTitle"));
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() + 1.5);
    tf.setBold(true);
    title->setFont(tf);
    title_row->addWidget(title);
    title_row->addStretch(1);
    root->addLayout(title_row);

    stats_label_ = new QLabel(QStringLiteral("No signal selected"), this);
    stats_label_->setObjectName(QStringLiteral("PanelSummary"));
    stats_label_->setWordWrap(true);
    root->addWidget(stats_label_);

    hint_label_ = new QLabel(QStringLiteral("Double-click cells to edit values or use the buttons to add and remove samples."), this);
    hint_label_->setObjectName(QStringLiteral("PanelDetail"));
    hint_label_->setWordWrap(true);
    root->addWidget(hint_label_);

    table_ = new QTableWidget(this);
    table_->setObjectName(QStringLiteral("SignalSamplesTable"));
    table_->setColumnCount(2);
    table_->setHorizontalHeaderLabels({QStringLiteral("time"), QStringLiteral("value")});
    table_->setAlternatingRowColors(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::DoubleClicked |
                            QAbstractItemView::EditKeyPressed |
                            QAbstractItemView::AnyKeyPressed);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->verticalHeader()->setVisible(false);
    table_->setShowGrid(false);
    table_->setFrameShape(QFrame::NoFrame);
    table_->viewport()->setAutoFillBackground(true);
    item_delegate_ = new SampleItemDelegate(table_);
    table_->setItemDelegate(item_delegate_);
    root->addWidget(table_, 1);

    auto* buttons = new QHBoxLayout();
    add_button_ = new QPushButton(QStringLiteral("+ Sample"), this);
    remove_button_ = new QPushButton(QStringLiteral("- Sample"), this);
    add_button_->setObjectName(QStringLiteral("AccentButton"));
    remove_button_->setObjectName(QStringLiteral("SubtleButton"));
    add_button_->setCursor(Qt::PointingHandCursor);
    remove_button_->setCursor(Qt::PointingHandCursor);
    buttons->addWidget(add_button_);
    buttons->addWidget(remove_button_);
    root->addLayout(buttons);

    connect(table_, &QTableWidget::itemChanged, this, &SignalTablePanel::onItemChanged);
    connect(table_, &QTableWidget::currentCellChanged, this,
            [this](int current_row, int, int, int) {
                remove_button_->setEnabled(signal_ != nullptr && current_row >= 0);
            });
    connect(add_button_, &QPushButton::clicked, this, &SignalTablePanel::onAddClicked);
    connect(remove_button_, &QPushButton::clicked, this, &SignalTablePanel::onRemoveClicked);

    refresh_summary();
}

void SignalTablePanel::set_signal(const Signal* signal) {
    signal_ = signal;
    if (auto* delegate = dynamic_cast<SampleItemDelegate*>(item_delegate_); delegate != nullptr) {
        delegate->set_signal(signal_);
    }
    repopulate();
}

void SignalTablePanel::refresh() {
    repopulate();
}

std::vector<SamplePoint> SignalTablePanel::samples() const {
    std::vector<SamplePoint> out;
    out.reserve(static_cast<std::size_t>(table_->rowCount()));
    for (int row = 0; row < table_->rowCount(); ++row) {
        const auto* time_item = table_->item(row, 0);
        const auto* value_item = table_->item(row, 1);
        const double t = time_item == nullptr ? 0.0 : time_item->data(Qt::EditRole).toDouble();

        double y = 0.0;
        if (value_item != nullptr) {
            const QVariant raw_value = value_item->data(Qt::EditRole);
            if (signal_ != nullptr && signal_->is_enumerated()) {
                y = signal_->value_for_label(raw_value.toString().toStdString());
            } else {
                y = raw_value.toDouble();
            }
        }
        out.push_back(SamplePoint{t, y});
    }
    return out;
}

void SignalTablePanel::onItemChanged(QTableWidgetItem* item) {
    if (suppress_item_changed_ || item == nullptr) {
        return;
    }
    emit editStarted();
    emit contentChanged();
}

void SignalTablePanel::onAddClicked() {
    if (signal_ == nullptr) {
        return;
    }

    emit editStarted();

    const int insert_row = table_->rowCount();
    const int edit_column = 0;

    suppress_item_changed_ = true;
    table_->insertRow(insert_row);
    set_row_values(insert_row, default_insert_time(), default_insert_value());
    suppress_item_changed_ = false;

    table_->selectRow(insert_row);
    table_->setCurrentCell(insert_row, edit_column);
    table_->scrollToItem(table_->item(insert_row, edit_column), QAbstractItemView::PositionAtCenter);
    remove_button_->setEnabled(true);
    emit contentChanged();

    QTimer::singleShot(0, this, [this, insert_row, edit_column]() {
        if (insert_row < 0 || insert_row >= table_->rowCount()) {
            return;
        }
        auto* item = table_->item(insert_row, edit_column);
        if (item == nullptr) {
            return;
        }
        table_->selectRow(insert_row);
        table_->setCurrentItem(item);
        table_->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        table_->editItem(item);
    });
}

void SignalTablePanel::onRemoveClicked() {
    const int row = table_->currentRow();
    if (row < 0 || row >= table_->rowCount()) {
        return;
    }

    emit editStarted();
    suppress_item_changed_ = true;
    table_->removeRow(row);
    suppress_item_changed_ = false;
    remove_button_->setEnabled(signal_ != nullptr && table_->currentRow() >= 0);
    emit contentChanged();
}

void SignalTablePanel::repopulate() {
    suppress_item_changed_ = true;
    const int previous_row = table_->currentRow();
    table_->clearContents();
    table_->setRowCount(0);

    if (signal_ != nullptr) {
        const auto& signal_samples = signal_->samples();
        table_->setRowCount(static_cast<int>(signal_samples.size()));
        for (int row = 0; row < static_cast<int>(signal_samples.size()); ++row) {
            set_row_values(row, signal_samples[static_cast<std::size_t>(row)].t,
                           signal_samples[static_cast<std::size_t>(row)].y);
        }
    }

    if (previous_row >= 0 && previous_row < table_->rowCount()) {
        table_->setCurrentCell(previous_row, 0);
    } else if (table_->rowCount() > 0) {
        table_->setCurrentCell(0, 0);
    }

    const bool has_signal = signal_ != nullptr;
    add_button_->setEnabled(has_signal);
    remove_button_->setEnabled(has_signal && table_->currentRow() >= 0);
    suppress_item_changed_ = false;
    refresh_summary();
}

void SignalTablePanel::set_row_values(int row, double t, double y) {
    auto* time_item = table_->item(row, 0);
    if (time_item == nullptr) {
        time_item = new QTableWidgetItem();
        table_->setItem(row, 0, time_item);
    }
    time_item->setData(Qt::EditRole, t);

    auto* value_item = table_->item(row, 1);
    if (value_item == nullptr) {
        value_item = new QTableWidgetItem();
        table_->setItem(row, 1, value_item);
    }

    if (signal_ != nullptr && signal_->is_enumerated()) {
        value_item->setData(Qt::EditRole, QString::fromStdString(signal_->label_for_value(y)));
    } else {
        value_item->setData(Qt::EditRole, y);
    }
}

double SignalTablePanel::default_insert_time() const {
    if (signal_ == nullptr || signal_->empty()) {
        return 0.0;
    }

    const auto& signal_samples = signal_->samples();
    if (signal_samples.size() >= 2) {
        const double last = signal_samples.back().t;
        const double prev = signal_samples[signal_samples.size() - 2].t;
        const double step = std::fabs(last - prev) > 1e-9 ? (last - prev) : 1.0;
        return last + step;
    }

    return signal_samples.back().t + 1.0;
}

double SignalTablePanel::default_insert_value() const {
    if (signal_ == nullptr) {
        return 0.0;
    }

    if (signal_->is_enumerated() && !signal_->enumeration().empty()) {
        if (!signal_->empty()) {
            return signal_->samples().back().y;
        }
        return signal_->enumeration().front().value;
    }

    if (signal_->empty()) {
        return 0.0;
    }
    return signal_->samples().back().y;
}

void SignalTablePanel::refresh_summary() const {
    if (signal_ == nullptr) {
        stats_label_->setText(QStringLiteral("No signal selected"));
        hint_label_->setText(QStringLiteral("Select a signal to inspect its sample points and interpolation mode."));
        return;
    }

    const QString interpolation = signal_->interpolation() == Signal::InterpolationMode::Step
        ? QStringLiteral("step")
        : QStringLiteral("linear");
    const QString enum_summary = signal_->is_enumerated()
        ? QStringLiteral(" | %1 states").arg(signal_->enumeration().size())
        : QString();
    stats_label_->setText(
        QStringLiteral("%1 rows | range %2s to %3s | %4 interpolation%5")
            .arg(signal_->size())
            .arg(signal_->empty() ? 0.0 : signal_->t_min(), 0, 'f', 4)
            .arg(signal_->empty() ? 0.0 : signal_->t_max(), 0, 'f', 4)
            .arg(interpolation)
            .arg(enum_summary));

    if (signal_->is_enumerated()) {
        hint_label_->setText(QStringLiteral("Enumerated signals use label-based editing in the value column and are always rendered with step interpolation."));
    } else {
        hint_label_->setText(QStringLiteral("Use the table for precise edits. Changes are mirrored in the plot immediately."));
    }
}

}  // namespace myprj::signal_editor::adapters::qt
