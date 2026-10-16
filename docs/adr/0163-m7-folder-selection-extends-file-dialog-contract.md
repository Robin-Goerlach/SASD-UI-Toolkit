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

## Implementation and validation record

This decision was implemented as the M7 Folder Selection slice across Core, Win32, AppKit and GTK4.
The Core API adds `SelectFolderDialogRequest` and `requestSelectFolder()`; all three operations use
the existing result event, ID sequence, pending boundary, EventQueue completion and lifecycle
protection. The native demos, deterministic backend tests, English and German backend documentation,
roadmaps and changelog were updated with the same contract.

Before the feature branch was created, the two historical documentation follow-ups were checked
individually and merged because they contained only their expected ADR changes: PR #46 merged as
`82d77fd4fac041f21d48c380b6852acbb995fdd3` and PR #48 merged as
`60768d864a62d858a53a4b622064ef08e9636ec3`. The folder-selection branch was based on that latter
main commit. Its final head was `f7476e48bc3484e8d2a7bfef2ab7177e05324ed0` in PR #49, which merged
as `36a6205dcf68a538abfeff499ba4c8e8223dea04`; the feature branch was then removed locally and
remotely.

All required checks for the exact PR #49 head passed: Linux GCC, Linux Clang, Linux Clang
ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux Release, Windows Release, macOS
Release, GTK Native and AppKit Native. The exact merge commit subsequently passed Main CI, GTK
Native CI and AppKit Native CI.

A fresh strict local GCC build with tests and examples enabled, warnings-as-errors enabled, and the
configured CTest suite passed all four tests. GTK and Xvfb were not available in WSL, and Windows
MSVC/AppKit could not be validated locally; their native CI jobs are authoritative. No visible manual
native smoke test was performed, so it is not claimed as executed. No release or tag was created;
the separate M3 macOS/Cocoa gate remains open.

PR #30 and PR #32 were inspected read-only and remain unchanged. Filters and richer options,
default extensions, suggested filenames, initial directories and multiselect remain deliberately
deferred to a later M7 slice.

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

## Umsetzungs- und Validierungsnachtrag

Diese Entscheidung wurde als M7-Slice „Folder Selection“ für Core, Win32, AppKit und GTK4 umgesetzt.
Die Core-API ergänzt `SelectFolderDialogRequest` und `requestSelectFolder()`; alle drei Operationen
verwenden weiterhin dasselbe Result-Event, dieselbe ID-Sequenz, dieselbe Pending-Grenze, dieselbe
EventQueue-Abschlussgrenze und denselben Lifecycle-Schutz. Native Demos, deterministische Backend-
Tests, englische und deutsche Backend-Dokumentation, Roadmaps und Changelog wurden entsprechend
aktualisiert.

Vor Erstellung des Feature-Branches wurden die beiden historischen Dokumentations-Follow-ups einzeln
geprüft und wegen ihrer ausschließlich erwarteten ADR-Änderungen gemergt: PR #46 als
`82d77fd4fac041f21d48c380b6852acbb995fdd3`, PR #48 als
`60768d864a62d858a53a4b622064ef08e9636ec3`. Der Folder-Branch basierte auf diesem Main-Commit. Sein
finaler Head war `f7476e48bc3484e8d2a7bfef2ab7177e05324ed0` in PR #49; dieser wurde als
`36a6205dcf68a538abfeff499ba4c8e8223dea04` gemergt. Der Feature-Branch wurde anschließend lokal und
remote entfernt.

Für exakt den finalen PR-#49-Head waren alle erforderlichen Checks grün: Linux GCC, Linux Clang,
Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux Release, Windows Release,
macOS Release, GTK Native und AppKit Native. Der exakte Merge-Commit bestand anschließend Main CI,
GTK Native CI und AppKit Native CI.

Ein frischer strikter lokaler GCC-Build mit aktivierten Tests und Beispielen sowie Warnings-as-Errors
war erfolgreich; die konfigurierte CTest-Suite bestand alle vier Tests. GTK und Xvfb waren unter WSL
nicht vorhanden, Windows MSVC und AppKit konnten lokal nicht geprüft werden; dafür sind die nativen
CI-Jobs maßgeblich. Ein sichtbarer manueller nativer Smoke-Test wurde nicht durchgeführt und wird
daher nicht als ausgeführt behauptet. Es wurde kein Release und kein Tag erstellt; das separate
M3-macOS/Cocoa-Gate bleibt offen.

PR #30 und PR #32 wurden ausschließlich lesend geprüft und bleiben unverändert. Filter und erweiterte
Optionen, Default-Erweiterungen, Vorschlagsdateinamen, Startverzeichnisse und Mehrfachauswahl bleiben
bewusst für einen späteren M7-Slice zurückgestellt.
