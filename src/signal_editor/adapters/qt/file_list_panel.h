#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

#include <vector>

class QLabel;
class QListWidget;

namespace myprj::signal_editor::adapters::qt {

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
     * @brief Returns the currently selected file index.
     * @return The active row or `-1` when nothing is selected.
     */
    [[nodiscard]] int current_index() const;

signals:
    void selectionChanged(int index);
    void removeRequested(int index);
    void detailsRequested(int index);

private slots:
    void onCurrentRowChanged(int row);
    void onCustomContextMenuRequested(const QPoint& pos);

private:
    std::vector<FileItem> files_;
    QLabel* summary_label_{nullptr};
    QLabel* detail_label_{nullptr};
    QListWidget* list_{nullptr};

    void refresh_summary(int current_row);
};

}  // namespace myprj::signal_editor::adapters::qt
