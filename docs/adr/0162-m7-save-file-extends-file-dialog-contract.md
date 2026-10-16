# ADR 0162: M7 Save File extends the validated file-dialog contract

## Status

Accepted

## Context

The Win32, AppKit and GTK Open File consumers validated one small portable file-dialog contract.
Applications also need to choose a destination for content that may not exist yet. The native APIs
already provide the expected save UX, including overwrite confirmation, so this slice must extend the
contract without introducing a second dialog architecture or filesystem side effects.

## Decision

Core adds only:

`SaveFileDialogRequest { std::string title{}; }` and
`FileDialogService::requestSaveFile(SaveFileDialogRequest)`.

Save reuses `FileDialogResultEvent`. `accepted` carries one non-empty
`std::filesystem::path`; `cancelled` and `failed` are pathless. Open and Save share the same
non-zero monotonic request-ID sequence and the same one-pending-operation boundary. Pending ends when
exactly one completion event has been queued, and `EventQueue` remains the asynchronous boundary;
there are no public callbacks, futures, promises or coroutines.

An accepted Save path is only a destination selected by the native dialog. It may already exist or
may not exist. The toolkit does not check existence, canonicalize or normalize the path, create,
open, truncate or write the file, and does not reserve the path. The application owns the subsequent
write policy. Native overwrite confirmation remains owned by the platform dialog.

The private consumers are:

- Win32 `IFileSaveDialog` with filesystem/path/no-working-directory options and native
  `FOS_OVERWRITEPROMPT`, without `FOS_FILEMUSTEXIST`.
- AppKit `NSSavePanel` as an asynchronous sheet owned by the existing `NSWindow`.
- GTK `gtk_file_dialog_save()`/`gtk_file_dialog_save_finish()` on GTK 4.10 and newer, with the
  existing asynchronous `GtkFileChooserNative` `GTK_FILE_CHOOSER_ACTION_SAVE` fallback on older
  supported GTK4 versions. GTK4's native save chooser owns overwrite confirmation.

Each backend keeps its existing private lifetime, generation, exactly-once and shutdown protections;
Open and Save cannot own simultaneous native operations. Folder selection, filters, suggested names,
default extensions and multiple selection remain deferred. No public native-dialog hierarchy is
needed.

## Implementation and validation record

This decision was implemented as the M7 Save File vertical slice across Core, Win32, AppKit and
GTK4. The change also added deterministic backend tests, updated the native file-dialog examples,
and aligned the backend documentation, roadmap and changelog. The examples only display the
accepted target path; they do not create or write a file.

The implementation was delivered in PR #47 from `codex/m7-save-file-dialog`. The task branch was
based on `03e8ea5b99ca8e2beade9525091640ff45056b39`, and the final implementation commit was
`94797e7`. A portability warning in the Win32 Unicode test was corrected before the final CI run.
PR #47 was merged into `main` as `c8394ba383d43d9195f12242c0d0e3551a9fa418`; the task branch was
then removed locally and remotely.

The exact final PR head passed the Linux GCC, Linux Clang, Clang ASan/UBSan, SDL3 adapter, Windows
MSVC, macOS AppleClang, Linux Release, Windows Release, macOS Release, GTK Native and AppKit Native
checks. The exact merge commit subsequently passed Main CI, GTK Native CI and AppKit Native CI.
A fresh strict GCC build and the configured local CTest suite also passed (4/4 tests). GTK was not
installed in the WSL environment, and Win32/AppKit production builds were therefore validated by
their authoritative CI jobs rather than locally. No visible native smoke test was run, so this ADR
does not claim manual platform verification. The historical visible M3 macOS/Cocoa gate remains a
separate status and is not closed by this slice. No release or tag was created.

PR #30 and PR #32 were inspected only as read-only repository context and remain unchanged. The
next M7 work remains explicitly deferred: folder selection, filters, suggested filenames, default
extensions and multiple selection.

## Deutsch

## Status

Akzeptiert

## Kontext

Die Win32-, AppKit- und GTK-Open-File-Consumer haben einen kleinen portablen File-Dialog-Vertrag
validiert. Anwendungen müssen zusätzlich ein Ziel für Inhalte auswählen können, das noch nicht
existiert. Die nativen APIs liefern bereits die erwartete Save-Oberfläche einschließlich
Überschreibbestätigung. Dieser Slice erweitert deshalb den bestehenden Vertrag ohne eine zweite
Dialogarchitektur oder Dateisystem-Seiteneffekte.

## Entscheidung

