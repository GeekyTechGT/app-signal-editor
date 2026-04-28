# Feature Branch File Map

## Scopo

Questo documento serve a pianificare una user story e un branch di lavoro per
ogni macro funzionalita' del prodotto. Per ogni macro funzionalita' indica:

- i requisiti software da coprire;
- i file sorgente principali da includere nel branch;
- i test e i dati di test da aggiornare;
- la documentazione da mantenere allineata.

La mappatura deriva dallo stato corrente di:

- `docs/specs/srs.md`
- `docs/product/prd.md`
- `docs/developer/source_file_guide.md`

## Regola Pratica Per Ogni Branch

Ogni branch di feature dovrebbe includere solo i file necessari alla
funzionalita', ma ci sono file trasversali che vanno considerati quasi sempre:

| Area | File |
|------|------|
| Requisiti | `docs/specs/srs.md`, `docs/product/prd.md` |
| Documentazione utente | `docs/user/user_manual.md` |
| Documentazione tecnica | `docs/developer/source_file_guide.md` e il documento developer specifico della feature |
| Changelog | `CHANGELOG.md` |
| Build sorgenti | `src/signal_editor/CMakeLists.txt` se si aggiungono o rimuovono `.cpp` |
| Build GUI | `apps/signal_editor/gui/CMakeLists.txt` se si modificano risorse, traduzioni o bootstrap GUI |
| Build test | `tests/03.unit_test/signal_editor/CMakeLists.txt` se si aggiungono test |
| Traduzioni | `resources/translations/signal_editor_en.ts`, `resources/translations/signal_editor_it.ts` se compaiono nuove stringhe visibili |

## Copertura Requisiti

Tutte le feature sotto coprono l'intero set di requisiti funzionali correnti
`FR-1` ... `FR-55` e non funzionali `NFR-1` ... `NFR-12`.

| Macro funzionalita' | Requisiti funzionali principali | Requisiti non funzionali principali |
|---------------------|----------------------------------|-------------------------------------|
| Import/export e persistence multi-formato | FR-1, FR-31, FR-41, FR-43, FR-44, FR-45, FR-46 | NFR-1, NFR-2, NFR-4, NFR-9, NFR-10 |
| Workspace multi-documento e file list | FR-2, FR-3, FR-4, FR-5, FR-6, FR-34, FR-37, FR-38, FR-48 | NFR-7, NFR-8, NFR-12 |
| Workbook XML/XLSX e selezione worksheet | FR-35, FR-36, FR-42, FR-46, FR-55 | NFR-2, NFR-8, NFR-9, NFR-10 |
| Lista segnali, visibilita' e segnale attivo | FR-7, FR-8, FR-9, FR-10, FR-11, FR-12, FR-13 | NFR-7, NFR-8 |
| Plot editing e navigazione | FR-11, FR-12, FR-13, FR-17, FR-18, FR-19, FR-20, FR-47 | NFR-7, NFR-11 |
| LOD rendering per segnali densi | FR-50, FR-51 | NFR-11 |
| Table editing | FR-14, FR-15, FR-16, FR-21 | NFR-7, NFR-11 |
| Interpolazione | FR-22, FR-23, FR-29, FR-30 | NFR-1, NFR-7, NFR-10 |
| Creazione segnali e segnali enumerati | FR-24, FR-25, FR-26, FR-27, FR-28, FR-31, FR-32, FR-33 | NFR-1, NFR-7, NFR-10 |
| Undo e stato dirty documento | FR-39, FR-40 | NFR-8 |
| Async loading e diagnostica import | FR-52, FR-53, FR-54, FR-55 | NFR-12 |
| Settings, tema, branding, lingua e packaging GUI | FR-49 | NFR-3, NFR-5, NFR-6, NFR-9 |
| Architettura, API e qualita' progetto | Supporto trasversale a tutti gli FR | NFR-1, NFR-2, NFR-3, NFR-4, NFR-5, NFR-6, NFR-9, NFR-10 |

## 1. Import/export E Persistence Multi-Formato

