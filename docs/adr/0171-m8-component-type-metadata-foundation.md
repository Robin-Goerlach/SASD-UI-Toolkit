# ADR 0171: M8 Component Type Metadata Foundation

## Status

Accepted

## Predecessors

[0003](0003-public-naming-and-namespaces.md),
[0004](0004-component-model-and-ownership.md),
[0005](0005-backend-peer-and-capabilities.md),
[0170](0170-m7-desktop-integration-exit-review.md)

## English

### Context

M8 needs a durable foundation for developer tools, but a first metadata slice must not silently
become reflection, serialization or a designer API. The current Core already has a meaningful
Component hierarchy: non-visual Components, Widgets, Containers, controls, layouts and model
Components. Tooling needs to identify those types without depending on compiler ABI details or a
native backend.

### Decision

The Core exposes `sasd/ui/component_metadata.hpp` with three small pieces of information:

- `ComponentTypeMetadata::type_id` is an explicit, stable, case-sensitive toolkit ID such as
  `sasd.ui.Button`.
- `ComponentTypeMetadata::base_type_id` names the direct SASD Component metadata base, or is empty
  only for `sasd.ui.Component`.
- `ComponentTypeMetadata::kind` is one of `component`, `widget` or `container`.

The IDs are non-localized and backend-neutral. They are not derived from `typeid(T).name()`, compiler
demangling, `__PRETTY_FUNCTION__`, `__FUNCSIG__` or any other implementation-specific spelling.
The direct-base representation avoids duplicating complete ancestor lists while still allowing a
consumer to walk the catalog.

`builtInComponentTypes()` returns a read-only `std::span` over immutable static descriptors, and
`findBuiltInComponentType()` performs an exact case-sensitive query. Unknown IDs return `nullptr`.
The current catalog includes every public Component-derived type implemented in Core, including
abstract model bases and concrete StringListModel, StringTableModel and StringTreeModel types. It
currently contains 25 descriptors; types described only in long-term documentation are not invented.

The catalog is a type catalog, not an instance-reflection mechanism. No virtual metadata method is
added to `Component`, so an unknown external C++ class derived from Button, Widget or Component
cannot be misreported as its base type by a default implementation. Runtime instance introspection
is deferred until a precise extension contract exists.

### Implementation and validation record

The foundation was implemented in commit `3888e8f`. The follow-up commit `553935e` added
target-local AddressSanitizer and UndefinedBehaviorSanitizer link options for the headless metadata
demo after the first sanitizer run exposed an incomplete executable link configuration. The final
implementation head reviewed before this documentation follow-up was
`553935edf034d3d4befa50de0fcfc55503bbf26c` in PR #64.

PR #64 then received one authorized documentation-only commit,
`2e5459f94fa7c6453dd658ae9561da57e24fcab8` (`docs: record M8 metadata validation`). Compared with
`553935e`, that commit changes only this ADR by adding 30 lines and deleting none; it does not alter
production code, public API, tests, CMake, workflows or backend code. This is the final PR head and
remains within M8 Slice 1.

For that exact implementation head, the required Linux GCC/Clang, sanitizer, SDL3, Windows MSVC,
macOS AppleClang, GTK Native, AppKit Native and Linux/Windows/macOS Release checks passed. Local
strict Debug and Release builds, CTest and the headless demo also passed. The WSL environment did
not provide `clang++`, so no local sanitizer result is claimed; the sanitizer result above is from
CI. No visible native GUI or manual macOS validation is claimed by this record.

PR #64 was merged with a normal merge commit as
`b356a473d364e30644679e7b16ecf1a9ec87af76`. For that exact merge SHA, Main CI including the complete
Linux/Windows/macOS Release matrix, GTK Native CI and AppKit Native CI passed. The local `main` was
fast-forwarded to the merge SHA, the merged feature branch was deleted remotely, and the working tree
was left clean. No release or tag was created by this slice.

### Consequences and explicit boundaries

The catalog is immutable, deterministic, backend-neutral and free of mutable global registration or
hidden initialization side effects. It has no ownership relationship with Components and does not
change visual parenting, events, layout, presentation, lifetime or the Terminal first-class
contract. A headless example consumes the same public header and Core library without opening a
window or initializing a backend.

This slice deliberately does not add:

- property metadata, setters, validation attributes or design-time values;
- factories, construction, cloning or plugin registration;
- JSON/XML/YAML or any other UI serialization;
- resource/localization strings, icons or designer/toolbox information;
- native peer, backend capability or platform-specific metadata.

Built-in catalog first is the decision; third-party extension and factory registration remain
deferred. Later M8 slices may build on the stable type IDs, but must define their own contracts
instead of expanding this descriptor speculatively.

## Deutsch

### Kontext

M8 benötigt eine belastbare Grundlage für Developer-Tools. Der erste Metadaten-Slice darf jedoch
nicht unbemerkt zu Reflection, Serialisierung oder einer Designer-API anwachsen. Der aktuelle Core
besitzt bereits eine aussagekräftige Component-Hierarchie aus nichtvisuellen Components, Widgets,
Containern, Controls, Layouts und Model-Components. Tooling muss diese Typen erkennen können, ohne
von Compiler-ABI-Details oder einem nativen Backend abzuhängen.

### Entscheidung

Der Core stellt mit `sasd/ui/component_metadata.hpp` drei kleine Informationen bereit:

- `ComponentTypeMetadata::type_id` ist eine explizite, stabile, case-sensitive Toolkit-ID wie
  `sasd.ui.Button`.
