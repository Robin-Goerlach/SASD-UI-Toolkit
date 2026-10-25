# ADR 0183: M8 ComboBox Collection and Optional Property Coverage

- Status: Accepted
- Date: 2026-10-25

## English

### Context

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) identifies two real
ComboBox persistence gaps: the owned items and optional committed selection. The existing typed
schema/access/default/snapshot pipeline covers 19 descriptors with four concrete value kinds.

### Decision

Append `string_collection` and `optional_unsigned_integer` without changing existing enum values.
Declare exactly `ComboBox.items` then `ComboBox.selectedIndex` on `sasd.ui.ComboBox`; inherited
Widget properties remain declared only by Widget. The catalog now has 21 descriptors and six kinds.
There is no generic collection interface, PropertyValue, variant, any, reflection engine or dependency.

Runtime collection reads copy `itemCount()` / `itemAt()` into owning `vector<string>`. No borrowed
string_view escapes. Writes delegate exclusively to `setItems()`, preserving invalidation, preview
clearing, selection clearing and SelectionChanged(nullopt) on real replacement of selected content.
Same-value replacement leaves selection intact. No callback suppression or special construction phase
is introduced. Property writer true means recognized compatible access, not the setter's changed bool.
After a setter may invoke application code, the bridge never accesses the Component again.

Optional unsigned reads and default queries return `optional<optional<uint64_t>>`: outer nullopt
means invalid access/query (or unrepresentable read), engaged outer with empty inner means valid none,
and an engaged inner holds an index. A single optional would conflate failure with semantic absence.
No UINT64_MAX, zero or other sentinel represents none. Reads check size_t -> uint64_t and writes
check uint64_t -> size_t before conversion. Representation failure returns nullopt/false without
mutation. Domain validation stays in `setSelectedIndex()`; representable out-of-range indices throw
std::out_of_range through the bridge. Same-value writes return true but trigger no extra callback.
Selection clearing delegates to the same setter; ordinary selection-change callbacks remain active.

Defaults are queried without constructing a ComboBox: items is an engaged empty immutable span of
string_view (allocation-free and noexcept); selectedIndex is engaged outer with empty inner.
Full descriptor identity (declaring type, local ID and kind), compatible targets and exact case
remain required; copied valid descriptors are accepted.

Separate owned snapshot structures add `string_collection_properties` with vector<string> values
and `optional_unsigned_integer_properties` with optional<uint64_t> values. Here one optional is
sufficient because capture already succeeded. Missing reads/defaults or inconsistent descriptors
reject the whole snapshot, never a partial list. Empty items and absent selection are defaults;
non-empty items and an index are non-default. Replacing items captures the cleared default selection.
Snapshots own identities and strings after mutation or destruction of the source Component.

Future reconstruction must restore ComboBox.items before selectedIndex. The index is validated
against current items and setItems clears selection, so the reverse order is semantically wrong.
Kind-separated snapshot vectors do not encode global write order. This dependency is documented
only: no reconstruction ordering engine, restore transaction, factory, serializer, encoding or
deserialization is implemented.

selectedText is derived from items + selectedIndex, never independently persisted. previewIndex,
previewText, dropDownOpen, pressed and callbacks are excluded as transient/executable state.
Widget.sizeConstraints remains an uncovered compound candidate; TextStyle belongs to later
style/resource policy. Neither property capture nor the completed ComboBox coverage implies full
UI-description serialization coverage. The macOS/Cocoa M3 visible-validation gate remains open.

### Local validation

Strict GCC 12.2 / WSL Debug and Release builds with warnings as errors passed, including examples.
Both full CTest runs passed all four suites (475 Core, 321 Terminal, 129 Rendered tests and native
POSIX PTY smoke). The focused Debug property/ComboBox program and independently configured Release
consumer of the installed public package each passed 66 tests. Both headless demos printed owned
items and selectedIndex values. Descriptor/enum compatibility, the complete 29-type ADR-0177 list,
relative document links and diff whitespace were checked. Other platform/toolchain checks belong
to CI; visible macOS/Cocoa M3 validation remains separate.

## Deutsch