User story: come utente voglio caricare e salvare segnali in CSV, TSV/TXT,
JSON, SpreadsheetML XML e XLSX, preservando dati, interpolazione e mapping
enumerati quando il formato lo supporta.

Requisiti: `FR-1`, `FR-31`, `FR-41`, `FR-43`, `FR-44`, `FR-45`, `FR-46`,
`NFR-1`, `NFR-2`, `NFR-4`, `NFR-9`, `NFR-10`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Port | `src/signal_editor/ports/signal_repository.h` |
| Domain condiviso | `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp`, `src/signal_editor/core/domain/signal_library.h`, `src/signal_editor/core/domain/signal_library.cpp`, `src/signal_editor/core/domain/sample_point.h`, `src/signal_editor/core/domain/result.h` |
| Use case | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| Filesystem comune | `src/signal_editor/adapters/filesystem/workbook_model.h`, `src/signal_editor/adapters/filesystem/tabular_signal_rows.h`, `src/signal_editor/adapters/filesystem/tabular_signal_rows.cpp`, `src/signal_editor/adapters/filesystem/signal_file_repository.h`, `src/signal_editor/adapters/filesystem/signal_file_repository.cpp` |
| CSV/TSV/JSON/XML/XLSX | `src/signal_editor/adapters/filesystem/csv_signal_repository.h`, `src/signal_editor/adapters/filesystem/csv_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/delimited_signal_repository.h`, `src/signal_editor/adapters/filesystem/delimited_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/json_signal_repository.h`, `src/signal_editor/adapters/filesystem/json_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h`, `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.h`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp` |
| API | `src/signal_editor/api/signal_editor_api.h`, `src/signal_editor/api/signal_editor_api.cpp` |
| Build | `src/signal_editor/CMakeLists.txt` |
| Test | `tests/03.unit_test/signal_editor/test_csv_repository.cpp`, `tests/03.unit_test/signal_editor/test_delimited_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_json_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_spreadsheet_xml_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_xlsx_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_signal_file_repository.cpp`, `tests/03.unit_test/signal_editor/CMakeLists.txt` |
| Fixture | `tests/01.data/sample_signals.csv`, `tests/01.data/sample_time_column_valid.csv`, `tests/01.data/sample_time_column_invalid.csv`, `tests/01.data/sample_enumerated_signals.csv`, `tests/01.data/sample_inline_enumerated_signals.csv`, `tests/01.data/sample_enumerated_signals.json`, `tests/01.data/sample_enumerated_signals.xml`, `tests/01.data/sample_automotive_signals.csv`, `tests/01.data/sample_automotive_signals_2_tab.csv`, `tests/01.data/sample_automotive_signals_2_tab.xlsx`, `tests/01.data/sample_automotive_signals_2_tab_new.xlsx` |
| Docs | `docs/developer/filesystem_and_persistence.md`, `docs/developer/workbook_and_xlsx.md`, `docs/developer/source_file_guide.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 2. Workspace Multi-Documento E File List

User story: come utente voglio caricare piu' file, selezionarli senza cambiare
automaticamente il documento aperto, aprire esplicitamente un file, ricaricarlo
da disco e rimuovere file in batch.

Requisiti: `FR-2`, `FR-3`, `FR-4`, `FR-5`, `FR-6`, `FR-34`, `FR-37`, `FR-38`,
`FR-48`, `NFR-7`, `NFR-8`, `NFR-12`.

File da includere nel branch:

