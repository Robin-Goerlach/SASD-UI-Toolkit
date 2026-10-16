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
