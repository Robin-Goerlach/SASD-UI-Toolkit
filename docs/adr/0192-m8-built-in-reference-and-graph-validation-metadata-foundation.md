# ADR 0192: M8 Built-in Reference and Graph Validation Metadata Foundation

- Status: Accepted
- Date: 2026-10-29

## English

### Context

ADRs 0189 through 0191 made scalar domains, single-property policies and the first same-node
cross-property relation available as instance-free metadata. The format-neutral document validator
still contained two separate built-in reference truths: an eleven-entry source/target table and a
hardcoded RadioGroup-exclusivity branch. The JSON v1 parser repeated the source/target IDs even
though parsing requires only the encoding-specific symbol-to-kind mapping.

Reference compatibility is neither a property-local rule nor JSON syntax. It is a stable relation
contract useful to document validation and future tooling. RadioGroup exclusivity is an additional
graph constraint over several source nodes, their reference edges, one target and a persistent
source property. Keeping those two levels distinct allows every reference kind to have exactly one
endpoint contract and zero or more graph rules without introducing a general rule engine.

### Decision

Add the public, backend- and format-neutral `component_reference_metadata.hpp` layer. Its immutable
built-in catalog contains exactly these eleven entries in established deterministic order:

| Reference kind | Source contract | Target contract |
| --- | --- | --- |
| `button_command` | `sasd.ui.Button` | `sasd.ui.Command` |
| `radio_button_group` | `sasd.ui.RadioButton` | `sasd.ui.RadioGroup` |
| `list_view_model` | `sasd.ui.ListView` | `sasd.ui.ListModel` |
| `table_view_model` | `sasd.ui.TableView` | `sasd.ui.TableModel` |
| `tree_view_model` | `sasd.ui.TreeView` | `sasd.ui.TreeModel` |
| `list_view_selection_model` | `sasd.ui.ListView` | `sasd.ui.ListSelectionModel` |
| `table_view_selection_model` | `sasd.ui.TableView` | `sasd.ui.TableSelectionModel` |
| `tree_view_selection_model` | `sasd.ui.TreeView` | `sasd.ui.TreeSelectionModel` |
| `list_selection_model_model` | `sasd.ui.ListSelectionModel` | `sasd.ui.ListModel` |
| `table_selection_model_model` | `sasd.ui.TableSelectionModel` | `sasd.ui.TableModel` |
| `tree_selection_model_model` | `sasd.ui.TreeSelectionModel` | `sasd.ui.TreeModel` |

`builtInComponentReferenceMetadata()` exposes the catalog and
`findBuiltInComponentReferenceMetadata()` performs fail-closed lookup. A fabricated enum returns
`nullptr`; there is no fallback relation. Actual endpoints are compatible when their exact built-in
type equals or derives through the existing metadata base chain from the declared contract. This is
required for concrete StringList/Table/TreeModel targets under their abstract model contracts.

Every built-in reference kind is an optional singular relation per source and kind. A source can
have no target or exactly one target of one kind, while independent kinds can coexist. This current
universal contract needs no speculative cardinality enum. Catalog order is deterministic tooling
order, not a persistent numeric enum-ordinal contract.

Graph validation is represented separately by
`BuiltInComponentReferenceGraphValidationMetadata` and the zero-or-more query
`builtInComponentReferenceGraphValidations()`. The sole current rule is:

- reference: `radio_button_group`;
- kind: `at_most_one_source_boolean_true_per_target`;
- source property: the complete `sasd.ui.RadioButton.selected` / `boolean` catalog descriptor.

For each RadioGroup target, at most one incoming group edge may have a source whose exact selected
property is true. Any number may be false. Standalone selected RadioButtons do not participate;
ownership or adjacency never implies membership. Different targets have independent counts.
Validation only accepts or rejects and never clears, selects or normalizes a member.

