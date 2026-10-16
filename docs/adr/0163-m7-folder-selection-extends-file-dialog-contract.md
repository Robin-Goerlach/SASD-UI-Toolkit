# ADR 0163: M7 Folder Selection Extends the FileDialogService Contract

## Status

Accepted

## Context

M7 already provides one portable `FileDialogService` for Open File and Save File. The next small
slice needs to select exactly one existing local filesystem directory without introducing a second
service or a platform-shaped public dialog hierarchy.

## Decision

Add `SelectFolderDialogRequest` and `FileDialogService::requestSelectFolder()` while reusing
`FileDialogResultEvent`. An accepted result contains exactly one non-empty local
`std::filesystem::path`; cancellation and failure contain no path. The native picker, rather than a
racy second filesystem query, guarantees that the selected item is an existing directory. The
toolkit does not create, remove, rename or write anything and does not change the process working
directory.

Open, Save and Folder share one monotone non-zero request-ID sequence, one pending-operation
boundary, the existing `EventQueue` completion boundary, and the existing exactly-once,
generation, UI-thread and shutdown rules. No new capability is needed: `file_dialogs` covers all
three operations.

The Win32 consumer uses the existing owner `HWND` and `IFileOpenDialog` with `FOS_PICKFOLDERS`,
`FOS_FORCEFILESYSTEM`, `FOS_PATHMUSTEXIST` and `FOS_NOCHANGEDIR`; it does not add
`FOS_FILEMUSTEXIST`. AppKit uses an asynchronously sheet-owned `NSOpenPanel` with the existing
`NSWindow`, `canChooseDirectories = YES`, `canChooseFiles = NO` and
`allowsMultipleSelection = NO`. GTK 4.10 and newer uses `gtk_file_dialog_select_folder()` /
`select_folder_finish()`; older supported GTK4 headers use asynchronous
`GtkFileChooserNative` with `GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER`. Each backend copies its native
filesystem representation directly into `std::filesystem::path` and rejects non-local or empty
native results.

## Consequences

The public contract remains small and backend-neutral. Filters, richer options, suggested names,
initial directories and multiselect remain deferred. Private native invoker seams are extended only
where required for deterministic tests. No `FolderDialogService`, `NativeFolderDialog`, public native
hierarchy or new capability is introduced.

## Deutsch

## Status

Angenommen

## Kontext

M7 stellt bereits einen portablen `FileDialogService` für Open File und Save File bereit. Der nächste
kleine Slice soll genau ein bestehendes lokales Dateisystemverzeichnis auswählen, ohne einen zweiten
Service oder eine plattformförmige öffentliche Dialoghierarchie einzuführen.

## Entscheidung

`SelectFolderDialogRequest` und `FileDialogService::requestSelectFolder()` werden ergänzt; als
Ergebnis wird weiterhin `FileDialogResultEvent` verwendet. Ein akzeptiertes Ergebnis enthält genau
einen nichtleeren lokalen `std::filesystem::path`, Cancel und Failed enthalten keinen Pfad. Der
native Picker garantiert das bestehende Verzeichnis; eine zusätzliche racy Dateisystemprüfung wird
nicht vorgenommen. Das Toolkit erzeugt, löscht, benennt nichts um, schreibt nichts und ändert das
Working Directory nicht.

Open, Save und Folder teilen eine monotone, bei null ungültige Request-ID-Sequenz, genau eine
Pending-Operation, die bestehende EventQueue-Grenze sowie Exactly-once-, Generation-, UI-Thread-
und Shutdown-Regeln. `file_dialogs` bleibt die gemeinsame Capability.

Win32 verwendet am bestehenden `HWND` `IFileOpenDialog` mit `FOS_PICKFOLDERS`,
`FOS_FORCEFILESYSTEM`, `FOS_PATHMUSTEXIST` und `FOS_NOCHANGEDIR`, ausdrücklich ohne
`FOS_FILEMUSTEXIST`. AppKit verwendet ein asynchrones Sheet am bestehenden `NSWindow` mit einem
`NSOpenPanel`, das nur Verzeichnisse und genau eine Auswahl erlaubt. GTK ab 4.10 verwendet
`gtk_file_dialog_select_folder()` / `select_folder_finish()`; ältere unterstützte GTK4-Header
verwenden asynchron `GtkFileChooserNative` mit `GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER`. Die native
Dateisystemrepräsentation wird direkt in `std::filesystem::path` kopiert; nichtlokale oder leere
Ergebnisse schlagen geschlossen fehl.

## Konsequenzen

Die öffentliche Vereinbarung bleibt klein und backendneutral. Filter, erweiterte Optionen,
Vorschlagsnamen, Startverzeichnisse und Mehrfachauswahl bleiben zurückgestellt. Es gibt keinen
`FolderDialogService`, kein `NativeFolderDialog`, keine öffentliche native Hierarchie und keine neue
Capability.