| Tipo | File |
|------|------|
| GUI orchestration | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| File list | `src/signal_editor/adapters/qt/file_list_panel.h`, `src/signal_editor/adapters/qt/file_list_panel.cpp` |
| Service/persistence | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp`, `src/signal_editor/adapters/filesystem/signal_file_repository.h`, `src/signal_editor/adapters/filesystem/signal_file_repository.cpp` |
| UI support | `src/signal_editor/adapters/qt/constants.hpp`, `src/signal_editor/adapters/qt/icon_theme.h`, `src/signal_editor/adapters/qt/icon_theme.cpp` |
| App wiring | `apps/signal_editor/gui/main.cpp`, `apps/signal_editor/gui/bootstrap.h`, `apps/signal_editor/gui/bootstrap.cpp`, `apps/signal_editor/gui/resources.qrc` |
| Resources | `resources/img/open.svg`, `resources/img/new.svg`, `resources/img/remove.svg`, `resources/img/rename.svg`, `resources/img/settings.svg` |
| Test | `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp`, `tests/03.unit_test/signal_editor/test_signal_file_repository.cpp` |
| Docs | `docs/developer/workspace_and_selection.md`, `docs/developer/filesystem_and_persistence.md`, `docs/developer/source_file_guide.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 3. Workbook XML/XLSX E Selezione Worksheet

User story: come utente voglio aprire file workbook, vedere i fogli disponibili
e scegliere il foglio attivo senza perdere stato, mapping enumerati o contesto
del documento.

Requisiti: `FR-35`, `FR-36`, `FR-42`, `FR-46`, `FR-55`, `NFR-2`, `NFR-8`,
`NFR-9`, `NFR-10`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Workbook model | `src/signal_editor/adapters/filesystem/workbook_model.h` |
| Repository workbook | `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h`, `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.h`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/signal_file_repository.h`, `src/signal_editor/adapters/filesystem/signal_file_repository.cpp` |
| Tabular parser | `src/signal_editor/adapters/filesystem/tabular_signal_rows.h`, `src/signal_editor/adapters/filesystem/tabular_signal_rows.cpp` |
| GUI | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp`, `src/signal_editor/adapters/qt/file_list_panel.h`, `src/signal_editor/adapters/qt/file_list_panel.cpp`, `src/signal_editor/adapters/qt/signal_list_panel.h`, `src/signal_editor/adapters/qt/signal_list_panel.cpp`, `src/signal_editor/adapters/qt/signal_plot_widget.h`, `src/signal_editor/adapters/qt/signal_plot_widget.cpp`, `src/signal_editor/adapters/qt/signal_table_panel.h`, `src/signal_editor/adapters/qt/signal_table_panel.cpp` |
| Test | `tests/03.unit_test/signal_editor/test_spreadsheet_xml_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_xlsx_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_signal_file_repository.cpp` |
| Fixture | `tests/01.data/sample_automotive_signals_2_tab.xlsx`, `tests/01.data/sample_automotive_signals_2_tab_new.xlsx`, `tests/01.data/sample_automotive_signals_2_tab.csv`, `tests/01.data/sample_enumerated_signals.xml` |
| Docs | `docs/developer/workbook_and_xlsx.md`, `docs/developer/workspace_and_selection.md`, `docs/developer/filesystem_and_persistence.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 4. Lista Segnali, Visibilita' E Segnale Attivo

User story: come utente voglio vedere i segnali del documento aperto,
controllarne la visibilita' e scegliere un segnale attivo modificabile senza
confondere selezione, visibilita' e stato di editing.

Requisiti: `FR-7`, `FR-8`, `FR-9`, `FR-10`, `FR-11`, `FR-12`, `FR-13`,
`NFR-7`, `NFR-8`.

File da includere nel branch:

| Tipo | File |
|------|------|
| GUI orchestration | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| Signal list | `src/signal_editor/adapters/qt/signal_list_panel.h`, `src/signal_editor/adapters/qt/signal_list_panel.cpp` |
| Plot/table bindings | `src/signal_editor/adapters/qt/signal_plot_widget.h`, `src/signal_editor/adapters/qt/signal_plot_widget.cpp`, `src/signal_editor/adapters/qt/signal_table_panel.h`, `src/signal_editor/adapters/qt/signal_table_panel.cpp` |
| Domain/use case | `src/signal_editor/core/domain/signal_library.h`, `src/signal_editor/core/domain/signal_library.cpp`, `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| UI support | `src/signal_editor/adapters/qt/theme.h`, `src/signal_editor/adapters/qt/theme.cpp`, `src/signal_editor/adapters/qt/constants.hpp` |
| Test | `tests/03.unit_test/signal_editor/test_signal_library.cpp`, `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp` |
| Docs | `docs/developer/workspace_and_selection.md`, `docs/developer/plot_subsystem.md`, `docs/developer/source_file_guide.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 5. Plot Editing, Navigazione Ed Export Screenshot

User story: come utente voglio modificare un segnale dal grafico con drag,
inserimento, rimozione, brushing gaussiano, zoom, pan e salvataggio screenshot.

Requisiti: `FR-11`, `FR-12`, `FR-13`, `FR-17`, `FR-18`, `FR-19`, `FR-20`,
`FR-47`, `NFR-7`, `NFR-11`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Plot | `src/signal_editor/adapters/qt/signal_plot_widget.h`, `src/signal_editor/adapters/qt/signal_plot_widget.cpp` |
| GUI orchestration | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| Service | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| Domain | `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp`, `src/signal_editor/core/domain/sample_point.h` |
| UI support | `src/signal_editor/adapters/qt/constants.hpp`, `src/signal_editor/adapters/qt/icon_theme.h`, `src/signal_editor/adapters/qt/icon_theme.cpp`, `src/signal_editor/adapters/qt/theme.h`, `src/signal_editor/adapters/qt/theme.cpp` |
| Resources | `resources/img/zoom-in.svg`, `resources/img/zoom-out.svg`, `resources/img/zoom-fit.svg`, `resources/img/zoom-selection.svg`, `resources/img/pan.svg`, `resources/img/save.svg`, `resources/img/screen-share.svg` |
| Test | `tests/03.unit_test/signal_editor/test_signal.cpp`, `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp` |
| Docs | `docs/developer/plot_subsystem.md`, `docs/developer/workspace_and_selection.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 6. LOD Rendering Per Segnali Densi

