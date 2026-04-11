#pragma once

class QApplication;

namespace myprj::signal_editor::adapters::qt {

/**
 * @brief Applies the house visual language used by the Signal Editor GUI.
 *
 * The function centralises palette and stylesheet setup so that every window,
 * panel, list, and editor shares the same visual direction.
 *
 * @param app QApplication instance that will receive the palette and style.
 */
void apply_dark_fusion_theme(QApplication& app);

}  // namespace myprj::signal_editor::adapters::qt