- `ComponentTypeMetadata::base_type_id` bezeichnet die direkte SASD-Component-Metadatenbasis oder
  ist nur bei `sasd.ui.Component` leer.
- `ComponentTypeMetadata::kind` ist eine der Klassifikationen `component`, `widget` oder
  `container`.

Die IDs sind nicht lokalisiert und backendneutral. Sie werden nicht aus `typeid(T).name()`,
Compiler-Demangling, `__PRETTY_FUNCTION__`, `__FUNCSIG__` oder einer anderen
implementierungsabhängigen Schreibweise abgeleitet. Die direkte Basisdarstellung vermeidet
duplizierte vollständige Ahnenlisten und erlaubt einem Consumer dennoch, den Katalog abzulaufen.

`builtInComponentTypes()` liefert einen read-only-`std::span` über unveränderliche statische
Deskriptoren; `findBuiltInComponentType()` führt eine exakte case-sensitive Abfrage aus. Unbekannte
IDs liefern `nullptr`. Der aktuelle Katalog enthält jeden öffentlichen, im Core implementierten
Component-Untertyp, einschließlich abstrakter Model-Basen und der konkreten Typen StringListModel,
StringTableModel und StringTreeModel. Er enthält derzeit 25 Deskriptoren; nur langfristig
dokumentierte, noch nicht implementierte Typen werden nicht erfunden.

Der Katalog ist ein Typkatalog und keine Instanz-Reflection. `Component` erhält keine virtuelle
Metadatenmethode. Dadurch kann eine unbekannte externe C++-Klasse, die von Button, Widget oder
Component erbt, nicht durch eine Default-Implementierung fälschlich als Basistyp gemeldet werden.
Runtime-Instanzabfragen bleiben zurückgestellt, bis ein präziser Erweiterungsvertrag existiert.

### Implementierungs- und Validierungsprotokoll

Die Grundlage wurde in Commit `3888e8f` implementiert. Der Folgecommit `553935e` ergänzte lokale
Link-Optionen für AddressSanitizer und UndefinedBehaviorSanitizer am headless Metadata-Demo-Target,
nachdem der erste Sanitizer-Lauf eine unvollständige Link-Konfiguration des Executables sichtbar
gemacht hatte. Der vor diesem Dokumentations-Follow-up geprüfte Implementierungsstand war
`553935edf034d3d4befa50de0fcfc55503bbf26c` in PR #64.

PR #64 erhielt danach einen autorisierten, ausschließlich dokumentarischen Commit,
`2e5459f94fa7c6453dd658ae9561da57e24fcab8` (`docs: record M8 metadata validation`). Gegenüber
`553935e` änderte dieser Commit ausschließlich dieses ADR mit 30 zusätzlichen Zeilen und ohne
Löschungen; Produktionscode, öffentliche API, Tests, CMake, Workflows und Backend-Code blieben
unverändert. Dieser Commit ist der finale PR-Head und bleibt innerhalb von M8 Slice 1.

Für diesen exakten Implementierungsstand waren die erforderlichen Checks für Linux GCC/Clang,
Sanitizer, SDL3, Windows MSVC, macOS AppleClang, GTK Native, AppKit Native sowie Linux/Windows/macOS
Release erfolgreich. Die lokalen strikten Debug- und Release-Builds, CTest und das headless Demo
waren ebenfalls erfolgreich. In der WSL-Umgebung war kein `clang++` vorhanden; deshalb wird kein
lokales Sanitizer-Ergebnis behauptet, sondern das oben genannte CI-Ergebnis als Nachweis verwendet.
Eine sichtbare native GUI-Prüfung oder manuelle macOS-Validierung wird durch diesen Eintrag nicht
behauptet.

PR #64 wurde mit dem normalen Merge-Commit
`b356a473d364e30644679e7b16ecf1a9ec87af76` zusammengeführt. Für genau diesen Merge-SHA waren Main
CI einschließlich der vollständigen Linux-/Windows-/macOS-Release-Matrix, GTK Native CI und AppKit
Native CI erfolgreich. Der lokale `main` wurde auf diesen Merge-SHA vorgespult, der gemergte
Feature-Branch wurde remote gelöscht und der Working Tree blieb sauber. Durch diesen Slice wurden
kein Release und kein Tag erzeugt.

### Folgen und ausdrückliche Grenzen

Der Katalog ist unveränderlich, deterministisch, backendneutral und frei von veränderbarer globaler
Registrierung oder versteckten Initialisierungseffekten. Er besitzt keine Ownership-Beziehung zu
Components und verändert weder visuelles Parenting noch Events, Layout, Presentation, Lifetime oder
den First-Class-Vertrag des Terminals. Ein headless Beispiel verwendet denselben öffentlichen Header
und die Core-Bibliothek, ohne ein Fenster zu öffnen oder ein Backend zu initialisieren.

Dieser Slice fügt ausdrücklich nicht hinzu:

- Property-Metadaten, Setter, Validierungsattribute oder Design-Time-Werte;
- Factories, Konstruktion, Cloning oder Plugin-Registrierung;
- JSON/XML/YAML oder eine andere UI-Serialisierung;
- Resource-/Lokalisierungsstrings, Icons oder Designer-/Toolbox-Informationen;
- Native-Peer-, Backend-Capability- oder plattformspezifische Metadaten.

Zuerst gilt der Built-in-Katalog; Erweiterungs- und Factory-Registrierung für Drittanbieter bleiben
zurückgestellt. Spätere M8-Slices dürfen auf den stabilen Typ-IDs aufbauen, müssen aber eigene
Verträge definieren und diese Descriptor-Spezifikation nicht spekulativ vergrößern.
