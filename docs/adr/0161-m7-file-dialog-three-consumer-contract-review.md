# ADR 0161: M7 file-dialog three-consumer contract review

## Status

Accepted

## Context

M7 now has three real consumers of one portable file-dialog contract. The review checks whether GTK
exposed a missing cross-platform abstraction before richer dialog options are added.

## Decision

The current contract is sufficient. Core remains the small semantic boundary:

`FileDialogService` + `OpenFileDialogRequest` + non-zero `FileDialogRequestId` + queued
`FileDialogResultEvent` + `std::filesystem::path` + `BackendCapabilities::file_dialogs`.

The private projections deliberately retain their native differences:

| Consumer | Native owner and operation | Completion/path |
| --- | --- | --- |
| Win32 | existing `HWND`, synchronous/modal `IFileOpenDialog` with COM | native UTF-16 filesystem path |
| AppKit | existing `NSWindow`, asynchronous `NSOpenPanel` sheet | completion block, `fileSystemRepresentation` |
| GTK4 | existing `GtkWindow`, async `GtkFileDialog` or `GtkFileChooserNative` | GLib completion, local `GFile` via `g_file_get_path()` |

All three use monotonic non-zero IDs, at most one pending native operation, exactly one pathless
cancelled/failed or accepted path result, and pending ends at event enqueue. Private state protects
ownership, shutdown, UI-thread affinity, stale generations and duplicate completion. Native callbacks
never run application code. Local native filesystem representation is preserved without
canonicalization or Unicode normalization; invalid/non-local paths fail closed.

The EventQueue is intentionally the async boundary. A callback, future, promise, coroutine or generic
`AsyncOperation` would expose lifecycle/executor policy unnecessarily and would not unify Win32's
synchronous modal call. No public `NativeFileDialog` hierarchy, `DialogManager` or platform-specific
request type is introduced. Save, folder, filters and multiple selection remain deferred.

## Deutsch

## Status

Akzeptiert

## Kontext

M7 besitzt drei reale Consumer desselben portablen File-Dialog-Vertrags. Der Review prüft, ob GTK vor
reicheren Dialogoptionen eine fehlende gemeinsame Abstraktion sichtbar gemacht hat.

## Entscheidung

Der bestehende Vertrag ist ausreichend. Core bleibt die kleine semantische Grenze:

`FileDialogService` + `OpenFileDialogRequest` + nicht-null `FileDialogRequestId` + gequeutes
`FileDialogResultEvent` + `std::filesystem::path` + `BackendCapabilities::file_dialogs`.

Win32 behält das vorhandene `HWND` und den synchronen/modalen COM-`IFileOpenDialog` mit UTF-16-Pfad;
AppKit das vorhandene `NSWindow` und das asynchrone `NSOpenPanel`-Sheet mit
`fileSystemRepresentation`; GTK das vorhandene `GtkWindow`, die asynchrone
`GtkFileDialog`-/`GtkFileChooserNative`-Completion, den GLib-Default-Kontext und den lokalen
`GFile`-Pfad über `g_file_get_path()`.

Alle drei vergeben monotone, nicht-null IDs, erlauben höchstens eine Pending-Operation, legen genau
ein pfadloses Cancelled-/Failed- oder ein Accepted-Result ab und beenden Pending beim Queueing. Private
States schützen Ownership, Shutdown, UI-Thread, alte Generationen und doppelte Completions. Native
Callbacks führen keinen Application-Code aus. Lokale native Pfade bleiben ohne Canonicalization oder
Unicode-Normalisierung erhalten; ungültige oder nicht-lokale Pfade schlagen geschlossen fehl.

Die EventQueue ist bewusst die asynchrone Grenze. Callback, Future, Promise, Coroutine und generisches
`AsyncOperation` würden Lebensdauer- und Executor-Politik unnötig öffentlich machen und den synchronen
Win32-Dialog nicht vereinheitlichen. Es gibt keine öffentliche `NativeFileDialog`-Hierarchie, keinen
`DialogManager` und keinen plattformspezifischen Request-Typ. Save, Ordner, Filter und Mehrfachauswahl
bleiben spätere Vertragsentscheidungen.
