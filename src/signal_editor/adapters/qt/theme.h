#pragma once

class QApplication;

namespace myprj::signal_editor::adapters::qt {

// Apply the dark Fusion stylesheet used by the Signal Editor GUI.
// Centralised here so the look & feel can be changed in a single place.
void apply_dark_fusion_theme(QApplication& app);

}  // namespace myprj::signal_editor::adapters::qt
