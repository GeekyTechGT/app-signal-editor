#include "signal_editor/adapters/qt/signal_list_panel.h"

#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMetaObject>
#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionButton>
#include <QSignalBlocker>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace signal_editor::adapters::qt {

#define tr qt_signal_list_panel_tr

namespace {
QString qt_signal_list_panel_tr(const char* source_text,
                                const char* disambiguation = nullptr,
                                int n = -1) {
    return QCoreApplication::translate("SignalListPanel", source_text,
                                       disambiguation, n);
}

enum ItemRole : int {
    ActiveRole = Qt::UserRole + 1,
    VisibleRole,
    StoredNameRole,
};

class SignalListItemDelegate final : public QStyledItemDelegate {
public:
    explicit SignalListItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        const bool active = index.data(ActiveRole).toBool();
        const bool checked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        const QString text = opt.text;
        opt.text.clear();
        const QString badge_text = qt_signal_list_panel_tr("Active");

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRectF card = opt.rect.adjusted(4, 4, -4, -4);
        QColor fill = opt.palette.base().color();
        if (opt.state.testFlag(QStyle::State_Selected)) {
            fill = opt.palette.highlight().color().lighter(182);
        } else if (opt.features.testFlag(QStyleOptionViewItem::Alternate)) {
            fill = opt.palette.alternateBase().color();
        }

        QColor border = active
            ? QColor(20, 126, 160)
            : opt.palette.mid().color();
        border.setAlpha(active ? 210 : 110);

        painter->setPen(QPen(border, active ? 2.2 : 1.0));
        painter->setBrush(fill);
        painter->drawRoundedRect(card, 10.0, 10.0);

        QStyleOptionButton checkbox;
        checkbox.state = QStyle::State_Enabled | (checked ? QStyle::State_On : QStyle::State_Off);
        checkbox.rect = checkbox_rect(card);
        QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkbox, painter);

        QFont text_font = opt.font;
        text_font.setBold(active);
        const QFontMetrics text_metrics(text_font);
        const int text_left = checkbox.rect.right() + 10;
        QRect text_rect(static_cast<int>(text_left),
                        static_cast<int>(card.top()),
                        static_cast<int>(card.right()) - text_left - 12,
                        static_cast<int>(card.height()));