User story: come utente voglio visualizzare segnali con milioni di campioni
senza bloccare la UI, preservando spike e glitch anche quando sono zoomato out.

Requisiti: `FR-50`, `FR-51`, `NFR-11`.

File da includere nel branch:

| Tipo | File |
|------|------|
| LOD | `src/signal_editor/adapters/qt/signal_lod_pyramid.h`, `src/signal_editor/adapters/qt/signal_lod_pyramid.cpp` |
| Plot integration | `src/signal_editor/adapters/qt/signal_plot_widget.h`, `src/signal_editor/adapters/qt/signal_plot_widget.cpp` |
| Workspace/cache invalidation | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp`, `src/signal_editor/adapters/qt/signal_list_panel.h`, `src/signal_editor/adapters/qt/signal_list_panel.cpp` |
| Domain input | `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp`, `src/signal_editor/core/domain/sample_point.h` |
| Table safeguard | `src/signal_editor/adapters/qt/signal_table_panel.h`, `src/signal_editor/adapters/qt/signal_table_panel.cpp` |
| Build | `src/signal_editor/CMakeLists.txt`, `tests/03.unit_test/signal_editor/CMakeLists.txt` |
| Test | `tests/03.unit_test/signal_editor/test_signal_lod_pyramid.cpp` |
| Fixture/script | `scripts/generate_lod_sample.py`, `tests/01.data/generated/sample_lod_dense_120s_dt_100us.csv`, `tests/01.data/generated/lod_levels/sample_lod_raw_10000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_2_20000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_4_40000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_8_80000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_16_160000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_32_320000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_64_640000_samples.csv`, `tests/01.data/generated/lod_levels/sample_lod_bucket_128_1280000_samples.csv` |
| Docs | `docs/developer/lod_rendering.md`, `docs/developer/plot_subsystem.md`, `docs/developer/workspace_and_selection.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 7. Table Editing

User story: come utente voglio ispezionare e modificare campioni in tabella,
vedere una colonna tempo condivisa, filtrare le righe e modificare solo il
segnale attivo.

