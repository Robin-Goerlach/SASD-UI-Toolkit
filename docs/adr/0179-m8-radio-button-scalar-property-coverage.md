# ADR 0179: M8 RadioButton Scalar Property Coverage

- Status: Accepted
- Date: 2026-10-23
- Predecessors: [0177](0177-m8-serializable-ui-description-state-coverage-policy.md),
  [0178](0178-m8-numeric-property-value-kind-foundation.md)

## Context and decision

Slice 9 closes the persistent scalar gap identified in ADR 0177 without new value kinds.
Add exactly two direct descriptors, in order: `sasd.ui.RadioButton` / `text` / `string`, then
`sasd.ui.RadioButton` / `selected` / `boolean`. The catalog grows from 17 to 19. Widget properties
remain inherited. Full descriptor identity is checked; copied valid descriptors are accepted.

Reuse existing typed access, effective defaults and owned snapshot collections. Reads use text()
and isSelected(); writes exclusively use setText()/setSelected(). The selection setter preserves
RadioGroup exclusivity, coherent group state and permitted empty selection. Its bool reports state
change; the property writer bool reports valid compatible access, including same-value writes.
Existing false-to-true onSelected callbacks remain active without a suppression layer. No member
access follows the setter, because a callback may destroy the button.

Defaults are immutable source-derived constants: text="", selected=false. No instance is constructed.
Widget.focusable retains its RadioButton override of true. Base-to-derived snapshots contain three
inherited booleans, selected and owned text with default flags; numeric collections remain empty.

## Boundaries and consequences

RadioButton -> RadioGroup membership remains a persistent structural/reference relation without
technical coverage. RadioGroup.selectedButton is derived group state, not an independent persistent
property. A snapshot can capture selected=true without reconstructing a group. A selected=true and
B selected=false do not say that both belong to G. Future deserialization must define membership/
reference reconstruction and ordering before grouped selection forms a fully reconstructable UI
description. This slice implements no reconstruction policy.

No serializer, deserializer, factory, reference IDs, graph metadata, callback suppression,
collection/optional kinds or ComboBox coverage are added.

## Local validation

Existing WSL/GCC environment: strict Debug and Release builds with warnings as errors passed.
The focused Core suite and all four configured CTest suites (Core, Terminal, Rendered, POSIX PTY)
passed; both headless component metadata demos displayed the expected RadioButton snapshot.
Descriptor count is 19; changed-document relative links and git diff --check passed.
Native platform/SDL3 coverage is delegated to the unchanged CI matrix.

## Deutsch

### Kontext und Entscheidung

Slice 9 schließt die in ADR 0177 benannte skalare RadioButton-Lücke ohne neue Value Kinds.
Genau zwei direkte Deskriptoren kommen in dieser Reihenfolge hinzu: `sasd.ui.RadioButton` / `text` /
`string`, danach `sasd.ui.RadioButton` / `selected` / `boolean`. Der Katalog wächst von 17 auf 19.
Widget-Properties bleiben geerbt. Die komplette Identität wird geprüft; gültige Kopien funktionieren.

Bestehende Access-/Default-/Snapshot-Pfade werden verwendet. Getter sind text()/isSelected(); Writes
verwenden ausschließlich setText()/setSelected(). Gruppenexklusivität, konsistenter Gruppenstatus
und erlaubte leere Auswahl bleiben erhalten. Setter-bool bedeutet Zustandsänderung, Writer-bool
gültigen kompatiblen Zugriff; Same-Value-Writes sind erfolgreich. onSelected bleibt bei false->true
aktiv, ohne Callback-Unterdrückung. Nach dem Setter erfolgt kein Objektzugriff, da der Callback
den Button zerstören darf.

Defaults sind ohne Instanzerzeugung text="" und selected=false; Widget.focusable bleibt true.
Base->Derived-Snapshots enthalten drei geerbte Booleans, selected und besitzenden Text mit
Default-Markierungen; numerische Collections bleiben leer.

### Grenzen und Folgen

RadioButton -> RadioGroup-Mitgliedschaft bleibt eine persistente Struktur-/Referenzbeziehung ohne
technische Abdeckung. RadioGroup.selectedButton bleibt abgeleitet, keine unabhängige persistente
Property. A selected=true und B selected=false beschreiben ohne Zugehörigkeit zu G keine komplette
Gruppe. Spätere Deserialisierung muss Mitgliedschaft/Reference Reconstruction und deren Reihenfolge
definieren, bevor gruppierte Selection vollständig rekonstruierbar ist. Ein skalarer Snapshot reicht
nicht aus; dieser Slice implementiert keine Reconstruction Policy.

Kein Serializer, Deserializer, keine Factory, Reference IDs, Graph-Metadaten, Callback-Suppression,
Collection-/Optional-Kinds oder ComboBox-Abdeckung werden ergänzt.

### Lokale Validierung

Strikte Debug-/Release-Builds mit Warnings-as-Errors in der vorhandenen WSL/GCC-Umgebung waren
erfolgreich. Fokussierte Core-Tests, alle vier CTest-Suites (Core, Terminal, Rendered, POSIX PTY)
und beide headless Metadaten-Demos bestanden. Katalogzahl 19, relative Dokumentlinks und
Diff-Whitespace wurden geprüft. Native Plattformen/SDL3 werden durch die unveränderte CI geprüft.
