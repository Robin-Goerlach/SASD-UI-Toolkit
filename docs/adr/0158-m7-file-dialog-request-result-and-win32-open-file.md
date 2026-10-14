# ADR 0158 – M7 file dialogs start with an application-level request/result service and Win32 Open File / M7 File Dialogs beginnen mit application-level Request/Result und Win32 Open File

**Status:** Accepted
**Date:** 2026-10-14
**Predecessor:** [0157](0157-native-menu-portability-boundary.md)

## English

### Context

File dialogs are host/platform services, not Widgets, Rendered presentation, Commands or Menus.
Native dialog APIs also differ materially: Win32 and AppKit may run a modal operation while GTK is
expected to provide an asynchronous completion path. A callback would cross a native lifetime
boundary and could destroy the Application or Backend at an unsafe point.

### Decision

Core exposes the optional `FileDialogService`. Slice 1 contains only
`requestOpenFile(OpenFileDialogRequest)` for one existing filesystem file and single selection.
Every request receives a non-zero monotonically allocated `FileDialogRequestId`; version 1 allows one
pending request per backend and rejects a second pending request with `std::logic_error`. Overflow
fails closed. Completion is always the application-level `FileDialogResultEvent`: accepted has exactly
one `std::filesystem::path`, while cancelled and failed have no path. Request initiation may enter a
platform modal loop and may block, but it never returns the selected path and invokes no application
callback. Normal user cancellation is not an exception.

`std::filesystem::path` represents filesystem identity and keeps Windows native UTF-16 path semantics;
the contract performs no canonicalization, normalization or existence check after a native dialog has
already enforced its open-file policy. `Backend::fileDialogs()` is optional and non-owning; the
service is usable only during backend initialization. `BackendCapabilities::file_dialogs` is appended
to the capability aggregate so prior positional initialization keeps its meaning.

The first real consumer is private to `SASD::UI::Native::Win32`. It uses the existing backend HWND as
owner and `IFileOpenDialog` with filesystem/existing-file/path/no-directory-change options. The title
is strictly converted from UTF-8 to UTF-16. A small per-operation COM RAII boundary calls
`CoInitializeEx(COINIT_APARTMENTTHREADED)` on the UI thread, balances only successful initialization,
and fails closed for an incompatible apartment. `ERROR_CANCELLED` maps to `cancelled`; other native
failures become one pathless `failed` event after the request ID has been issued. There is no throw and
event for the same failure.

The Win32 implementation uses a private native-invocation seam for deterministic service tests; the
production `IFileOpenDialog` path is still compiled and linked in Windows CI. Filters, initial
directory, Save File, folder selection, multiple selection, AppKit and GTK consumers remain deferred.

## Deutsch

### Kontext

File Dialogs sind Host-/Plattformdienste und keine Widgets, Rendered-Presentation, Commands oder
Menus. Native Dialog-APIs unterscheiden sich außerdem wesentlich: Win32 und AppKit können modal
arbeiten, während GTK voraussichtlich einen asynchronen Abschluss liefert. Ein Callback würde die
native Lifetime-Grenze überschreiten und Application oder Backend zu einem unsicheren Zeitpunkt
zerstören können.

### Entscheidung

Der Core stellt den optionalen `FileDialogService` bereit. Slice 1 enthält nur
`requestOpenFile(OpenFileDialogRequest)` für genau eine existierende Datei mit Einzelauswahl. Jede
Anfrage erhält eine nicht-null, monoton vergebene `FileDialogRequestId`; Version 1 erlaubt je Backend
höchstens eine Pending-Anfrage und weist eine zweite mit `std::logic_error` zurück. Overflow schlägt
geschlossen fehl. Der Abschluss ist immer das application-level `FileDialogResultEvent`: accepted hat
genau einen `std::filesystem::path`, cancelled und failed haben keinen Pfad. Die Initiierung darf in
eine native modale Schleife gehen und blockieren, gibt aber niemals den Pfad zurück und ruft keinen
Application-Callback auf. Normaler Benutzer-Cancel ist keine Exception.

`std::filesystem::path` repräsentiert Dateisystemidentität und bewahrt unter Windows native UTF-16-
Pfadsemantik; es gibt keine Canonicalization, Normalisierung oder nachgelagerte Existenzprüfung, wenn
der native Dialog seine Open-File-Policy bereits durchgesetzt hat. `Backend::fileDialogs()` ist
optional und nicht-ownend; der Service ist nur während der Backend-Initialisierung nutzbar.
`BackendCapabilities::file_dialogs` wird am Ende des Capability-Aggregats ergänzt, damit frühere
positionsbasierte Initialisierungen ihre Bedeutung behalten.

Der erste reale Consumer bleibt privat in `SASD::UI::Native::Win32`. Er verwendet das vorhandene HWND
des Backends als Owner und `IFileOpenDialog` mit Filesystem-/Existing-File-/Path-/No-Directory-Change-
Optionen. Der Titel wird strikt von UTF-8 nach UTF-16 konvertiert. Eine kleine COM-RAII-Grenze pro
Operation ruft `CoInitializeEx(COINIT_APARTMENTTHREADED)` auf dem UI-Thread auf, gleicht nur eine
erfolgreiche Initialisierung aus und schlägt bei inkompatiblem Apartment geschlossen fehl.
`ERROR_CANCELLED` wird zu `cancelled`; andere native Fehler werden nach Ausgabe der Request-ID zu
genau einem pfadlosen `failed`-Event. Für denselben Fehler gibt es nicht gleichzeitig Exception und
Event.

Die Win32-Implementierung verwendet für deterministische Service-Tests eine private Native-
Invocation-Seam; der produktive `IFileOpenDialog`-Pfad wird in der Windows-CI weiterhin kompiliert und
gelinkt. Filter, Initial Directory, Save File, Ordnerauswahl, Mehrfachauswahl sowie AppKit- und GTK-
Consumer bleiben zurückgestellt.
