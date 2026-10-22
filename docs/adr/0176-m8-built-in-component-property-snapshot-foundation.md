# ADR 0176: M8 Built-in Component Property Snapshot Foundation

- Status: Accepted
- Date: 2026-10-22

## Predecessors

- [0171](0171-m8-component-type-metadata-foundation.md)
- [0172](0172-m8-component-property-metadata-schema-foundation.md)
- [0173](0173-m8-typed-runtime-property-value-access-foundation.md)
- [0174](0174-m8-effective-property-default-metadata-foundation.md)
- [0175](0175-m8-exact-built-in-runtime-type-identification-foundation.md)

## English

### Context

M8 now has the separate foundations for exact built-in runtime identification, immutable property
schema metadata, semantic runtime property access and target-type-relative effective defaults. A
future tooling or serialization consumer needs to use these layers together, but a complete
Component Description is not yet defined. The current schema contains only ten selected boolean and
string properties; other state such as bindings, models, layouts, ownership and visual children is
not represented by this metadata.

### Decision

Add the backend-neutral `sasd/ui/component_property_snapshot.hpp` layer with
`captureBuiltInComponentPropertySnapshot(const Component&)`. The function first obtains the exact
catalogued type through `builtInComponentTypeOf()`. It then walks the immutable `base_type_id` chain
from base to derived and visits each direct property in its existing declaration order.

The returned `BuiltInComponentPropertySnapshot` owns its `type_id` and contains two typed owned
collections: `ComponentBooleanPropertySnapshot` and `ComponentStringPropertySnapshot`. Each entry
owns `declaring_type_id`, `property_id` and, for strings, the captured value. Every catalogued
property is included, including properties equal to their effective default.

Runtime values are obtained only through the established Slice 3 read functions. Each default
comparison uses the established Slice 4 query with the exact snapshot target type, not the direct
declaring type. A failed runtime read, failed default lookup, invalid metadata descriptor or
malformed/cyclic base chain rejects the complete operation with `std::nullopt`; no partial snapshot
is returned. An exact known built-in with no catalogued properties is valid and returns empty
collections. Unknown derived types also return `std::nullopt` through the Slice 5 exact-type query.

Two explicit value-kind collections are used instead of `PropertyValue`, `std::variant`, `std::any`,
`void*` or another type-erased carrier. The snapshot is a C++ value structure, not a persistence
format. It performs no child or ownership recursion, does not enumerate a UI tree and does not
implement serialization, deserialization, factories, validation, design-time metadata or
third-party registration.

### Explicit boundary

A successful snapshot means: all properties currently catalogued for this exact built-in type were
captured. It does not mean that the complete semantic state of the Component was captured. Property
schema coverage can grow in separately justified slices, and later serialization work must define
its encoding and tree/reference policy independently.

### Consequences

The next tooling or serialization consumer can start from a live `const Component&` without trusting
a caller-supplied type ID and without silently omitting a currently catalogued inherited property.
The snapshot is independent of the Component lifetime and can be retained, compared or encoded by a
later layer. The contract intentionally keeps default-omission policy outside the snapshot, and it
leaves complete property coverage and component-tree semantics open.

## Implementation and validation record

The implementation was added as a separate public snapshot header and Core source file, with CMake
wiring, focused tests and an extension of the existing headless component metadata demo. The change
does not add properties, alter the Component ownership contract, introduce a persistence format or
modify the earlier metadata/access/default layers.

The implementation was delivered in commit `b40e3e238f7288c68ebd55f186d7902d1fa69928` on branch
`codex/m8-component-property-snapshot` and published as PR #77. The final PR head remained this
commit. The focused tests cover default and modified Button snapshots, Label, CheckBox, Command and
sanitized TextField values, a known built-in with no catalogued properties, unknown derived types,
owned snapshot strings after Component destruction, deterministic inherited order and unchanged
Component ownership/state.

Strict GCC Debug and Release builds with C++20 and warnings as errors were run from fresh
out-of-source build directories. The complete configured CTest suite passed in both configurations
(4/4 tests each), `git diff --check` passed, and both headless demo builds printed the expected
snapshot values and default markers. No backend initialization or tree traversal was introduced.

