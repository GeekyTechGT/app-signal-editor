#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

#include <vector>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QEvent;

namespace signal_editor::adapters::qt {

/**
 * @file
 * @brief File sidebar widget used by the main Signal Editor workspace.
 */

/**
 * @brief Workspace-aware sidebar that lists every loaded CSV document.
 *
 * The panel is intentionally presentation-only: it renders file metadata,
 * tracks the current selection, and emits semantic Qt signals when the user
 * wants to switch documents or open file-level actions from the context menu.
 */
class FileListPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Lightweight view model consumed by the file list.
     */
    struct FileItem {
        QString display_name;
        QString full_path;
        bool dirty{false};
    };

    /**
     * @brief Builds the file panel and its summary labels.
     * @param parent Optional owning widget supplied by Qt.
     */
    explicit FileListPanel(QWidget* parent = nullptr);

    /**
     * @brief Replaces the currently displayed workspace file collection.
     * @param files Ordered file descriptors to expose to the user.
     */
    void set_files(const std::vector<FileItem>& files);

    /**
     * @brief Programmatically selects a workspace file.
     * @param index Zero-based file index.
     */
    void select(int index);

    /**
     * @brief Enters in-place rename mode for the active workspace item.
     */
    void begin_rename_current();

    /**
     * @brief Returns the currently selected file index.
     * @return The active row or `-1` when nothing is selected.
     */
    [[nodiscard]] int current_index() const;

signals:
    /** @brief Emitted when the active workspace file changes. */
    void selectionChanged(int index);
    /** @brief Emitted when the user confirms an in-place file rename. */
    void renameRequested(int index, const QString& new_name);
    /** @brief Emitted when the user asks to remove a file from the workspace. */
    void removeRequested(int index);
    /** @brief Emitted when the user requests file metadata/details. */
    void detailsRequested(int index);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onCurrentRowChanged(int row);
    void onItemChanged(QListWidgetItem* item);
    void onCustomContextMenuRequested(const QPoint& pos);

private:
    std::vector<FileItem> files_;
    QLabel* title_label_{nullptr};
    QLabel* summary_label_{nullptr};
    QLabel* detail_label_{nullptr};
    QListWidget* list_{nullptr};
    bool suppress_item_changed_{false};

    /** @brief Retranslates labels, tooltips, and context-menu copy. */
    void retranslate_ui();
    /** @brief Reapplies title/body typography derived from the current theme. */
    void refresh_typography();
    /** @brief Recomputes the selection summary shown above the list. */
    void refresh_summary(int current_row);
};

}  // namespace signal_editor::adapters::qt