The common document validator now has two explicit passes. The first resolves reference metadata,
checks endpoints, self-reference, base-chain source/target compatibility and the singular
source-plus-kind contract. The second evaluates graph metadata only on type-valid edges. Missing
metadata, a missing exact source property or inconsistent metadata fails closed as
`invalid_reference`. The former local source/target table, `selected_group` state and hardcoded
`radio_button_group` branch are removed.

Canonical JSON v1 symbols remain encoding-specific. The parser table now stores only symbol and
reference kind; all eleven spellings and the writer's `referenceSymbol()` mapping remain unchanged.
There is no format-version, document-shape, error-code or canonical-byte change.

Runtime binding APIs remain authoritative. Capture continues to use typed getters, and
reconstruction continues to use its concrete typed operations, constructor-based RadioButton
membership and defensive casts. Metadata describes compatibility, not reflective invocation or
restore scheduling.

### Consequences and boundaries

Tooling can discover valid source/target contracts and filter candidate targets without constructing
Components. It can also discover that RadioGroup membership constrains the selected property across
sources. This slice implements no designer, picker, auto-fix or editor synchronization.

No graph DSL, expression AST, generic `ValidationRule`, predicate callback, `std::function`, variant
rule value, mutable registry, runtime reflection engine or reference-setter invocation layer is
introduced. JSON symbols do not leak into general metadata.

The validation coverage review found no further current built-in semantic rule that belongs in the
M8 property/reference validation-metadata families. Dense IDs, ownership validity, UTF-8, exact
schema completeness and typed property completeness are general document infrastructure. Atomic
TextStyle color validity remains part of its concrete compound value rather than a separate property
descriptor family. The roadmap's contextual cross-property/graph-validation and validation-metadata
subtree can therefore close. Designer-friendly property attributes, resources, IDE integration and
the enclosing property-metadata/design-time milestone remain open.

## Deutsch

### Kontext

ADRs 0189 bis 0191 machten Skalarbereiche, Single-Property-Policies und die erste Same-Node-Cross-
Property-Relation als instanzfreie Metadaten verfügbar. Der formatneutrale Document Validator
enthielt weiterhin zwei eigene Built-in-Reference-Wahrheiten: eine Tabelle mit elf Source-/Target-
Paaren und einen hardcodierten RadioGroup-Exklusivitätszweig. Der JSON-v1-Parser wiederholte die
Source-/Target-IDs, obwohl das Parsing nur die encoding-spezifische Symbol-zu-Kind-Zuordnung braucht.

Reference-Kompatibilität ist weder eine Property-lokale Regel noch JSON-Syntax, sondern ein stabiler
Relationsvertrag für Document Validation und künftiges Tooling. RadioGroup-Exklusivität ist eine
zusätzliche Graph-Constraint über mehrere Source-Nodes, ihre Edges, ein Target und eine persistente
Source-Property. Die getrennten Ebenen geben jedem Reference Kind genau einen Endpoint-Vertrag und
null bis viele Graph-Regeln, ohne eine allgemeine Rule Engine einzuführen.

### Entscheidung

Die öffentliche backend- und formatneutrale Schicht `component_reference_metadata.hpp` wird
ergänzt. Ihr unveränderlicher Built-in-Katalog enthält exakt die elf Einträge der englischen Tabelle
in der etablierten deterministischen Reihenfolge.

`builtInComponentReferenceMetadata()` liefert den Katalog;
`findBuiltInComponentReferenceMetadata()` sucht fail-closed. Ein fabriziertes Enum liefert
`nullptr`, niemals eine Fallback-Relation. Ein tatsächlicher Endpoint ist kompatibel, wenn sein
exakter Built-in-Typ dem Vertrag entspricht oder über die vorhandene Metadata-Base-Chain davon
ableitet. Dadurch sind konkrete StringList-/StringTable-/StringTreeModel-Targets unter den abstrakten
Model-Verträgen gültig.