        QRect badge_rect;
        if (active) {
            QFont badge_font = opt.font;
            badge_font.setBold(true);
            badge_font.setPointSizeF(std::max(8.0, badge_font.pointSizeF() - 0.5));
            const QFontMetrics badge_metrics(badge_font);
            badge_rect = badge_metrics.boundingRect(badge_text).adjusted(-10, -4, 10, 4);
            badge_rect.moveLeft(static_cast<int>(card.right()) - badge_rect.width() - 12);
            badge_rect.moveTop(static_cast<int>(card.center().y() - badge_rect.height() / 2.0));

            const int minimum_gap = 10;
            text_rect.setRight(std::max(text_rect.left(),
                                        badge_rect.left() - minimum_gap));

            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(20, 126, 160, 38));
            painter->drawRoundedRect(badge_rect, 8.0, 8.0);
            painter->setPen(QColor(20, 126, 160));
            painter->setFont(badge_font);
            painter->drawText(badge_rect, Qt::AlignCenter, badge_text);
        }

        painter->setPen(opt.palette.text().color());
        painter->setFont(text_font);
        painter->drawText(text_rect,
                          Qt::AlignVCenter | Qt::AlignLeft,
                          text_metrics.elidedText(text, Qt::ElideRight,
                                                  std::max(0, text_rect.width())));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(std::max(size.height(), 34));
        return size;
    }

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem&,
                          const QModelIndex&) const override {
        auto* editor = new QLineEdit(parent);
        editor->setFrame(false);
        return editor;
    }

    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex&) const override {
        editor->setGeometry(option.rect.adjusted(38, 6, -14, -6));
    }

    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override {
        if (model == nullptr || !index.isValid()) {
            return false;
        }

        if (event->type() == QEvent::MouseButtonRelease) {
            const auto* mouse_event = dynamic_cast<QMouseEvent*>(event);
            if (mouse_event == nullptr) {
                return false;
            }
            const QRectF card = option.rect.adjusted(4, 4, -4, -4);
            if (!checkbox_rect(card).contains(mouse_event->position().toPoint())) {
                return QStyledItemDelegate::editorEvent(event, model, option, index);
            }

            if (auto* list_widget = qobject_cast<QListWidget*>(parent()); list_widget != nullptr) {
                list_widget->setProperty("checkboxToggleInProgress", true);
            }

            const Qt::CheckState current_state =
                index.data(Qt::CheckStateRole).toInt() == Qt::Checked
                    ? Qt::Checked
                    : Qt::Unchecked;
            const Qt::CheckState next_state =
                current_state == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            return model->setData(index, next_state, Qt::CheckStateRole);
        }

        if (event->type() == QEvent::MouseButtonDblClick) {
            const auto* mouse_event = dynamic_cast<QMouseEvent*>(event);
            if (mouse_event != nullptr) {
                const QRectF card = option.rect.adjusted(4, 4, -4, -4);
                if (checkbox_rect(card).contains(mouse_event->position().toPoint())) {
                    return true;
                }
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

private:
    static QRect checkbox_rect(const QRectF& card) {
        return QRect(static_cast<int>(card.left()) + 10,
                     static_cast<int>(card.center().y()) - 8,
                     16, 16);
    }
};

bool contains_index(const std::vector<int>& indices, int index) {
    return std::find(indices.begin(), indices.end(), index) != indices.end();
}

void emit_visibility_changed_later(SignalListPanel* panel, int row, bool visible) {
    QMetaObject::invokeMethod(panel,
                              [panel, row, visible]() {
                                  emit panel->visibilityChanged(row, visible);
                              },
                              Qt::QueuedConnection);
}

void emit_selection_changed_later(SignalListPanel* panel, int row) {
    QMetaObject::invokeMethod(panel,
                              [panel, row]() {
                                  emit panel->selectionChanged(row);
                              },
                              Qt::QueuedConnection);
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
    list_->setItemDelegate(new SignalListItemDelegate(list_));
    list_->setSpacing(4);
    root->addWidget(list_, 1);

    auto* buttons = new QHBoxLayout();
    add_button_ = new QPushButton(this);
    options_button_ = new QPushButton(this);
    remove_button_ = new QPushButton(this);
    add_button_->setCursor(::Qt::PointingHandCursor);
    options_button_->setCursor(::Qt::PointingHandCursor);
    remove_button_->setCursor(::Qt::PointingHandCursor);
    buttons->addWidget(add_button_);
    buttons->addWidget(options_button_);
    buttons->addWidget(remove_button_);
    root->addLayout(buttons);

    connect(list_, &QListWidget::currentRowChanged,
            this, &SignalListPanel::onCurrentRowChanged);
    connect(list_, &QListWidget::itemChanged,
            this, &SignalListPanel::onItemChanged);
    connect(add_button_, &QPushButton::clicked,
            this, &SignalListPanel::onAddClicked);
    connect(options_button_, &QPushButton::clicked,
            this, &SignalListPanel::onOptionsClicked);
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

void SignalListPanel::set_visible_signal_indices(const std::vector<int>& indices) {
    visible_signal_indices_ = indices;
    refresh();
}

void SignalListPanel::set_active_signal_index(int index) {
    active_signal_index_ = index;
    refresh();
}

void SignalListPanel::refresh() {
    suppress_item_changed_ = true;
    const int previous = list_->currentRow();
    const QSignalBlocker blocker(list_);
    list_->clear();
    if (library_ != nullptr) {
        for (std::size_t signal_index = 0; signal_index < library_->size(); ++signal_index) {
            const auto& s = library_->at(signal_index);
            auto* item = new QListWidgetItem(QString::fromStdString(s.name()), list_);
            item->setFlags(item->flags() |
                           ::Qt::ItemIsEditable |
                           ::Qt::ItemIsUserCheckable);
            item->setCheckState(contains_index(visible_signal_indices_,
                                              static_cast<int>(signal_index))
                                    ? Qt::Checked
                                    : Qt::Unchecked);
            item->setData(ActiveRole, static_cast<int>(signal_index) == active_signal_index_);
            item->setData(VisibleRole, item->checkState() == Qt::Checked);
            item->setData(StoredNameRole, item->text());
            item->setToolTip(
                tr("%1 samples. Check to show in plot and table; double-click to rename.")
                    .arg(static_cast<qulonglong>(s.size())));
        }
    }
    if (active_signal_index_ >= 0 && active_signal_index_ < list_->count()) {
        list_->setCurrentRow(active_signal_index_);
    } else if (active_signal_index_ < 0) {
        list_->setCurrentRow(-1);
    } else if (previous >= 0 && previous < list_->count()) {
        list_->setCurrentRow(previous);
    } else {
        list_->setCurrentRow(-1);
    }
    suppress_item_changed_ = false;
    refresh_summary();
}

void SignalListPanel::select(int index) {
    if (index < 0) {
        list_->setCurrentRow(-1);
        return;
    }
    if (index < list_->count()) {
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
    const bool checkbox_toggle_in_progress =
        list_ != nullptr && list_->property("checkboxToggleInProgress").toBool();
    if (list_ != nullptr) {
        list_->setProperty("checkboxToggleInProgress", false);
    }

    if (!checkbox_toggle_in_progress &&
        row >= 0 &&
        row < list_->count()) {
        auto* item = list_->item(row);
        if (item != nullptr && item->checkState() != Qt::Checked) {
            suppress_item_changed_ = true;
            item->setCheckState(Qt::Checked);
            item->setData(VisibleRole, true);
            suppress_item_changed_ = false;
            emit_visibility_changed_later(this, row, true);
        }
    }

    Q_UNUSED(row);
    refresh_summary();
    emit_selection_changed_later(this, row);
}

void SignalListPanel::onItemChanged(QListWidgetItem* item) {
    if (suppress_item_changed_ || item == nullptr) {
        return;
    }
    const int row = list_->row(item);
    const bool visible = item->checkState() == Qt::Checked;
    if (item->data(VisibleRole).toBool() != visible) {
        item->setData(VisibleRole, visible);
        emit_visibility_changed_later(this, row, visible);
    }
    const QString previous_name = item->data(StoredNameRole).toString();
    if (previous_name != item->text()) {
        item->setData(StoredNameRole, item->text());
        emit renameRequested(row, item->text());
    }
}

void SignalListPanel::onAddClicked() {
    emit addRequested();
}

void SignalListPanel::onRemoveClicked() {
    emit removeRequested(list_->currentRow());
}

void SignalListPanel::onOptionsClicked() {
    emit optionsRequested(list_->currentRow());
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
    if (options_button_ != nullptr) {
        options_button_->setText(tr("Options"));
        options_button_->setToolTip(tr("Open options for the selected signal."));
        options_button_->setStatusTip(tr("Configure the selected signal."));
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
        options_button_->setEnabled(false);
        remove_button_->setEnabled(false);
        return;
    }

    summary_label_->setText(tr("%1 signal(s) ready for editing").arg(library_->size()));
    const int plotted_count = static_cast<int>(std::count_if(
        visible_signal_indices_.begin(), visible_signal_indices_.end(),
        [this](int index) {
            return library_ != nullptr &&
                   index >= 0 &&
                   index < static_cast<int>(library_->size());
        }));

    const int row = list_->currentRow();
    if (row >= 0 && row < static_cast<int>(library_->size())) {
        const auto& signal = library_->at(static_cast<std::size_t>(row));
        const QString interpolation = signal.interpolation() == Signal::InterpolationMode::Step
            ? tr("step")
            : tr("linear");
        detail_label_->setText(
            tr("Selected: %1\n%2 samples, %3 interpolation, %4")
                .arg(QString::fromStdString(signal.name()))
                .arg(signal.size())
                .arg(interpolation)
                .arg(row == active_signal_index_ ? tr("active signal") : tr("inactive signal")));
        summary_label_->setText(
            tr("%1 signal(s) ready for editing | %2 plotted")
                .arg(library_->size())
                .arg(plotted_count));
        options_button_->setEnabled(true);
        remove_button_->setEnabled(true);
        return;
    }

    detail_label_->setText(tr("Select a signal to edit its shape, interpolation, and samples."));
    options_button_->setEnabled(false);
    remove_button_->setEnabled(false);
}

#undef tr

}  // namespace signal_editor::adapters::qt
