# ADR 0193: M8 Property Authoring Category Metadata Foundation

- Status: Accepted
- Date: 2026-10-30

## English

### Context

M8 now has separate instance-free contracts for built-in property schema, effective construction
defaults, runtime value access and complete validation metadata. A future Property Inspector or
designer can discover what a property is and which values are valid, but it cannot group the 28
current descriptors by authoring purpose without parsing identifiers or maintaining a competing
SASD-specific table.

Human-facing names, descriptions and editor choices are not yet stable Core contracts. They require
localization and real designer-consumer experience. Freezing English labels such as "Size
Constraints" now would mix presentation policy into backend- and format-neutral metadata. Stable
machine-readable categories are a smaller useful foundation that does not decide those later UX
questions.

### Decision

Add the public `sasd/ui/component_property_authoring_metadata.hpp` layer, separate from
`ComponentPropertyMetadata`. Schema metadata remains the single truth about which properties exist
and their value kinds. Authoring metadata describes only a semantic grouping for a valid exact
built-in descriptor. It neither extends Component instances nor participates in runtime access,
validation, snapshots or persistence.

`ComponentPropertyAuthoringCategory` contains five non-localized semantic IDs:

- `appearance`: pure visual presentation/style, currently the six `textStyle` descriptors;
- `behavior`: participation in behavior or interaction: `Widget.visible`, `Widget.enabled`,
  `Widget.focusable` and `Command.enabled`;
- `content`: author-provided content: the six `text` descriptors and `ComboBox.items`;
- `layout`: geometry/constraints/spacing: `Widget.sizeConstraints`, both Box spacing descriptors,
  Grid column count/spacings and Form spacings;
- `state`: persisted initial control/selection state: `CheckBox.checked`,
  `RadioButton.selected` and `ComboBox.selectedIndex`.

The distinction between behavior and state is intentional. Visible/enabled/focusable configure
participation, while checked/selected/selectedIndex are explicit initial state. Tools may localize
and order categories independently; enum ordinals are not display order, ABI or persisted values.

`builtInComponentPropertyAuthoringMetadata()` accepts a `ComponentPropertyMetadata` value and
returns `std::optional<ComponentPropertyAuthoringMetadata>`. Identity is the complete
declaring-type/property/value-kind triple, not pointer identity. A copied valid descriptor succeeds;
unknown, malformed, wrong-kind, wrong-case and fabricated descriptors fail closed.

Inherited properties retain their declaring descriptor and therefore its category. For example,
Button lookup of `visible` returns `Widget.visible` / behavior. No Button-, Label- or CheckBox-
specific inherited duplicates are created. `declaredBuiltInComponentProperties()` remains the only
public enumeration source; there is no second public property catalog.

The implementation uses one private explicit mapping for all 28 descriptors. It does not infer
categories from names. Complete-catalog tests require every schema descriptor to have an engaged
result, forcing each future property addition to make a deliberate authoring decision. Current
regression counts are appearance 6, behavior 4, content 7, layout 8 and state 3.

### Consequences and boundaries

Tooling can group properties without constructing Components, parsing names or depending on JSON.
The query allocates nothing and remains backend- and format-neutral. Categories are not written to
UI-description JSON; format version, canonical bytes, parser, document model and reconstruction are
unchanged. Runtime controls, access/default/validation metadata and reference metadata are also
unchanged.

No display name, description, tooltip, localization key, editor kind, category order, browsable,
read-only, advanced or similar speculative field is added. All current properties are authorable,
and no concrete built-in case justifies those flags. Value kind plus validation metadata already
supports many generic editor choices; a preferred-editor contract waits for a real consumer gap.

This slice does not implement a Property Grid, visual designer, IDE adapter, resource system or
reference-authoring UX. Designer-friendly properties and the enclosing property-metadata/design-time
roadmap item remain open for human-facing/tooling attributes beyond these categories.

## Deutsch

### Kontext

M8 besitzt nun getrennte instanzfreie Verträge für Built-in-Property-Schema, effektive
Konstruktions-Defaults, Runtime-Wertzugriff und vollständige Validation Metadata. Ein künftiger
Property Inspector oder Designer kann erkennen, was eine Property ist und welche Werte gültig sind,
kann die 28 heutigen Deskriptoren jedoch nicht nach Authoring-Zweck gruppieren, ohne Identifiers zu
parsen oder eine konkurrierende SASD-spezifische Tabelle zu pflegen.

Menschenlesbare Namen, Beschreibungen und Editorentscheidungen sind noch keine stabilen Core-
Verträge. Sie benötigen Localization und Erfahrung mit einem realen Designer-Consumer. Englische
Labels wie "Size Constraints" jetzt festzuschreiben würde Presentation-Policy in backend- und
formatneutrale Metadata mischen. Stabile maschinenlesbare Kategorien sind eine kleinere nützliche
Grundlage, die diese späteren UX-Fragen nicht vorwegnimmt.