All required checks for PR #77 passed for the final head: Linux GCC, Linux Clang, Linux Clang
ASan + UBSan, Linux SDL3 adapter, Windows MSVC, macOS AppleClang, GTK4 Native, macOS AppKit
Native, Linux Release, Windows Release and macOS Release.

PR #77 was merged by a normal merge commit as
`3a3ca0926c00b5ddd4e687952cc2f8fb2d0fdde3`. Post-merge validation for exactly that merge SHA was
fully green: Main CI including the complete Linux, Windows and macOS Release matrix, GTK Native CI
and AppKit Native CI. The feature branch was deleted locally and remotely, followed by
`fetch --prune`; local `main` was synchronized to the merge SHA and the working tree was clean. PRs
#30, #32 and #63 were not changed. No release or tag was created. The macOS/Cocoa M3 gate remains
open. M8 remains active; property-coverage expansion, tree snapshots, persistence formats,
serialization, deserialization, factories, validation, design-time metadata, resources, designer
and IDE integration were not started.

## Deutsch

### Kontext

M8 besitzt inzwischen getrennte Grundlagen für exakte Built-in-Runtime-Identifikation,
unveränderliche Property-Schema-Metadaten, semantischen Runtime-Property-Zugriff und zieltyprelative
effektive Defaults. Ein künftiger Tooling- oder Serialisierungs-Consumer muss diese Schichten
gemeinsam verwenden können, ohne dass bereits eine vollständige Component Description definiert
ist. Das aktuelle Schema enthält nur zehn ausgewählte Boolean- und String-Properties; weiterer
Zustand wie Bindings, Models, Layouts, Ownership und visuelle Kinder ist in diesen Metadaten nicht
enthalten.

### Entscheidung

Die backendneutrale Schicht `sasd/ui/component_property_snapshot.hpp` wird mit
`captureBuiltInComponentPropertySnapshot(const Component&)` ergänzt. Die Funktion bestimmt zuerst
über `builtInComponentTypeOf()` den exakten katalogisierten Typ. Danach durchläuft sie die
unveränderliche `base_type_id`-Kette von der Basis zum Derived-Typ und besucht innerhalb jedes
direkt deklarierenden Typs die bestehende Property-Reihenfolge.

Der zurückgegebene `BuiltInComponentPropertySnapshot` besitzt seine `type_id` und enthält zwei
typsichere besitzende Collections: `ComponentBooleanPropertySnapshot` und
`ComponentStringPropertySnapshot`. Jeder Eintrag besitzt `declaring_type_id`, `property_id` und
bei Strings den erfassten Wert. Jede katalogisierte Property wird aufgenommen, auch wenn ihr Wert
dem effektiven Default entspricht.

Runtime-Werte werden ausschließlich über die bestehenden Slice-3-Read-Funktionen gelesen. Jeder
Default-Vergleich verwendet die bestehende Slice-4-Abfrage mit dem exakten Snapshot-Zieltyp, nicht
dem direkt deklarierenden Typ. Ein fehlender Runtime-Read, fehlender Default-Lookup, ungültiger
Metadaten-Deskriptor oder fehlerhafte/zyklische Basiskette verwirft die gesamte Operation mit
`std::nullopt`; ein partieller Snapshot wird nicht zurückgegeben. Ein exakt bekannter Built-in ohne
katalogisierte Properties ist gültig und liefert leere Collections. Unbekannte Derived-Typen liefern
über die exakte Slice-5-Abfrage ebenfalls `std::nullopt`.

Statt `PropertyValue`, `std::variant`, `std::any`, `void*` oder eines anderen type-erased Carriers
werden zwei explizite Value-Kind-Collections verwendet. Der Snapshot ist eine C++-Datenstruktur,
kein Persistenzformat. Es gibt keine Child- oder Ownership-Rekursion, keine UI-Tree-Aufzählung und
keine Implementierung von Serialisierung, Deserialisierung, Factories, Validierung, Design-Time-
Metadaten oder Drittanbieter-Registrierung.

### Explizite Grenze

