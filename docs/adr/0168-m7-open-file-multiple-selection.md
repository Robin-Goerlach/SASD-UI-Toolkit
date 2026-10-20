# ADR 0168: M7 Open File Multiple Selection

## Status

Accepted

## Context

The validated M7 file-dialog contract supports one pending operation, shared non-zero request IDs,
queued application-level completion and native local-file selection. Multiple selection changes result
cardinality and therefore must not be hidden behind a mode flag on the single-file operation.

## Decision

Add an additive `OpenFilesDialogRequest` with `title`, `extensions` and `initial_directory`, plus
`FileDialogService::requestOpenFiles()`. Add the separate `OpenFilesDialogResultEvent` containing a
request ID, status and `std::vector<std::filesystem::path>`. A separate operation and result type
are preferred over `bool multiple`, changing `FileDialogResultEvent::path`, adding both `path` and
`paths`, or returning only the first native result. The single Open contract remains exactly one
path; Save and Folder remain single-result operations; the existing file-dialog capability is
unchanged.

An accepted plural result contains one or more non-empty paths, so one selected file is valid.
Cancelled and failed results contain no paths. Invalid combinations and ID zero throw
`std::logic_error`. Native result order is authoritative: paths are not sorted, deduplicated,
canonicalized, case-folded, Unicode-normalized or otherwise rewritten. If any native member cannot
be represented as a non-empty local filesystem path, the entire completion is failed atomically;
partial success is forbidden. Remote URIs, directories, mixed file/directory selections and save
targets are outside this operation.

The plural request reuses the exact literal UTF-8 extension-token validation and absolute
initial-directory hint semantics of Single Open. It participates in the same request-ID sequence
and service-wide pending boundary. Completion is still exactly once, queued after ID allocation,
and pending ends after the completion is enqueued. `OpenFilesDialogResultEvent` is an application-
level event: Application and EventDispatcher do not resolve or deliver it to widgets.

Native projections are deliberately private. Win32 uses `IFileOpenDialog` with the existing Open
flags plus `FOS_ALLOWMULTISELECT`, then `GetResults()`/`IShellItemArray`, `GetItemAt()` and
`SIGDN_FILESYSPATH`, preserving UTF-16 paths directly. AppKit uses `NSOpenPanel` with
`canChooseFiles=YES`, `canChooseDirectories=NO`, `allowsMultipleSelection=YES` and `URLs`; each
URL must be a local file URL with a non-empty `fileSystemRepresentation`. GTK 4.10+ uses
`gtk_file_dialog_open_multiple()` and `GListModel`, converting each `GFile` with `g_file_get_path()`;
the older supported GTK4 route uses `GtkFileChooserNative`, select-multiple and the actual
`gtk_file_chooser_get_files()` API. All native completion lifetimes and stale/shutdown protection
remain those of the existing backend services.

## Consequences

The type system exposes result cardinality without a speculative common result hierarchy, wrapper
variant or public options base. A small field duplication in the two Open request aggregates is
intentional and easier to review than a mode-dependent abstraction. The M7 richer-options block is
complete after this slice. The visible manual macOS/Cocoa M3 gate remains open because CI and unit
tests are not visible manual macOS validation.

## Deutsch

### Kontext

Der validierte M7-Dateidialog-Vertrag unterstützt eine Pending-Operation, gemeinsame nichtnull
Request-IDs, ein Application-Level-Completion-Event und die Auswahl lokaler Dateien. Eine
Mehrfachauswahl verändert die Ergebniskardinalität und darf daher nicht als Modus-Schalter der
Einzeloperation versteckt werden.

### Entscheidung

Ergänzt werden `OpenFilesDialogRequest` mit `title`, `extensions` und `initial_directory` sowie
`FileDialogService::requestOpenFiles()`. Dazu kommt der getrennte
`OpenFilesDialogResultEvent` mit Request-ID, Status und `std::vector<std::filesystem::path>`. Eine
separate Operation und ein separater Result-Typ sind vorzuziehen gegenüber `bool multiple`, einer
Änderung von `FileDialogResultEvent::path`, parallelen `path`-/`paths`-Feldern oder dem Zurückgeben
nur des ersten nativen Ergebnisses. Single Open bleibt exakt ein Pfad; Save und Folder bleiben
Einzel-Result-Operationen; die bestehende Capability bleibt unverändert.

Ein akzeptiertes Plural-Result enthält mindestens einen nichtleeren Pfad; eine Datei ist daher
gültig. Cancel und Failure enthalten keine Pfade. Ungültige Kombinationen und ID null lösen
`std::logic_error` aus. Die native Reihenfolge bleibt maßgeblich: Es wird nicht sortiert,
dedupliziert, canonicalisiert, normalisiert oder anderweitig umgeschrieben. Wenn ein natives Element
nicht als nichtleerer lokaler Dateisystempfad darstellbar ist, wird atomar das gesamte Completion-
Result zu Failed; Teil-Erfolg ist verboten. Remote-URIs, Verzeichnisse, gemischte Auswahl und Save-
Ziele gehören nicht zu dieser Operation.

Die Plural-Request verwendet exakt die Literal-UTF-8-Extension-Validierung und die Regeln des
absoluten Startverzeichnisses von Single Open. Sie nutzt dieselbe Request-ID-Sequenz und dieselbe
serviceweite Pending-Grenze. Completion bleibt Exactly-once, wird nach ID-Vergabe eingereiht, und
Pending endet nach erfolgreichem Enqueue. Das Plural-Result ist ein Application-Level-Event und
wird von Application und EventDispatcher nicht an Widgets geroutet.

Die nativen Projektionen bleiben privat: Win32 verwendet `IFileOpenDialog` mit den bestehenden
Open-Flags plus `FOS_ALLOWMULTISELECT` sowie `GetResults()`/`IShellItemArray` und übernimmt UTF-16-
Pfade direkt. AppKit verwendet `NSOpenPanel` mit Datei-only, `allowsMultipleSelection=YES` und
`URLs`; jeder Eintrag muss eine lokale File-URL mit nichtleerer `fileSystemRepresentation` sein.
GTK ab 4.10 verwendet `gtk_file_dialog_open_multiple()` und `GListModel` sowie
`g_file_get_path()`; der ältere GTK4-Pfad nutzt `GtkFileChooserNative`, Mehrfachauswahl und die
tatsächliche `gtk_file_chooser_get_files()`-API. Lifetime-, Stale- und Shutdown-Schutz bleiben
unverändert.

## Implementation and validation record

This slice is implemented on feature branch `codex/m7-open-file-multiple-selection`. The local
strict GCC Debug build and configured CTest suite pass in WSL. GTK4, Windows MSVC and AppKit are
validated by their native CI jobs when available; no visible manual native GUI smoke test is claimed
from WSL. No release or tag is created.