### Entscheidung

Die öffentliche Schicht `sasd/ui/component_property_authoring_metadata.hpp` wird getrennt von
`ComponentPropertyMetadata` ergänzt. Schema Metadata bleibt die einzige Wahrheit darüber, welche
Properties existieren und welche Value Kinds sie besitzen. Authoring Metadata beschreibt nur eine
semantische Gruppierung für einen gültigen exakten Built-in-Deskriptor. Sie erweitert weder
Component-Instanzen noch nimmt sie an Runtime-Zugriff, Validation, Snapshots oder Persistenz teil.

`ComponentPropertyAuthoringCategory` enthält fünf nicht lokalisierte semantische IDs:

- `appearance`: reine visuelle Darstellung/Stil, heute die sechs `textStyle`-Deskriptoren;
- `behavior`: Teilnahme an Verhalten oder Interaktion: `Widget.visible`, `Widget.enabled`,
  `Widget.focusable` und `Command.enabled`;
- `content`: vom Autor bereitgestellter Inhalt: die sechs `text`-Deskriptoren und `ComboBox.items`;
- `layout`: Geometrie/Constraints/Abstände: `Widget.sizeConstraints`, beide Box-Spacing-
  Deskriptoren, Grid-Spaltenzahl/-Abstände und Form-Abstände;
- `state`: persistierter initialer Control-/Selection-Zustand: `CheckBox.checked`,
  `RadioButton.selected` und `ComboBox.selectedIndex`.

Die Trennung von Behavior und State ist bewusst. Visible/enabled/focusable konfigurieren Teilnahme,
während checked/selected/selectedIndex expliziten initialen Zustand beschreiben. Tools dürfen
Kategorien unabhängig lokalisieren und anordnen; Enum-Ordinals sind weder Display-Reihenfolge noch
ABI- oder Persistenzwerte.

`builtInComponentPropertyAuthoringMetadata()` akzeptiert einen `ComponentPropertyMetadata`-Wert und
liefert `std::optional<ComponentPropertyAuthoringMetadata>`. Identität ist das vollständige
Declaring-Type-/Property-/Value-Kind-Tripel, nicht Pointer-Identität. Ein kopierter gültiger
Deskriptor funktioniert; unbekannte, fehlerhafte, Wrong-Kind-, Wrong-Case- und fabrizierte
Deskriptoren arbeiten fail-closed.

Geerbte Properties behalten ihren deklarierenden Deskriptor und damit dessen Kategorie. Der Button-
Lookup für `visible` liefert beispielsweise `Widget.visible` / behavior. Es entstehen keine Button-,
Label- oder CheckBox-spezifischen geerbten Duplikate. `declaredBuiltInComponentProperties()` bleibt
die einzige öffentliche Enumerationsquelle; es gibt keinen zweiten öffentlichen Property-Katalog.

Die Implementierung verwendet eine private explizite Zuordnung für alle 28 Deskriptoren. Sie leitet
Kategorien nicht aus Namen ab. Complete-Catalog-Tests verlangen für jeden Schema-Deskriptor ein
vorhandenes Ergebnis und zwingen damit jede künftige Property-Ergänzung zu einer bewussten Authoring-
Entscheidung. Die heutigen Regression Counts sind Appearance 6, Behavior 4, Content 7, Layout 8 und
State 3.

### Folgen und Grenzen

Tooling kann Properties gruppieren, ohne Components zu konstruieren, Namen zu parsen oder von JSON
abzuhängen. Die Abfrage alloziert nichts und bleibt backend- und formatneutral. Kategorien werden
nicht in UI-Description-JSON geschrieben; Formatversion, kanonische Bytes, Parser, Dokumentmodell
und Reconstruction bleiben unverändert. Runtime-Controls, Access-/Default-/Validation-Metadata und
Reference Metadata bleiben ebenfalls unverändert.

Es entstehen weder Display Name, Description, Tooltip, Localization Key, Editor Kind,
Category Order, Browsable-, Read-only-, Advanced- noch ähnliche spekulative Felder. Alle heutigen
Properties sind authorable; kein konkreter Built-in-Fall begründet diese Flags. Value Kind und
Validation Metadata ermöglichen bereits viele generische Editorentscheidungen; ein Preferred-
Editor-Vertrag wartet auf eine reale Consumer-Lücke.

Dieser Slice implementiert weder Property Grid, visuellen Designer, IDE Adapter, Resource-System
noch Reference-Authoring-UX. Designerfreundliche Properties und der umschließende Roadmap-Punkt für
Property Metadata/Design-Time bleiben für menschenlesbare und weitere Tooling-Attribute offen.