Ein erfolgreicher Snapshot bedeutet: Alle für diesen exakten Built-in-Typ aktuell katalogisierten
Properties wurden erfasst. Er bedeutet nicht, dass der vollständige semantische Zustand der
Component erfasst wurde. Die Property-Schema-Abdeckung kann in separat begründeten Slices wachsen;
spätere Serialisierung muss Encoding sowie Tree-/Reference-Policy unabhängig festlegen.

### Folgen

Der nächste Tooling- oder Serialisierungs-Consumer kann von einer lebenden `const Component&`
ausgehen, ohne eine vom Aufrufer gelieferte Typ-ID zu vertrauen und ohne eine aktuell katalogisierte
geerbte Property still zu verlieren. Der Snapshot ist unabhängig von der Lebenszeit der Component
und kann später aufbewahrt, verglichen oder encodiert werden. Die Default-Omission-Policy bleibt
bewusst außerhalb des Snapshots; vollständige Property-Abdeckung und Component-Tree-Semantik bleiben
offen.

### Implementierungs- und Validierungsprotokoll

Die Implementierung ergänzt einen separaten öffentlichen Snapshot-Header und eine Core-Quelldatei,
CMake-Verdrahtung, fokussierte Tests sowie eine Erweiterung des bestehenden headless Component-
Metadata-Demos. Es werden keine Properties ergänzt, der Component-Ownership-Vertrag nicht verändert,
kein Persistenzformat eingeführt und die vorherigen Metadata-/Access-/Default-Schichten nicht
verändert.

Die Implementierung wurde im Commit `b40e3e238f7288c68ebd55f186d7902d1fa69928` auf dem Branch
`codex/m8-component-property-snapshot` erstellt und als PR #77 veröffentlicht. Der finale PR-Head
blieb dieser Commit. Die fokussierten Tests prüfen Default- und geänderte Button-Snapshots, Label,
CheckBox, Command und sanitisierten TextField-Wert, einen bekannten Built-in ohne katalogisierte
Properties, unbekannte Derived-Typen, besitzende Snapshot-Strings nach der Zerstörung der Component,
deterministische geerbte Reihenfolge sowie unveränderten Component-Zustand und Ownership.

Strikte GCC-Debug- und Release-Builds mit C++20 und Warnings-as-Errors wurden in frischen
Out-of-Source-Builds ausgeführt. Die vollständige konfigurierte CTest-Suite war in beiden
Konfigurationen erfolgreich (jeweils 4/4 Tests), `git diff --check` war erfolgreich und beide
headless Demo-Builds gaben die erwarteten Snapshot-Werte und Default-Markierungen aus. Es wurden
keine Backend-Initialisierung und keine Tree-Traversierung eingeführt.

Alle erforderlichen Checks von PR #77 waren für den finalen Head erfolgreich: Linux GCC, Linux
Clang, Linux Clang ASan + UBSan, Linux SDL3 Adapter, Windows MSVC, macOS AppleClang, GTK4 Native,
macOS AppKit Native, Linux Release, Windows Release und macOS Release.

PR #77 wurde mit dem normalen Merge-Commit
`3a3ca0926c00b5ddd4e687952cc2f8fb2d0fdde3` gemergt. Die Post-Merge-Validierung für genau diesen
Merge-SHA war vollständig erfolgreich: Main CI einschließlich der vollständigen Linux-/Windows- und
macOS-Release-Matrix, GTK Native CI und AppKit Native CI. Der Feature-Branch wurde lokal und remote
gelöscht; anschließend wurde `fetch --prune` ausgeführt, `main` auf den Merge-SHA synchronisiert
und ein sauberer Working Tree bestätigt. Die PRs #30, #32 und #63 wurden nicht verändert. Es wurde
kein Release und kein Tag erzeugt. Das macOS/Cocoa-M3-Gate bleibt offen. M8 bleibt aktiv;
Property-Coverage-Erweiterung, Tree-Snapshots, Persistenzformate, Serialisierung, Deserialisierung,
Factories, Validierung, Design-Time-Metadaten, Ressourcen, Designer und IDE-Integration wurden nicht
begonnen.
