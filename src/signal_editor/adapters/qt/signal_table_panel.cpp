#include "signal_editor/adapters/qt/signal_table_panel.h"

#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleValidator>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QMenu>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QMetaType>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace signal_editor::adapters::qt {

#define tr qt_signal_table_panel_tr

namespace {
constexpr int kMaxMaterializedRows = 10000;

QString qt_signal_table_panel_tr(const char* source_text,
                                 const char* disambiguation = nullptr,
                                 int n = -1) {
    return QCoreApplication::translate("SignalTablePanel", source_text,
                                       disambiguation, n);
}

double parse_flexible_double(const QString& text, bool* ok = nullptr) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        if (ok != nullptr) {
            *ok = false;
        }
        return 0.0;
    }

    QString normalized = trimmed;
    const int last_dot = normalized.lastIndexOf('.');
    const int last_comma = normalized.lastIndexOf(',');
    const int decimal_index = std::max(last_dot, last_comma);
    for (int i = 0; i < normalized.size(); ++i) {
        if (normalized[i] == '.' || normalized[i] == ',') {
            normalized[i] = (i == decimal_index) ? QChar('.') : QChar();
        }
    }
    normalized.remove(QChar());

    bool local_ok = false;
    const double value = normalized.toDouble(&local_ok);
    if (ok != nullptr) {
        *ok = local_ok;
    }
    return value;
}

QString format_line_edit_double(const QLocale& locale, double value, int decimals) {
    QLocale copy(locale);
    copy.setNumberOptions(copy.numberOptions() | QLocale::OmitGroupSeparator);
    return copy.toString(value, 'f', decimals);
}

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
    void set_active_value_column(int column) noexcept { active_value_column_ = column; }

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem&,
                          const QModelIndex& index) const override {
        if (index.column() == active_value_column_ &&
            signal_ != nullptr &&
            signal_->is_enumerated()) {
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
        editor->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral(R"(^[+-]?(?:\d+([.,]\d*)?|[.,]\d+)$)")),
            editor));
        return editor;
    }

    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex&) const override {
        editor->setGeometry(option.rect.adjusted(3, 3, -3, -3));
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        if (index.column() == active_value_column_ &&
            signal_ != nullptr &&
            signal_->is_enumerated()) {
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
        line_edit->setText(format_line_edit_double(line_edit->locale(),
                                                   index.data(Qt::EditRole).toDouble(),
                                                   6));
        line_edit->selectAll();
    }

    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override {
        if (index.column() == active_value_column_ &&
            signal_ != nullptr &&
            signal_->is_enumerated()) {
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
        const double value = parse_flexible_double(line_edit->text(), &ok);
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
    int active_value_column_{-1};
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
    title_label_ = new QLabel(this);
    title_label_->setObjectName(QStringLiteral("PanelTitle"));
    title_label_->setProperty("uiFontRole", QStringLiteral("panel-title"));
    title_row->addWidget(title_label_);
    title_row->addStretch(1);
    root->addLayout(title_row);

    stats_label_ = new QLabel(this);
    stats_label_->setObjectName(QStringLiteral("PanelSummary"));
    stats_label_->setWordWrap(true);
    root->addWidget(stats_label_);

    hint_label_ = new QLabel(this);
    hint_label_->setObjectName(QStringLiteral("PanelDetail"));
    hint_label_->setWordWrap(true);
    root->addWidget(hint_label_);

    search_edit_ = new QLineEdit(this);
    search_edit_->setObjectName(QStringLiteral("TableSearchEdit"));
    root->addWidget(search_edit_);

    table_ = new QTableWidget(this);
    table_->setObjectName(QStringLiteral("SignalSamplesTable"));
    table_->setColumnCount(1);
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
    table_->setContextMenuPolicy(Qt::CustomContextMenu);
    table_->viewport()->setAutoFillBackground(true);
    item_delegate_ = new SampleItemDelegate(table_);
    table_->setItemDelegate(item_delegate_);
    root->addWidget(table_, 1);

    auto* buttons = new QHBoxLayout();
    add_button_ = new QPushButton(this);
    remove_button_ = new QPushButton(this);
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
                remove_button_->setEnabled(active_signal() != nullptr &&
                                           is_active_signal_displayed() &&
                                           current_row >= 0);
            });
    connect(add_button_, &QPushButton::clicked, this, &SignalTablePanel::onAddClicked);
    connect(remove_button_, &QPushButton::clicked, this, &SignalTablePanel::onRemoveClicked);
    connect(table_, &QTableWidget::customContextMenuRequested,
            this, &SignalTablePanel::onContextMenuRequested);
    connect(search_edit_, &QLineEdit::textChanged,
            this, [this]() { apply_search_filter(); });

    refresh_typography();
    retranslate_ui();
    refresh_summary();
}

