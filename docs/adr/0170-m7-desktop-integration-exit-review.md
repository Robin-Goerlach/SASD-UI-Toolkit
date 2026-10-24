# ADR 0170: M7 Desktop Integration Exit Review

## Status

Accepted

## Predecessors

[0157](0157-native-menu-portability-boundary.md),
[0161](0161-m7-file-dialog-three-consumer-contract-review.md),
[0169](0169-m7-file-dialog-richer-options-completion-review.md)

## English

### Review result

The currently defined M7 Desktop Integration scope is technically complete. The review compared
the current source, tests, native CI workflows, accepted ADRs, backend documentation, changelog and
English/German roadmaps. The review found no production contract contradiction or regression. It did
add deterministic native seam coverage for the plural Open operation on Win32, AppKit and GTK4, so
the three native consumers now test the same order-preserving plural-result boundary as well as the
single-file and option paths.

### Implementation and validation record

The documentation-only file-dialog review was merged as PR #61
(`20d6fe02ce33de368d05ae7e7c05e22d1b3ebbda`). The M7 exit review was then merged as PR #62: its
final review head was `8c3e834c4ad6948c284700b861b45fd2483a71cd`, and the resulting `main` merge was
`522e3548ee21b9391c99a5ee040e704797794066`.

For that final `main`, the post-merge Main CI, GTK Native CI and AppKit Native CI completed
successfully (workflow runs `39467187651`, `39467187607` and `39467187357`, respectively). The
exit-review branch also passed the local strict Debug and Release builds and the configured test
suite. Native GUI interaction was not claimed from WSL: the visible macOS/Cocoa M3 gate remains open,
as stated below.

### Completed scope

M7 has validated three real native consumers for the same backend-neutral Core boundary:

- Native menus: Win32 `HMENU`, AppKit `NSMenu` and GTK4 `GMenu`/`GAction` projections.
- File dialogs: single Open, separate multiple Open, Save and Folder Selection.
- File-dialog options: literal Open/Save extension filters, native Save default-extension preference,
  Save suggested filename and initial-directory hints.
- Shared request IDs, one pending-operation boundary, queued application-level results,
  exactly-once completion, stale/generation protection, UI-thread rules and shutdown safety.

Open Single and Open Multiple intentionally remain separate operations and result types. The result
cardinality is therefore visible in the type system; no generic result hierarchy or public native
dialog hierarchy is required. Native menu and file-dialog projections remain private to their
backends, while `CommandActivationEvent`, file-dialog results and capability discovery remain
application-level/Core contracts.

### Boundaries and deferred work

Core remains free of Win32, AppKit and GTK types and no mandatory runtime dependency was added to the
portable Core. Terminal remains a first-class backend. Native child-control peers remain deferred;
they are not required to close M7.

The M7 exit does not implement or claim multiple Save/Folder selection, named filter groups, MIME or
URI picker APIs, recent locations, bookmarks, security roots, drag and drop, accessibility, IME,
printing, theme integration or any M8 component-metadata/tooling work. Those remain separate future
decisions.

The visible manual macOS/Cocoa M3 gate remains open. Native CI, AppKit CI and deterministic tests do
not count as visible manual macOS validation.

### Consequences

M7 is closed for its currently defined Desktop Integration scope. Future desktop integrations must
begin as separately scoped vertical slices with their own contract and ADR when they cross the
portable API or backend boundary. M8 is not started by this decision.

## Deutsch

### Review-Ergebnis

Der aktuell definierte Umfang von M7 Desktop-Integration ist technisch abgeschlossen. Der Review
verglich aktuellen Source, Tests, native CI-Workflows, angenommene ADRs, Backend-Dokumentation,
Changelog sowie englische und deutsche Roadmaps. Es wurde kein Produktions-Vertragswiderspruch und
keine Regression gefunden. Der Review ergänzte deterministische native Seam-Tests für die Mehrfach-
Open-Operation auf Win32, AppKit und GTK4. Damit testen alle drei nativen Consumer nun dieselbe
reihenfolgetreue Mehrfach-Result-Grenze sowie die Einzel- und Optionspfade.

