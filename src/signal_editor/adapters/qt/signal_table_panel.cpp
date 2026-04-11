#include "signal_editor/adapters/qt/signal_table_panel.h"

#include "signal_editor/core/domain/signal.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVariant>
#include <QMetaType>

#include <cmath>
#include <stdexcept>

namespace myprj::signal_editor::adapters::qt {

namespace {
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
            for (const auto& entry : signal_->enumeration()) {
                editor->addItem(QString::fromStdString(entry.label));
            }
            editor->setEditable(false);
            return editor;
        }

        auto* editor = new QDoubleSpinBox(parent);
        editor->setDecimals(6);
        editor->setRange(-1e12, 1e12);
        editor->setSingleStep(index.column() == 0 ? 0.01 : 0.1);
        editor->setFrame(false);
        return editor;
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

        auto* spin = qobject_cast<QDoubleSpinBox*>(editor);
        if (spin == nullptr) {
            return;
        }
        spin->setValue(index.data(Qt::EditRole).toDouble());
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

        auto* spin = qobject_cast<QDoubleSpinBox*>(editor);
        if (spin == nullptr) {
            return;
        }
        spin->interpretText();
        model->setData(index, spin->value(), Qt::EditRole);
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
    item_delegate_ = new SampleItemDelegate(table_);
    table_->setItemDelegate(item_delegate_);
    root->addWidget(table_, 1);

    auto* buttons = new QHBoxLayout();
    add_button_ = new QPushButton(QStringLiteral("+ Sample"), this);
    remove_button_ = new QPushButton(QStringLiteral("- Sample"), this);
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
    emit editStarted();

    const int current_row = table_->currentRow();
    const int insert_row = current_row >= 0 ? current_row + 1 : table_->rowCount();

    suppress_item_changed_ = true;
    table_->insertRow(insert_row);
    set_row_values(insert_row,
                   default_insert_time(current_row),
                   default_insert_value(current_row));
    suppress_item_changed_ = false;

    table_->setCurrentCell(insert_row, signal_ != nullptr && signal_->is_enumerated() ? 1 : 0);
    remove_button_->setEnabled(true);
    table_->editItem(table_->item(insert_row, signal_ != nullptr && signal_->is_enumerated() ? 1 : 0));
    emit contentChanged();
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

double SignalTablePanel::default_insert_time(int row) const {
    const auto current_samples = samples();
    if (current_samples.empty()) {
        return 0.0;
    }
    if (row >= 0 && row + 1 < static_cast<int>(current_samples.size())) {
        return 0.5 * (current_samples[static_cast<std::size_t>(row)].t +
                      current_samples[static_cast<std::size_t>(row + 1)].t);
    }

    if (current_samples.size() >= 2) {
        const double last = current_samples.back().t;
        const double prev = current_samples[current_samples.size() - 2].t;
        const double step = std::fabs(last - prev) > 1e-9 ? (last - prev) : 1.0;
        return last + step;
    }

    return current_samples.back().t + 1.0;
}

double SignalTablePanel::default_insert_value(int row) const {
    if (signal_ != nullptr && signal_->is_enumerated() && !signal_->enumeration().empty()) {
        if (row >= 0) {
            const auto current_samples = samples();
            if (row < static_cast<int>(current_samples.size())) {
                return current_samples[static_cast<std::size_t>(row)].y;
            }
        }
        return signal_->enumeration().front().value;
    }

    const auto current_samples = samples();
    if (current_samples.empty()) {
        return 0.0;
    }
    if (row >= 0 && row < static_cast<int>(current_samples.size())) {
        return current_samples[static_cast<std::size_t>(row)].y;
    }
    return current_samples.back().y;
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