Core ergänzt nur `SaveFileDialogRequest { std::string title{}; }` und
`FileDialogService::requestSaveFile(SaveFileDialogRequest)`. Save verwendet weiterhin
`FileDialogResultEvent`: `accepted` enthält genau einen nicht-leeren
`std::filesystem::path`, `cancelled` und `failed` bleiben pfadlos.

Open und Save teilen dieselbe monotone, nicht-null Request-ID-Sequenz und dieselbe Grenze von höchstens
einer Pending-Operation. Pending endet beim Queueing genau eines Completion-Events. Die `EventQueue`
bleibt die asynchrone Grenze; öffentliche Callbacks, Futures, Promises und Coroutines werden nicht
eingeführt.

Ein akzeptierter Save-Pfad ist ausschließlich ein vom nativen Dialog gewähltes Ziel. Er darf existieren
oder noch nicht existieren. Das Toolkit prüft keine Existenz, führt keine Canonicalization oder
Normalisierung durch und erzeugt, öffnet, truncatet oder schreibt die Datei nicht. Die Anwendung
entscheidet selbst über den späteren Schreibvorgang. Die native Plattform bleibt für die
Überschreibbestätigung zuständig.

Privat verwenden Win32 `IFileSaveDialog` ohne `FOS_FILEMUSTEXIST`, AppKit `NSSavePanel` als
asynchrones Sheet am bestehenden `NSWindow` und GTK ab 4.10 `gtk_file_dialog_save()`/
`save_finish()` sowie bei älteren unterstützten GTK4-Versionen den bestehenden asynchronen
`GtkFileChooserNative`-Fallback mit `GTK_FILE_CHOOSER_ACTION_SAVE`. Native Lifetime-, Generation-,
Exactly-once- und Shutdown-Schutzmechanismen bleiben backendlokal gemeinsam mit Open. Ordner,
Filter, vorgeschlagene Namen, Standardendungen und Mehrfachauswahl bleiben zurückgestellt; eine
öffentliche Native-Dialog-Hierarchie ist nicht erforderlich.

## Umsetzungs- und Validierungsnachtrag

Diese Entscheidung wurde als vollständiger M7-Save-File-Vertical-Slice für Core, Win32, AppKit und
GTK4 umgesetzt. Dazu kamen deterministische Backend-Tests, Erweiterungen der nativen File-Dialog-
Beispiele sowie die Aktualisierung von Backend-Dokumentation, Roadmap und Changelog. Die Beispiele
zeigen den akzeptierten Zielpfad nur an; sie erzeugen und schreiben keine Datei.

Die Umsetzung wurde als PR #47 aus `codex/m7-save-file-dialog` geliefert. Ausgangspunkt war
`03e8ea5b99ca8e2beade9525091640ff45056b39`; der finale Implementierungs-Commit war `94797e7`. Vor
dem finalen CI-Lauf wurde eine MSVC-Portabilitätswarnung im Win32-Unicode-Test korrigiert. PR #47
wurde als `c8394ba383d43d9195f12242c0d0e3551a9fa418` nach `main` gemergt; der Task-Branch wurde
anschließend lokal und remote entfernt.

Der exakte finale PR-Head war in Linux GCC, Linux Clang, Clang ASan/UBSan, SDL3-Adapter, Windows
MSVC, macOS AppleClang, Linux Release, Windows Release, macOS Release, GTK Native und AppKit Native
grün. Der exakte Merge-Commit war anschließend in Main CI, GTK Native CI und AppKit Native CI grün.
Ein frischer strikter GCC-Build und die konfigurierte lokale CTest-Suite waren ebenfalls erfolgreich
(4/4 Tests). GTK war in der WSL-Umgebung nicht installiert; Win32- und AppKit-Produktions-Builds
wurden daher durch die maßgeblichen CI-Jobs statt lokal validiert. Es wurde kein sichtbarer nativer
Smoke-Test ausgeführt; manuelle Plattformvalidierung wird hier deshalb nicht behauptet. Das
historische sichtbare M3-macOS/Cocoa-Gate bleibt ein unabhängiger Status und wird durch diesen Slice
nicht geschlossen. Es wurde kein Release und kein Tag erstellt.

PR #30 und PR #32 wurden nur read-only als Repository-Kontext geprüft und bleiben unverändert. Als
nächste M7-Schritte bleiben Ordnerauswahl, Filter, vorgeschlagene Dateinamen, Standardendungen und
Mehrfachauswahl ausdrücklich zurückgestellt.