### Implementierungs- und Validierungsprotokoll

Der dokumentations-only File-Dialog-Review wurde als PR #61 gemergt
(`20d6fe02ce33de368d05ae7e7c05e22d1b3ebbda`). Danach wurde der M7-Exit-Review als PR #62 gemergt:
Der finale Review-Head war `8c3e834c4ad6948c284700b861b45fd2483a71cd`; der daraus entstandene
`main`-Merge war `522e3548ee21b9391c99a5ee040e704797794066`.

Für diesen finalen `main` liefen Main CI, GTK Native CI und AppKit Native CI erfolgreich durch
(Workflow-Runs `39467187651`, `39467187607` beziehungsweise `39467187357`). Der Exit-Review-Branch
bestand außerdem die lokalen strikten Debug- und Release-Builds sowie die konfigurierte Testsuite.
Native GUI-Interaktion wurde unter WSL nicht behauptet: Der sichtbare macOS/Cocoa-M3-Gate bleibt wie
unten beschrieben offen.

### Abgeschlossener Umfang

M7 hat drei reale native Consumer für dieselbe backendneutrale Core-Grenze validiert:

- Native Menüs: Win32-`HMENU`-, AppKit-`NSMenu`- und GTK4-`GMenu`-/`GAction`-Projektionen.
- File Dialogs: Einzel-Open, separates Mehrfach-Open, Save und Ordnerauswahl.
- File-Dialog-Optionen: literale Open-/Save-Extension-Filter, native Save-Default-Extension-
  Präferenz, vorgeschlagener Save-Dateiname und Startverzeichnis-Hinweise.
- Gemeinsame Request-IDs, genau eine Pending-Operation, eingereihte Application-Level-Ergebnisse,
  Exactly-once-Abschluss, Stale-/Generation-Schutz, UI-Thread-Regeln und Shutdown-Sicherheit.

Einzel-Open und Mehrfach-Open bleiben bewusst getrennte Operationen und Result-Typen. Die
Ergebniskardinalität bleibt dadurch im Typsystem sichtbar; eine generische Result-Hierarchie oder
öffentliche native Dialoghierarchie ist nicht erforderlich. Native Menü- und File-Dialog-
Projektionen bleiben backendprivat; `CommandActivationEvent`, File-Dialog-Resultate und Capability-
Discovery bleiben Application-Level-/Core-Verträge.

### Grenzen und zurückgestellte Arbeiten

Core bleibt frei von Win32-, AppKit- und GTK-Typen; für den portablen Core wurde keine verpflichtende
Runtime-Abhängigkeit ergänzt. Terminal bleibt ein First-Class-Backend. Native Child-Control-Peers
bleiben zurückgestellt und sind für den M7-Abschluss nicht erforderlich.

Der M7-Abschluss implementiert oder behauptet nicht Mehrfachauswahl für Save/Folder, benannte
Filtergruppen, öffentliche MIME- oder URI-Picker-APIs, Recent Locations, Bookmarks, Security Roots,
Drag & Drop, Accessibility, IME, Printing, Theme-Integration oder Arbeiten aus M8 wie Component-
Metadata und Tooling. Diese Themen bleiben eigene spätere Entscheidungen.

Der sichtbare manuelle macOS/Cocoa-M3-Gate bleibt offen. Native CI, AppKit-CI und deterministische
Tests zählen nicht als sichtbare manuelle macOS-Validierung.

### Folgen

M7 ist für den aktuell definierten Umfang der Desktop-Integration abgeschlossen. Künftige Desktop-
Integrationen beginnen als separat gescopte vertikale Slices mit eigenem Vertrag und ADR, sobald sie
die portable API- oder Backend-Grenze berühren. M8 wird durch diese Entscheidung nicht begonnen.
