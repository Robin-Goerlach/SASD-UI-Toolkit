# ADR 0160: M7 GTK4 Open File validates the third file-dialog consumer

## Status

Accepted

## Context

The backend-neutral `FileDialogService` already has Win32's synchronous `IFileOpenDialog` and
AppKit's asynchronous `NSOpenPanel` consumers. GTK must validate the same boundary without GTK types
in Core, a second host, a second main loop or a callback API. GTK is discovered through `pkg-config`
only for the optional adapter and the project does not promise GTK 4.10 or newer.

## Decision

The private GTK service uses `GtkFileDialog` when the discovered headers are GTK 4.10 or newer. For
older GTK4 headers it uses `GtkFileChooserNative` with its asynchronous `response` signal. Both are
behind a private invoker seam; tests inject a fake and never display a chooser. No runtime dependency
or GTK minimum changes.

The existing `GtkWindow` owns the native dialog. Open selects one existing file, excludes directories
and multiple selection, and completes through the GLib default main-context pump in `pollEvent()`.
The native callback maps the result, copies a local path and queues `FileDialogResultEvent`; it never
executes commands or application callbacks.

IDs are non-zero, monotonic within one service lifetime and overflow-safe. Setup errors (uninitialized,
wrong thread, pending request, malformed UTF-8/NUL title and ID exhaustion) are synchronous. After ID
allocation, native start/completion failures become one pathless `failed` event. Pending starts after
ID allocation and ends when one completion event is successfully queued.

Shared completion state is captured weakly and records request ID, generation, pending and completed
flags plus a non-owning queue pointer. Duplicate and stale callbacks fail closed. Shutdown invalidates
state before cancel/hide and native-object release, then removes queue/window references, so late
callbacks produce no event and cannot access the backend or service.

`GFile` is accepted only when `g_file_get_path()` returns a non-empty local native path. The bytes are
copied immediately into `std::filesystem::path`, then GLib string and GObject references are released.
URI labels, canonicalization, normalization and existence checks are not substituted for the native
filesystem representation. Cancel maps to `cancelled`; unexpected/error, non-local and empty results
map to `failed`. Save, folder, multiple selection, filters, preview and accessory UI remain deferred.
No `GtkApplication` is required.

## Deutsch

## Status

Akzeptiert

## Kontext

Der backendneutrale `FileDialogService` besitzt bereits den synchronen Win32-`IFileOpenDialog`-
Consumer und den asynchronen AppKit-`NSOpenPanel`-Consumer. GTK soll dieselbe Grenze validieren, ohne
GTK-Typen in Core, einen zweiten Host, eine zweite Main Loop oder eine Callback-API einzuführen. Das
Projekt findet GTK nur optional über `pkg-config` und garantiert kein GTK 4.10 oder neuer.

## Entscheidung

Der private GTK-Service verwendet bei GTK-Headern ab 4.10 `GtkFileDialog`, bei älteren GTK4-Headern
`GtkFileChooserNative` mit asynchronem `response`-Signal. Beide bleiben hinter einer privaten
Invoker-Seam; Tests injizieren einen Fake und öffnen niemals einen Chooser. Laufzeitabhängigkeit und
GTK-Minimum ändern sich nicht.

Owner ist das bestehende `GtkWindow`. Open erlaubt eine existierende Datei, schließt Verzeichnisse und
Mehrfachauswahl aus und läuft über den GLib-Default-Kontext, den `pollEvent()` pumpt. Der native
Callback mappt nur das Ergebnis, kopiert den lokalen Pfad und legt `FileDialogResultEvent` ab; er führt
keine Commands oder Application-Callbacks aus.

IDs sind nicht-null, innerhalb einer Service-Lebenszeit monoton und overflow-sicher. Setup-Fehler
werden synchron gemeldet; nach ID-Vergabe werden native Start-/Completion-Fehler zu genau einem
pfadlosen `failed`-Event. Pending beginnt nach ID-Vergabe und endet beim erfolgreichen Queueing.

Der geteilte Completion-State wird schwach eingefangen und enthält ID, Generation, Pending-/Completed-
Flags sowie einen nicht besitzenden Queue-Zeiger. Doppelte und alte Callbacks schlagen geschlossen
fehl. Shutdown invalidiert den State vor Cancel/Hide und Freigabe nativer Objekte und entfernt Queue-
und Window-Referenzen; späte Callbacks erzeugen kein Event und greifen nicht auf Backend oder Service
zu.

`GFile` wird nur akzeptiert, wenn `g_file_get_path()` einen nicht-leeren lokalen nativen Pfad liefert.
Die Bytes werden sofort in `std::filesystem::path` kopiert, danach werden GLib-String und GObject-
Referenzen freigegeben. URI-Labels, Canonicalization, Normalisierung und Existenzprüfung ersetzen nicht
die native Dateisystemrepräsentation. Cancel wird `cancelled`, unerwartete Fehler sowie nicht-lokale
und leere Ergebnisse werden `failed`. Save, Ordner, Mehrfachauswahl, Filter, Preview und Accessory UI
bleiben zurückgestellt. Eine `GtkApplication` ist nicht erforderlich.
