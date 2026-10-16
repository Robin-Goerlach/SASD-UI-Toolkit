# ADR 0159 – M7 AppKit NSOpenPanel validates asynchronous file-dialog completion / M7 AppKit NSOpenPanel validiert asynchronen File-Dialog-Abschluss

**Status:** Accepted
**Date:** 2026-10-16
**Predecessor:** [0158](0158-m7-file-dialog-request-result-and-win32-open-file.md)

## English

### Decision

AppKit is the second real consumer of the existing backend-neutral `FileDialogService`. It reuses
`OpenFileDialogRequest`, non-zero request IDs and queued `FileDialogResultEvent`; no native hierarchy,
factory, future, promise or callback API is added. `NSOpenPanel` is configured for files only, no
directories and single selection, and is presented asynchronously with
`beginSheetModalForWindow:completionHandler:` using the backend's existing private `NSWindow`.

`requestOpenFile()` returns after assigning the ID and starting the sheet. One pending operation means
the interval from successful ID assignment through queueing exactly one completion event. The event
need not have been consumed; a new request is allowed immediately after queueing. A private invoker
seam makes tests deterministic without showing a panel.

The service owns a shared completion state containing only the active queue pointer and request
identity. Native completion captures it weakly. Shutdown first invalidates active/pending/generation
state and queue access, then cancels the panel; late completion is ignored. Generation and completed
checks make stale and duplicate completions fail closed. The completion block copies and validates the
native result, maps OK/cancel/technical responses, and queues the application event only; no
Application, Widget, Command or user callback runs in the native block.

Titles use strict UTF-8 conversion. Accepted results require one file URL and use its native
filesystem representation to construct `std::filesystem::path`; no canonicalization, normalization,
or post-selection existence check is performed. Save, folder selection, filters and multiselect remain
deferred. GTK4 is the next validation consumer, followed by a three-consumer contract review.

### Consequences

Win32 now demonstrates synchronous/modal completion queued before return, while AppKit demonstrates
asynchronous sheet completion after return. Both validate the same Core request/result contract.

### Implementation and validation

The decision is implemented by the private AppKit service in
`src/native/appkit/appkit_file_dialog_service.mm`, exposed only through the existing
`AppKitWindowBackend::fileDialogs()` overrides. The backend reports `file_dialogs` only while
initialized and invalidates the completion state before shutdown tears down the event queue and
window. The native `NSOpenPanel` path is compiled and linked by AppKit Native CI; tests use only the
private invoker seam and therefore never display a panel.

The deterministic AppKit tests cover capability and service lifetime, asynchronous return-before-event
ordering, rejection of a second pending request, request reuse after queueing, accepted/cancelled/
failed mapping, strict malformed-title rejection, native filesystem-representation preservation,
double completion, stale generation, and completion after shutdown or service destruction. The demo
exercises the complete `File -> Open File` menu, command activation, queued result event, and result
printing without introducing an application callback API. PR #43 was merged at `f1b9236`; its final
AppKit Native, GTK Native, and main CI checks passed for the validated merge state.

## Deutsch

### Entscheidung

AppKit ist der zweite reale Consumer des bestehenden backendneutralen `FileDialogService`. Es verwendet
`OpenFileDialogRequest`, nicht-null Request-IDs und queued `FileDialogResultEvent` unverändert; es gibt
keine Native-Hierarchie, Factory, Future, Promise- oder Callback-API. `NSOpenPanel` wird auf Dateien,
keine Ordner und Einzelauswahl konfiguriert und mit `beginSheetModalForWindow:completionHandler:` am
bereits vorhandenen privaten `NSWindow` asynchron angezeigt.

`requestOpenFile()` kehrt nach ID-Vergabe und Start des Sheets zurück. Eine Pending-Operation reicht
von erfolgreicher ID-Vergabe bis zum Queueing genau eines Completion-Events. Das Event muss noch nicht
konsumiert sein; nach dem Queueing ist ein neuer Request erlaubt. Eine private Invoker-Seam macht Tests
deterministisch, ohne ein sichtbares Panel zu öffnen.

Der Service besitzt einen geteilten Completion-State mit nur aktivem Queue-Pointer und Request-
Identität. Native Completions halten ihn schwach. Beim Shutdown werden aktiver/pending/generation-State
und Queue-Zugriff zuerst invalidiert, danach wird das Panel abgebrochen; verspätete Completions werden
ignoriert. Generation- und Completed-Prüfungen schlagen bei alten und doppelten Completions geschlossen
fehl. Der Completion-Block kopiert und validiert das native Ergebnis, mappt OK/Cancel/technische Fehler
und legt ausschließlich das Application-Event ab; er ruft weder Application, Widgets, Commands noch
User-Callbacks auf.

Titel werden strikt als UTF-8 konvertiert. Akzeptierte Ergebnisse erfordern eine File-URL und verwenden
deren native Dateisystemrepräsentation für `std::filesystem::path`; Canonicalization, Normalisierung
und nachträgliche Existenzprüfung entfallen. Save, Ordnerauswahl, Filter und Mehrfachauswahl bleiben
zurückgestellt. GTK4 ist der nächste Validierungs-Consumer, danach folgt ein Drei-Consumer-Review.

### Konsequenzen

Win32 zeigt weiterhin den synchron/modal vor der Rückkehr gequeueten Abschluss; AppKit zeigt den
asynchronen Sheet-Abschluss nach der Rückkehr. Beide validieren denselben Core-Request-/Result-Vertrag.

### Umsetzung und Validierung

Die Entscheidung ist im privaten AppKit-Service unter
`src/native/appkit/appkit_file_dialog_service.mm` umgesetzt und ausschließlich über die bestehenden
`AppKitWindowBackend::fileDialogs()`-Overrides erreichbar. Das Backend meldet `file_dialogs` nur im
initialisierten Zustand und invalidiert den Completion-State, bevor EventQueue und Window beim
Shutdown abgebaut werden. Der native `NSOpenPanel`-Pfad wird in der AppKit Native CI kompiliert und
gelinkt; die Tests verwenden ausschließlich die private Invoker-Seam und zeigen daher kein Panel an.

Die deterministischen AppKit-Tests decken Capability- und Service-Lifetime, die asynchrone
Rückkehr-vor-Event-Reihenfolge, die Ablehnung eines zweiten Pending-Requests, Wiederverwendung nach
dem Queueing, Accepted/Cancelled/Failed-Mapping, strikte Ablehnung fehlerhafter Titel, Erhalt der
nativen Dateisystemrepräsentation, doppelte Completion, veraltete Generation sowie Completion nach
Shutdown oder Service-Zerstörung ab. Die Demo durchläuft den vollständigen Ablauf `File -> Open File`,
Command-Aktivierung, gequeuetes Result-Event und Result-Ausgabe, ohne eine Application-Callback-API
einzuführen. PR #43 wurde mit `f1b9236` gemergt; die finalen AppKit-Native-, GTK-Native- und Main-CI-
Checks für den validierten Merge-Stand waren erfolgreich.