Requisiti: `FR-14`, `FR-15`, `FR-16`, `FR-21`, `NFR-7`, `NFR-11`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Table | `src/signal_editor/adapters/qt/signal_table_panel.h`, `src/signal_editor/adapters/qt/signal_table_panel.cpp` |
| GUI orchestration | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| Signal list/visibility | `src/signal_editor/adapters/qt/signal_list_panel.h`, `src/signal_editor/adapters/qt/signal_list_panel.cpp` |
| Service | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| Domain | `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp`, `src/signal_editor/core/domain/signal_library.h`, `src/signal_editor/core/domain/signal_library.cpp` |
| Test | `tests/03.unit_test/signal_editor/test_signal.cpp`, `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp` |
| Docs | `docs/developer/workspace_and_selection.md`, `docs/developer/source_file_guide.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 8. Interpolazione

User story: come utente voglio controllare l'interpolazione dei segnali visibili
da un selettore condiviso e mantenere i segnali enumerati sempre coerenti con
la semantica step.

Requisiti: `FR-22`, `FR-23`, `FR-29`, `FR-30`, `NFR-1`, `NFR-7`, `NFR-10`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Domain | `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp` |
| Service | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| Persistence metadata | `src/signal_editor/adapters/filesystem/csv_signal_repository.h`, `src/signal_editor/adapters/filesystem/csv_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/delimited_signal_repository.h`, `src/signal_editor/adapters/filesystem/delimited_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/json_signal_repository.h`, `src/signal_editor/adapters/filesystem/json_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h`, `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.h`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp` |
| GUI controls | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp`, `src/signal_editor/adapters/qt/signal_plot_widget.h`, `src/signal_editor/adapters/qt/signal_plot_widget.cpp`, `src/signal_editor/adapters/qt/signal_table_panel.h`, `src/signal_editor/adapters/qt/signal_table_panel.cpp` |
| Test | `tests/03.unit_test/signal_editor/test_signal.cpp`, `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp`, `tests/03.unit_test/signal_editor/test_csv_repository.cpp`, `tests/03.unit_test/signal_editor/test_delimited_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_json_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_xlsx_signal_repository.cpp` |
| Docs | `docs/developer/plot_subsystem.md`, `docs/developer/filesystem_and_persistence.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 9. Creazione Segnali E Segnali Enumerati

User story: come utente voglio creare segnali da template numerici o enumerati,
rinominarli, rimuoverli, ispezionarli e modificare mapping label/valore per i
canali di stato.

Requisiti: `FR-24`, `FR-25`, `FR-26`, `FR-27`, `FR-28`, `FR-31`, `FR-32`,
`FR-33`, `NFR-1`, `NFR-7`, `NFR-10`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Domain | `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp`, `src/signal_editor/core/domain/signal_library.h`, `src/signal_editor/core/domain/signal_library.cpp`, `src/signal_editor/core/domain/sample_point.h` |
| Service | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| Parser/persistence enum | `src/signal_editor/adapters/filesystem/tabular_signal_rows.h`, `src/signal_editor/adapters/filesystem/tabular_signal_rows.cpp`, `src/signal_editor/adapters/filesystem/csv_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/delimited_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/json_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp` |
| GUI | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp`, `src/signal_editor/adapters/qt/signal_list_panel.h`, `src/signal_editor/adapters/qt/signal_list_panel.cpp`, `src/signal_editor/adapters/qt/signal_plot_widget.h`, `src/signal_editor/adapters/qt/signal_plot_widget.cpp`, `src/signal_editor/adapters/qt/signal_table_panel.h`, `src/signal_editor/adapters/qt/signal_table_panel.cpp` |
| Resources | `resources/img/new.svg`, `resources/img/rename.svg`, `resources/img/remove.svg`, `resources/img/settings.svg` |
| Test | `tests/03.unit_test/signal_editor/test_signal.cpp`, `tests/03.unit_test/signal_editor/test_signal_library.cpp`, `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp`, `tests/03.unit_test/signal_editor/test_csv_repository.cpp`, `tests/03.unit_test/signal_editor/test_json_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_xlsx_signal_repository.cpp` |
| Fixture | `tests/01.data/sample_enumerated_signals.csv`, `tests/01.data/sample_inline_enumerated_signals.csv`, `tests/01.data/sample_enumerated_signals.json`, `tests/01.data/sample_enumerated_signals.xml` |
| Docs | `docs/developer/filesystem_and_persistence.md`, `docs/developer/workbook_and_xlsx.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 10. Undo E Stato Dirty Documento