### Kontext und Entscheidung

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) identifiziert die offenen
ComboBox-Werte items und selectedIndex. Die bestehende typisierte Pipeline wird um genau
`string_collection` und `optional_unsigned_integer` erweitert, hinten an die Enum angehängt.
ComboBox deklariert items vor selectedIndex; Widget-Properties werden nicht dupliziert.
Der Katalog wächst von 19 auf 21 Properties und von vier auf sechs konkrete Kinds.
Keine generische Collection-/PropertyValue-Schicht, variant, any, Reflection oder neue Dependency.

Collection-Reads kopieren itemCount()/itemAt() in besitzende vector<string>; keine Views entkommen.
Writes nutzen ausschließlich setItems(): echte Ersetzung löscht Selection und Preview, invalidiert
und kann SelectionChanged(nullopt) auslösen. Gleiche Items erhalten die Auswahl. setSelectedIndex()
bleibt allein für Domain-Validierung verantwortlich; std::out_of_range propagiert unverändert.
size_t/uint64_t-Konvertierungen werden in beiden Richtungen geprüft; Repräsentationsfehler liefern
nullopt/false ohne Mutation. Writer-true bedeutet gültigen Zugriff, nicht Änderung. Gleiche Werte
liefern true ohne zusätzliche Setter-Callbacks. Keine Callback-Unterdrückung; kein Memberzugriff
nach einem Setter, dessen Application-Callback die Component zerstören darf.

Reads und Defaults für optionale Indizes verwenden optional<optional<uint64_t>>: außen nullopt
bedeutet ungültigen Zugriff/Query oder nicht darstellbaren Read; außen engaged und innen leer ist
gültig ohne Auswahl; innen ein Wert ist die Auswahl. Kein Sentinel repräsentiert none. Defaults
benötigen keine Component-Konstruktion: items ist ein gültiger leerer immutable string_view-Span,
allokationsfrei/noexcept; selectedIndex ist gültig mit innerem nullopt. Vollständige Descriptor-
Identität und Groß-/Kleinschreibung bleiben strikt; gültige Descriptor-Kopien werden akzeptiert.

Getrennte besitzende Snapshot-Strukturen speichern vector<string> bzw. optional<uint64_t> in
string_collection_properties bzw. optional_unsigned_integer_properties. Ein Optional reicht dort,
weil der Capture bereits erfolgreich war. Ungültige Reads/Defaults/Descriptoren verwerfen atomar
den gesamten Snapshot. Leere Items und keine Auswahl sind Defaults; echte Items/Selection nicht.
Nach Ersetzung bildet der Snapshot die gelöschte Selection ab. Strings überleben Mutation/Zerstörung.

Zukünftige Rekonstruktion muss ComboBox.items vor selectedIndex wiederherstellen: der Index wird
gegen aktuelle Items validiert und setItems löscht Selection. Getrennte Kind-Vektoren garantieren
keine globale Write-Reihenfolge. Die Abhängigkeit ist ausschließlich dokumentiert, nicht implementiert:
keine Ordering-Engine, Restore-Transaktion, Factory, Serialisierung, Encoding oder Deserialisierung.
selectedText bleibt abgeleitet und nicht separat persistiert; previewIndex, previewText, dropDownOpen,
pressed und Callbacks bleiben transient/ausführbar und ausgeschlossen. Widget.sizeConstraints bleibt
offener Compound-Kandidat, TextStyle spätere Style-/Resource-Policy. Vollständige Serialisierung und
das sichtbare macOS-/Cocoa-M3-Gate bleiben offen.

### Lokale Validierung

Strikte GCC-12.2-/WSL-Builds in Debug und Release mit Warnings-as-Errors und Beispielen bestanden.
Beide vollständigen CTest-Läufe bestanden alle vier Suites (475 Core-, 321 Terminal-, 129 Rendered-
Tests und nativer POSIX-PTY-Smoke). Das fokussierte Debug-Property-/ComboBox-Programm und der separat
konfigurierte Release-Consumer des installierten Public-Pakets bestanden jeweils 66 Tests. Beide
headless Demos zeigten besitzende Items und selectedIndex. Descriptor-/Enum-Kompatibilität,
vollständige 29-Typen-Liste in ADR 0177, relative Dokumentlinks und Diff-Whitespace wurden geprüft.
Andere Plattformen/Toolchains werden über CI geprüft; sichtbare macOS-/Cocoa-M3-Validierung bleibt separat.