void SignalTablePanel::set_library(const SignalLibrary* library,
                                   int active_signal_index,
                                   const std::vector<int>& visible_signal_indices) {
    library_ = library;
    active_signal_index_ = active_signal_index;
    visible_signal_indices_ = visible_signal_indices;
    if (auto* delegate = dynamic_cast<SampleItemDelegate*>(item_delegate_); delegate != nullptr) {
        delegate->set_signal(active_signal());
        delegate->set_active_value_column(active_value_column());
    }
    repopulate();
}

void SignalTablePanel::refresh() {
    repopulate();
}

void SignalTablePanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslate_ui();
        refresh_summary();
    } else if (event->type() == QEvent::FontChange ||
               event->type() == QEvent::ApplicationFontChange) {
        refresh_typography();
    }
    QWidget::changeEvent(event);
}

std::vector<SamplePoint> SignalTablePanel::samples() const {
    std::vector<SamplePoint> out;
    out.reserve(static_cast<std::size_t>(table_->rowCount()));
    const int active_column = active_value_column();
    const Signal* signal = active_signal();
    for (int row = 0; row < table_->rowCount(); ++row) {
        const auto* time_item = table_->item(row, 0);
        const auto* value_item = active_column > 0 ? table_->item(row, active_column) : nullptr;
        const double t = time_item == nullptr ? 0.0 : time_item->data(Qt::EditRole).toDouble();

        double y = 0.0;
        if (value_item != nullptr) {
            const QVariant raw_value = value_item->data(Qt::EditRole);
            if (signal != nullptr && signal->is_enumerated()) {
                y = signal->value_for_label(raw_value.toString().toStdString());
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
    const int active_column = active_value_column();
    const bool editable_change =
        item->column() == 0 || (active_column > 0 && item->column() == active_column);
    if (!editable_change || !is_active_signal_displayed()) {
        return;
    }
    emit editStarted();
    const int row = item->row();
    if (row >= 0 && row < table_->rowCount()) {
        const auto edited_samples = samples();
        if (static_cast<std::size_t>(row) < edited_samples.size()) {
            emit sampleEdited(row, edited_samples[static_cast<std::size_t>(row)].t,
                              edited_samples[static_cast<std::size_t>(row)].y);
        }
    }
    emit contentChanged();
}

void SignalTablePanel::onAddClicked() {
    if (active_signal() == nullptr || !is_active_signal_displayed()) {
        return;
    }

    emit editStarted();

    const int insert_row = table_->rowCount();
    const int edit_column = active_value_column() > 0 ? active_value_column() : 0;

    suppress_item_changed_ = true;
    table_->insertRow(insert_row);
    set_row_values(insert_row);
    auto* time_item = table_->item(insert_row, 0);
    if (time_item != nullptr) {
        time_item->setData(Qt::EditRole, default_insert_time());
    }
    if (edit_column > 0) {
        auto* value_item = table_->item(insert_row, edit_column);
        if (value_item != nullptr) {
            const Signal* signal = active_signal();
            const double value = default_insert_value();
            if (signal != nullptr && signal->is_enumerated()) {
                value_item->setData(Qt::EditRole,
                                    QString::fromStdString(signal->label_for_value(value)));
            } else {
                value_item->setData(Qt::EditRole, value);
            }
        }
    }
    suppress_item_changed_ = false;

    table_->selectRow(insert_row);
    table_->setCurrentCell(insert_row, edit_column);
    table_->scrollToItem(table_->item(insert_row, edit_column), QAbstractItemView::PositionAtCenter);
    remove_button_->setEnabled(true);
    emit sampleInserted(default_insert_time(), default_insert_value());
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
    remove_button_->setEnabled(active_signal() != nullptr &&
                               is_active_signal_displayed() &&
                               table_->currentRow() >= 0);
    emit sampleRemoved(row);
    emit contentChanged();
}

void SignalTablePanel::onContextMenuRequested(const QPoint& pos) {
    QMenu menu(this);
    QAction* offset_all_action = menu.addAction(tr("Apply offset to all samples in file"));
    QAction* offset_row_action = menu.addAction(tr("Apply offset to selected sample"));
    const int row = table_->rowAt(pos.y());
    offset_row_action->setEnabled(row >= 0 && is_active_signal_displayed());

    QAction* chosen = menu.exec(table_->viewport()->mapToGlobal(pos));
    if (chosen == nullptr) {
        return;
    }

    if (chosen == offset_all_action) {
        bool ok = false;
        const double delta = QInputDialog::getDouble(
            this,
            tr("Offset all samples"),
            tr("Offset to add to every sample in the current file"),
            0.0, -1e12, 1e12, 6, &ok);
        if (ok) {
            emit editStarted();
            emit offsetAllRequested(delta);
            emit contentChanged();
        }
        return;
    }

    if (chosen == offset_row_action && row >= 0) {
        bool ok = false;
        const double delta = QInputDialog::getDouble(
            this,
            tr("Offset selected sample"),
            tr("Offset to add to the selected sample"),
            0.0, -1e12, 1e12, 6, &ok);
        if (ok) {
            emit editStarted();
            emit sampleOffsetRequested(row, delta);
            emit contentChanged();
        }
    }
}

void SignalTablePanel::retranslate_ui() {
    if (title_label_ != nullptr) {
        title_label_->setText(tr("Samples"));
        title_label_->setToolTip(tr("Editable sample points of the currently selected signal."));
    }
    if (stats_label_ != nullptr) {
        stats_label_->setToolTip(tr("Summary of rows, time range, and interpolation for the current signal."));
    }
    if (hint_label_ != nullptr) {
        hint_label_->setToolTip(tr("Editing hint for the current signal type and table behavior."));
    }
    if (table_ != nullptr) {
        QStringList headers;
        headers.push_back(tr("time"));
        for (int signal_index : displayed_signal_indices()) {
            if (library_ == nullptr ||
                signal_index < 0 ||
                signal_index >= static_cast<int>(library_->size())) {
                continue;
            }
            headers.push_back(QString::fromStdString(
                library_->at(static_cast<std::size_t>(signal_index)).name()));
        }
        table_->setHorizontalHeaderLabels(headers);
        table_->setToolTip(tr("Edit sample times and values. Double-click a cell or start typing to modify it."));
        if (table_->horizontalHeaderItem(0) != nullptr) {
            table_->horizontalHeaderItem(0)->setToolTip(tr("Time coordinate of the sample point."));
        }
        for (int column = 1; column < table_->columnCount(); ++column) {
            if (table_->horizontalHeaderItem(column) != nullptr) {
                table_->horizontalHeaderItem(column)->setToolTip(
                    column == active_value_column()
                        ? tr("Editable values of the active signal.")
                        : tr("Read-only values of another plotted signal."));
            }
        }
    }
    if (search_edit_ != nullptr) {
        search_edit_->setPlaceholderText(tr("Search in table..."));
        search_edit_->setToolTip(
            tr("Filter rows by matching time or value text in any visible column."));
        search_edit_->setClearButtonEnabled(true);
    }
    if (add_button_ != nullptr) {
        add_button_->setText(tr("+ Sample"));
        add_button_->setToolTip(tr("Insert a new sample after the current table content."));
        add_button_->setStatusTip(tr("Add a new sample row."));
    }
    if (remove_button_ != nullptr) {
        remove_button_->setText(tr("- Sample"));
        remove_button_->setToolTip(tr("Remove the currently selected sample row."));
        remove_button_->setStatusTip(tr("Remove the selected sample row."));
    }
}

void SignalTablePanel::refresh_typography() {
    if (title_label_ == nullptr) {
        return;
    }
    QFont title_font = font();
    title_font.setPointSizeF(title_font.pointSizeF() + 1.5);
    title_font.setBold(true);
    title_label_->setFont(title_font);
}

void SignalTablePanel::repopulate() {
    suppress_item_changed_ = true;
    const int previous_row = table_->currentRow();
    table_->clearContents();
    table_->setRowCount(0);

    const auto display_indices = displayed_signal_indices();
    table_->setColumnCount(1 + static_cast<int>(display_indices.size()));

    if (const Signal* signal = reference_signal(); signal != nullptr) {
        displayed_row_count_ = static_cast<int>(
            std::min<std::size_t>(signal->size(), kMaxMaterializedRows));
        table_->setRowCount(displayed_row_count_);
        for (int row = 0; row < displayed_row_count_; ++row) {
            set_row_values(row);
        }
    } else {
        displayed_row_count_ = 0;
    }

    if (previous_row >= 0 && previous_row < table_->rowCount()) {
        table_->setCurrentCell(previous_row, 0);
    } else if (table_->rowCount() > 0) {
        table_->setCurrentCell(0, 0);
    }

    retranslate_ui();
    if (auto* delegate = dynamic_cast<SampleItemDelegate*>(item_delegate_); delegate != nullptr) {
        delegate->set_signal(active_signal());
        delegate->set_active_value_column(active_value_column());
    }
    const bool has_signal = active_signal() != nullptr && is_active_signal_displayed();
    add_button_->setEnabled(has_signal);
    remove_button_->setEnabled(has_signal && table_->currentRow() >= 0);
    suppress_item_changed_ = false;
    apply_search_filter();
    refresh_summary();
}

void SignalTablePanel::set_row_values(int row) {
    const Signal* reference = reference_signal();
    if (reference == nullptr || row < 0 || row >= static_cast<int>(reference->size())) {
        return;
    }

    auto* time_item = table_->item(row, 0);
    if (time_item == nullptr) {
        time_item = new QTableWidgetItem();
        table_->setItem(row, 0, time_item);
    }
    time_item->setData(Qt::EditRole, reference->samples()[static_cast<std::size_t>(row)].t);
    Qt::ItemFlags time_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (is_active_signal_displayed()) {
        time_flags |= Qt::ItemIsEditable;
    }
    time_item->setFlags(time_flags);

    const auto display_indices = displayed_signal_indices();
    for (int column = 0; column < static_cast<int>(display_indices.size()); ++column) {
        const int signal_index = display_indices[static_cast<std::size_t>(column)];
        if (library_ == nullptr ||
            signal_index < 0 ||
            signal_index >= static_cast<int>(library_->size())) {
            continue;
        }
        const auto& signal = library_->at(static_cast<std::size_t>(signal_index));
        auto* value_item = table_->item(row, column + 1);
        if (value_item == nullptr) {
            value_item = new QTableWidgetItem();
            table_->setItem(row, column + 1, value_item);
        }
        if (row >= static_cast<int>(signal.size())) {
            value_item->setData(Qt::DisplayRole, QString());
            value_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            continue;
        }
        const double y = signal.samples()[static_cast<std::size_t>(row)].y;
        if (signal.is_enumerated()) {
            value_item->setData(Qt::EditRole, QString::fromStdString(signal.label_for_value(y)));
        } else {
            value_item->setData(Qt::EditRole, y);
        }
        Qt::ItemFlags value_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (signal_index == active_signal_index_) {
            value_flags |= Qt::ItemIsEditable;
        }
        value_item->setFlags(value_flags);
    }
}

double SignalTablePanel::default_insert_time() const {
    const Signal* signal = active_signal();
    if (signal == nullptr || signal->empty()) {
        return 0.0;
    }

    const auto& signal_samples = signal->samples();
    if (signal_samples.size() >= 2) {
        const double last = signal_samples.back().t;
        const double prev = signal_samples[signal_samples.size() - 2].t;
        const double step = std::fabs(last - prev) > 1e-9 ? (last - prev) : 1.0;
        return last + step;
    }

    return signal_samples.back().t + 1.0;
}

double SignalTablePanel::default_insert_value() const {
    const Signal* signal = active_signal();
    if (signal == nullptr) {
        return 0.0;
    }

    if (signal->is_enumerated() && !signal->enumeration().empty()) {
        if (!signal->empty()) {
            return signal->samples().back().y;
        }
        return signal->enumeration().front().value;
    }

    if (signal->empty()) {
        return 0.0;
    }
    return signal->samples().back().y;
}

std::vector<int> SignalTablePanel::displayed_signal_indices() const {
    std::vector<int> out;
    if (library_ == nullptr) {
        return out;
    }
    for (int index : visible_signal_indices_) {
        if (index >= 0 &&
            index < static_cast<int>(library_->size()) &&
            std::find(out.begin(), out.end(), index) == out.end()) {
            out.push_back(index);
        }
    }
    return out;
}

const Signal* SignalTablePanel::reference_signal() const {
    if (const Signal* signal = active_signal();
        signal != nullptr && is_active_signal_displayed()) {
        return signal;
    }
    const auto display_indices = displayed_signal_indices();
    if (library_ == nullptr || display_indices.empty()) {
        return nullptr;
    }
    return &library_->at(static_cast<std::size_t>(display_indices.front()));
}

const Signal* SignalTablePanel::active_signal() const {
    if (library_ == nullptr ||
        active_signal_index_ < 0 ||
        active_signal_index_ >= static_cast<int>(library_->size())) {
        return nullptr;
    }
    return &library_->at(static_cast<std::size_t>(active_signal_index_));
}

bool SignalTablePanel::is_active_signal_displayed() const {
    const auto display_indices = displayed_signal_indices();
    return std::find(display_indices.begin(), display_indices.end(), active_signal_index_) !=
           display_indices.end();
}

int SignalTablePanel::active_value_column() const {
    const auto display_indices = displayed_signal_indices();
    for (std::size_t i = 0; i < display_indices.size(); ++i) {
        if (display_indices[i] == active_signal_index_) {
            return static_cast<int>(i) + 1;
        }
    }
    return -1;
}

bool SignalTablePanel::is_row_display_truncated() const {
    const Signal* signal = reference_signal();
    return signal != nullptr &&
           signal->size() > static_cast<std::size_t>(displayed_row_count_);
}

const Signal* SignalTablePanel::signal_for_value_column(int column) const {
    if (column <= 0) {
        return nullptr;
    }
    const auto display_indices = displayed_signal_indices();
    const std::size_t position = static_cast<std::size_t>(column - 1);
    if (library_ == nullptr || position >= display_indices.size()) {
        return nullptr;
    }
    return &library_->at(static_cast<std::size_t>(display_indices[position]));
}

void SignalTablePanel::refresh_summary() const {
    const Signal* signal = active_signal();
    if (signal == nullptr) {
        stats_label_->setText(tr("No signal selected"));
        hint_label_->setText(tr("Select and plot a signal to inspect its sample points and interpolation mode."));
        return;
    }
    const int plotted_columns = static_cast<int>(displayed_signal_indices().size());
    if (!is_active_signal_displayed()) {
        stats_label_->setText(
            tr("%1 plotted signal(s) visible, but the active signal is unchecked")
                .arg(plotted_columns));
        hint_label_->setText(
            tr("Check the active signal in the sidebar to edit its values and shared time axis."));
        return;
    }

    const QString interpolation = signal->interpolation() == Signal::InterpolationMode::Step
        ? tr("step")
        : tr("linear");
    const QString enum_summary = signal->is_enumerated()
        ? tr(" | %1 states").arg(signal->enumeration().size())
        : QString();
    const QString row_summary = is_row_display_truncated()
        ? tr("%1 of %2 rows shown").arg(displayed_row_count_).arg(signal->size())
        : tr("%1 rows").arg(signal->size());
    stats_label_->setText(
        tr("%1 | range %2s to %3s | %4 interpolation | %5 plotted%6")
            .arg(row_summary)
            .arg(signal->empty() ? 0.0 : signal->t_min(), 0, 'f', 4)
            .arg(signal->empty() ? 0.0 : signal->t_max(), 0, 'f', 4)
            .arg(interpolation)
            .arg(plotted_columns)
            .arg(enum_summary));

    if (signal->is_enumerated()) {
        hint_label_->setText(tr("Enumerated signals use label-based editing in the active value column; other plotted columns stay read-only."));
    } else if (is_row_display_truncated()) {
        hint_label_->setText(tr("Large signals are previewed in the table to keep the interface responsive. Use the plot to inspect the full signal."));
    } else {
        hint_label_->setText(tr("Use the table for precise edits. The time column and active value column are editable; other plotted signals are read-only for comparison."));
    }
}

void SignalTablePanel::apply_search_filter() {
    if (table_ == nullptr) {
        return;
    }
    const QString pattern = search_edit_ == nullptr
        ? QString()
        : search_edit_->text().trimmed();
    const bool has_pattern = !pattern.isEmpty();
    for (int row = 0; row < table_->rowCount(); ++row) {
        bool matches = !has_pattern;
        if (has_pattern) {
            for (int column = 0; column < table_->columnCount(); ++column) {
                const auto* item = table_->item(row, column);
                if (item != nullptr &&
                    item->text().contains(pattern, Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
        }
        table_->setRowHidden(row, !matches);
    }
}

#undef tr

}  // namespace signal_editor::adapters::qt