User story: come utente voglio annullare modifiche nel documento attivo senza
contaminare gli altri documenti caricati e voglio che lo stato modificato sia
coerente con salvataggio, reload e cambio documento.

Requisiti: `FR-39`, `FR-40`, `NFR-8`.

File da includere nel branch:

| Tipo | File |
|------|------|
| GUI/state | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| File list feedback | `src/signal_editor/adapters/qt/file_list_panel.h`, `src/signal_editor/adapters/qt/file_list_panel.cpp` |
| Service/domain | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp`, `src/signal_editor/core/domain/signal_library.h`, `src/signal_editor/core/domain/signal_library.cpp`, `src/signal_editor/core/domain/signal.h`, `src/signal_editor/core/domain/signal.cpp` |
| Resources | `resources/img/undo.svg`, `resources/img/save.svg` |
| Test | `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp`, `tests/03.unit_test/signal_editor/test_signal_library.cpp` |
| Docs | `docs/developer/workspace_and_selection.md`, `docs/developer/filesystem_and_persistence.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 11. Async Loading E Diagnostica Import

User story: come utente voglio aprire file grandi senza bloccare finestra,
minimize/resize/restore, e ricevere messaggi chiari quando un file importato e'
malformato.

Requisiti: `FR-52`, `FR-53`, `FR-54`, `FR-55`, `NFR-12`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Async GUI | `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| Persistence errors | `src/signal_editor/adapters/filesystem/tabular_signal_rows.h`, `src/signal_editor/adapters/filesystem/tabular_signal_rows.cpp`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.h`, `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp`, `src/signal_editor/adapters/filesystem/signal_file_repository.h`, `src/signal_editor/adapters/filesystem/signal_file_repository.cpp` |
| Service | `src/signal_editor/core/usecases/signal_editor_service.h`, `src/signal_editor/core/usecases/signal_editor_service.cpp` |
| Test | `tests/03.unit_test/signal_editor/test_csv_repository.cpp`, `tests/03.unit_test/signal_editor/test_delimited_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_spreadsheet_xml_signal_repository.cpp`, `tests/03.unit_test/signal_editor/test_xlsx_signal_repository.cpp` |
| Fixture | `tests/01.data/sample_time_column_invalid.csv`, `tests/01.data/sample_time_column_valid.csv`, `tests/01.data/sample_automotive_signals_2_tab.xlsx` |
| Docs | `docs/developer/filesystem_and_persistence.md`, `docs/developer/workspace_and_selection.md`, `docs/developer/workbook_and_xlsx.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 12. Settings, Tema, Branding, Lingua E Packaging GUI

User story: come utente voglio un'applicazione desktop coerente, con tema,
icone, lingua e preferenze persistenti per versione, senza conflitti tra release.

Requisiti: `FR-49`, `NFR-3`, `NFR-5`, `NFR-6`, `NFR-9`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Qt settings/theme | `src/signal_editor/adapters/qt/constants.hpp`, `src/signal_editor/adapters/qt/theme.h`, `src/signal_editor/adapters/qt/theme.cpp`, `src/signal_editor/adapters/qt/branding.h`, `src/signal_editor/adapters/qt/branding.cpp`, `src/signal_editor/adapters/qt/icon_theme.h`, `src/signal_editor/adapters/qt/icon_theme.cpp`, `src/signal_editor/adapters/qt/main_window.h`, `src/signal_editor/adapters/qt/main_window.cpp` |
| App bootstrap | `apps/signal_editor/gui/main.cpp`, `apps/signal_editor/gui/bootstrap.h`, `apps/signal_editor/gui/bootstrap.cpp`, `apps/signal_editor/gui/resources.qrc`, `apps/signal_editor/gui/CMakeLists.txt` |
| Resources | `resources/img/app_logo.svg`, `resources/img/app_logo.png`, `resources/img/app_icon.png`, `resources/img/app_icon.ico`, `resources/img/*.svg`, `resources/translations/signal_editor_en.ts`, `resources/translations/signal_editor_it.ts`, `external/res-qt-themes/themes/dark.qss`, `external/res-qt-themes/themes/light.qss`, `external/res-qt-themes/themes/splash_screen.qss` |
| Project/build/package | `project.json`, `project.metadata.json`, `VERSION`, `CMakeLists.txt`, `CMakePresets.json`, `cmake/ProjectOptions.cmake`, `cmake/ExternalLibraries.cmake`, `scripts/package_customer.py`, `scripts/package_deploy_linux.sh`, `scripts/project_manager.py` |
| Docs | `docs/guidelines/semantic_versioning.md`, `docs/guidelines/naming_conventions.md`, `docs/guidelines/template_customization.md`, `docs/developer/onboarding.md`, `docs/developer/source_file_guide.md`, `docs/specs/srs.md`, `docs/product/prd.md`, `docs/user/user_manual.md`, `CHANGELOG.md` |

## 13. Architettura, API E Qualita' Progetto

User story: come sviluppatore voglio mantenere separati dominio, use case, port,
adapter filesystem e adapter Qt, cosi' ogni feature resta testabile e il core
rimane indipendente dalla GUI.

Requisiti: supporto trasversale a tutti gli `FR`; `NFR-1`, `NFR-2`, `NFR-3`,
`NFR-4`, `NFR-5`, `NFR-6`, `NFR-9`, `NFR-10`.

File da includere nel branch:

| Tipo | File |
|------|------|
| Architettura | `docs/architecture/architecture_overview.md`, `docs/architecture/hexagonal_skeleton_structure.md`, `docs/architecture/c4_context.md`, `docs/architecture/c4_container.md`, `docs/architecture/c4_component.md` |
| API/core | `src/signal_editor/api/signal_editor_api.h`, `src/signal_editor/api/signal_editor_api.cpp`, `src/signal_editor/ports/signal_repository.h`, `src/signal_editor/core/domain/result.h` |
| Build | `CMakeLists.txt`, `src/signal_editor/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/03.unit_test/signal_editor/CMakeLists.txt`, `cmake/ProjectOptions.cmake`, `cmake/ExternalLibraries.cmake`, `CMakePresets.json` |
| Project metadata | `project.json`, `project.metadata.json`, `VERSION`, `include/signal_editor/version.h.in`, `include/signal_editor/signal_editor_version.h.in` |
| Pipeline/template | `tests/05.pipeline_test/scaffold_apps/test_scaffold_apps.py`, `tests/05.pipeline_test/customize_project/test_customize_project.py`, `scripts/scaffold_apps.py`, `scripts/customize_project.py`, `requirements.txt`, `requirements/requirements.txt` |
| Governance/docs | `README.md`, `CONTRIBUTING.md`, `GOVERNANCE.md`, `LICENSE.md`, `docs/developer/README.md`, `docs/developer/onboarding.md`, `docs/developer/source_file_guide.md`, `docs/guidelines/naming_conventions.md`, `docs/guidelines/semantic_versioning.md`, `docs/guidelines/template_customization.md`, `CHANGELOG.md` |

## Checklist Di Chiusura Per Ogni Feature Branch

Prima di chiudere una user story, verificare:

- i requisiti funzionali e non funzionali collegati sono aggiornati in
  `docs/specs/srs.md`;
- `docs/product/prd.md` e `docs/user/user_manual.md` riflettono il comportamento
  utente;
- il documento developer specifico e `docs/developer/source_file_guide.md` sono
  coerenti con i file modificati;
- i test automatici coprono dominio, use case o adapter coinvolti;
- eventuali fixture nuove sono piccole o motivate;
- `CHANGELOG.md` descrive il cambiamento utente o tecnico rilevante;
- se sono stati aggiunti `.cpp`, sono stati aggiornati i `CMakeLists.txt`
  corretti;
- se sono state aggiunte stringhe GUI, sono state aggiornate le traduzioni.