Jeder Built-in Reference Kind ist pro Source und Kind eine optionale singuläre Relation: kein oder
genau ein Target dieses Kinds, bei gleichzeitig möglichen unabhängigen anderen Kinds. Dieser heute
universelle Vertrag benötigt keinen spekulativen Cardinality-Enum. Die Katalogreihenfolge ist
deterministische Tooling-Reihenfolge und kein persistenter numerischer Enum-Ordinal-Vertrag.

Graphvalidierung bleibt mit `BuiltInComponentReferenceGraphValidationMetadata` und der Null-bis-
Viele-Abfrage `builtInComponentReferenceGraphValidations()` getrennt. Die einzige heutige Regel ist:

- Reference: `radio_button_group`;
- Kind: `at_most_one_source_boolean_true_per_target`;
- Source-Property: der vollständige Katalogdeskriptor
  `sasd.ui.RadioButton.selected` / `boolean`.

Pro RadioGroup-Target darf höchstens eine eingehende Group-Edge eine Source mit wahrer exakter
Selected-Property besitzen. Beliebig viele dürfen false sein. Standalone ausgewählte RadioButtons
nehmen nicht teil; Ownership oder Nachbarschaft erzeugen keine Membership. Verschiedene Targets
haben unabhängige Zähler. Die Validierung akzeptiert oder verwirft ausschließlich und verändert
keinen Member.

Der gemeinsame Document Validator besitzt nun zwei ausdrückliche Phasen. Die erste löst Reference-
Metadata auf und prüft Endpoints, Self-References, Base-Chain-Kompatibilität sowie den singulären
Source-plus-Kind-Vertrag. Die zweite wertet Graph-Metadata nur auf typgültigen Edges aus. Fehlende
Metadata, eine fehlende exakte Source-Property oder inkonsistente Metadata arbeitet fail-closed mit
`invalid_reference`. Die lokale Source-/Target-Tabelle, der `selected_group`-Zustand und der
hardcodierte `radio_button_group`-Zweig entfallen.

Kanonische JSON-v1-Symbole bleiben encoding-spezifisch. Die Parser-Tabelle enthält nur noch Symbol
und Reference Kind; alle elf Schreibweisen und das Writer-Mapping `referenceSymbol()` bleiben
unverändert. Formatversion, Dokumentform, Error Codes und kanonische Bytes ändern sich nicht.

Runtime-Binding-APIs bleiben autoritativ. Capture verwendet weiterhin typisierte Getter;
Reconstruction behält konkrete typisierte Operationen, konstruktorbasierte RadioButton-Membership
und defensive Casts. Metadata beschreibt Kompatibilität, keine reflektive Invocation oder Restore-
Planung.

### Folgen und Grenzen

Tooling kann gültige Source-/Target-Verträge erkennen und Zielkandidaten ohne Component-Instanzen
filtern. Zusätzlich ist die gruppenübergreifende Selected-Constraint auffindbar. Dieser Slice baut
keinen Designer, Picker, Auto-Fix oder Property-Editor-Abgleich.

Es entsteht keine Graph-DSL, Expression-AST, generische `ValidationRule`, Predicate-Callback-,
`std::function`-, Variant-Rule-Value-, mutable Registry-, Runtime-Reflection- oder Reference-Setter-
Invocation-Schicht. JSON-Symbole gelangen nicht in allgemeine Metadata.

Der Validation-Coverage-Review fand keine weitere heutige Built-in-Semantikregel für die M8-
Property-/Reference-Validation-Metadata-Familien. Dense IDs, Ownership-Gültigkeit, UTF-8, exakte
Schema- und typisierte Property-Vollständigkeit sind allgemeine Dokumentinfrastruktur. Die atomare
TextStyle-Color-Gültigkeit bleibt Teil ihres konkreten Compound-Werts. Der Roadmap-Unterbaum für
kontextabhängige Cross-Property-/Graph-Validation und Validation Metadata kann daher schließen.
Designerfreundliche Property-Attribute, Resources, IDE-Integration und der umschließende Property-
Metadata-/Design-Time-Meilenstein bleiben offen.
