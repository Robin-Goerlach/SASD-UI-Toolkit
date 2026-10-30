# Architektur

## Überblick

Das SASD UI Toolkit soll eine **semantische UI-Schicht** von ihrer konkreten Darstellung trennen. Anwendungscode arbeitet mit `Window`, `Button`, `Label`, Layouts, Models und Events. Ein Backend entscheidet, wie diese Konzepte auf einer Zielumgebung umgesetzt werden.

```text
                    SASD-Anwendung
                         │
                  Öffentliche UI-API
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   Komponenten         Layouts          Models
        │                │                │
        └────────────────┼────────────────┘
                         │
                    Events/Commands
                         │
                      Toolkit
                         │
                 Backend-/Peer-API
                         │
       ┌─────────────────┼──────────────────┐
       │                 │                  │
 Native Desktop     Rendered Desktop      Terminal
 Win32/GTK/AppKit       z. B. SDL3       ANSI/VT/Console
```

## Architektonische Leitidee

Die öffentliche UI-API darf nicht von einer konkreten Plattform abhängig sein. Gleichzeitig soll das Projekt nicht versuchen, jede Plattform künstlich identisch erscheinen zu lassen.

Daraus folgen drei Regeln:

1. **Semantik wird vereinheitlicht.** Ein Button ist ein Button, unabhängig vom Backend.
2. **Darstellung darf unterschiedlich sein.** Ein nativer Windows-Button und ein Terminal-Button müssen nicht pixel- oder zeichengenau gleich aussehen.
3. **Capabilities werden explizit gemacht.** Plattformunterschiede werden abgefragt statt versteckt.

## Komponentenmodell

Inspiriert von VCL und AWT wird zwischen allgemeinen Komponenten und sichtbaren Widgets unterschieden.

```text
Component
├── Command
├── Action
├── Timer
├── DataSource
└── Widget
    ├── Label
    ├── Button
    ├── CheckBox
    ├── TextField
    └── Container
        ├── Panel
        ├── ScrollView
        └── Window
            └── Dialog
```

`Component` ist nicht zwangsläufig sichtbar. Dadurch können später auch Actions, Commands, Timer oder Datenquellen in dasselbe Ownership- und Lebenszyklusmodell integriert werden.

### Komponenten-Typmetadaten

Der Core stellt nun über `sasd/ui/component_metadata.hpp` eine bewusst kleine Grundlage für
Typmetadaten bereit. `ComponentTypeMetadata` enthält eine explizite stabile Toolkit-Typ-ID, die
direkte Metadaten-Basis-Typ-ID und die grobe Klassifikation `component`/`widget`/`container`. Der
unveränderliche Built-in-Katalog beschreibt die aktuell im Core implementierten öffentlichen
Component-Untertypen und kann ohne Instanzerzeugung oder Backend-Initialisierung abgefragt werden.

Diese IDs sind keine C++-RTTI-Namen und nicht lokalisiert. Der Katalog bietet weder Runtime-
Instanzreflexion noch Factories, Serialisierung oder eine veränderbare Plugin-Registry; diese
Entscheidungen bleiben spätere M8-Slices. Komponentenmetadaten beschreiben ausschließlich Typen und
beeinflussen weder Component-Ownership noch visuelles Parenting, Events oder Layout.

### Exakte Runtime-Identifikation von Built-in-Typen

Die separate Schicht `sasd/ui/component_runtime_metadata.hpp` beantwortet, welchem exakten
katalogisierten Built-in-Typ eine lebende `Component`-Instanz entspricht. Sie vergleicht das
dynamische `typeid(component)` mit jedem bekannten Built-in-C++-Typ und liefert anschließend den
zugehörigen Pointer aus dem bestehenden unveränderlichen Katalog zurück. Sie verwendet niemals
`type_info::name()`, RTTI-Hashes oder compilerspezifische Namen als Identität. Unbekannte
Drittanbieter-Derived-Typen fallen mit `nullptr` fail-closed aus; sie werden nicht auf den nächsten
bekannten Basistyp angenähert, weil ein künftiger Serialisierungs-Consumer sonst abgeleiteten Zustand
still verlieren könnte.

Dies ist eine interne Grundlage für den nächsten serialisierungsorientierten Slice, nicht die
Serialisierung selbst: Es gibt noch kein Beschreibungsformat, keine Factory, keinen Deserializer,
keine Child-Aufzählung, veränderbare Registry oder Drittanbieter-Registrierung. `Component` bleibt
frei von einer virtuellen `typeId()`-Reflection-Methode.

### Komponenten-Property-Metadaten

Die separate Schicht `sasd/ui/component_property_metadata.hpp` beschreibt ein kleines,
unveränderliches Schema für ausgewählte Built-in-Komponententypen. `ComponentPropertyMetadata`
enthält eine stabile lokale Property-ID, die ID des direkt deklarierenden Typs und den einfachen
semantischen Werttyp `boolean`, `string`, `signed_integer`, `unsigned_integer`,
`string_collection`, `optional_unsigned_integer`, `size_constraints` oder `text_style`. Direkt
deklarierte Properties werden unabhängig vom
Typkatalog abgefragt; der geerbte Lookup folgt der vorhandenen direkten `base_type_id`-Kette und
liefert den ursprünglichen deklarierenden Deskriptor zurück. Geerbte Properties werden nicht in
abgeleiteten Typen dupliziert.

Dies bleibt ausschließlich Schema-Metadaten. Die getrennte Schicht
`sasd/ui/component_property_access.hpp` stellt für die katalogisierten Built-ins sechzehn explizite
typisierte Read-/Write-Funktionen bereit. Sie validiert
Deskriptoren semantisch, liefert
besitzende Strings, benutzt die bestehenden semantischen Getter/Setter und arbeitet bei unbekannten
Kinds, Identitäten oder inkompatiblen Components fail-closed. Writes melden Zugriffserfolg statt
Wertänderung. Die Zugriffsschicht verwendet intern sichere RTTI-Prüfungen, exponiert aber weder RTTI-
Namen noch einen Runtime-Typenumerationsvertrag. Sie ergänzt kein `std::variant`, `std::any`, `void*`,
keine Getter-/Setter-Pointer, veränderliche Registry, Defaults, Validierung, Serialisierung oder
Drittanbieter-Registrierung und bleibt backendneutral.

M8 Slice 14 ergänzt `Widget.sizeConstraints` als konkrete typisierte Compound-Property ([ADR 0184](../adr/0184-m8-widget-size-constraints-compound-property-coverage.md)).
Access, Defaults und besitzende Snapshots nutzen direkt den bestehenden backendneutralen Public Value
`SizeConstraints`, ohne generische Compound-Abstraktion. Der atomare semantische Setter besitzt Range-
Validierung und Measurement-Invalidierung. Same-value-Zugriff gelingt, Domain-Exceptions propagieren;
Preferred außerhalb min/max und negative Hints erhalten keine zusätzliche Einschränkung. Bounds,
Desired Size, Measure-Validität und Parent-MeasureConstraints bleiben außerhalb der UI Description.

M8 Slice 15 erfasst inline `TextStyle` für genau Label, Button, TextField, CheckBox, RadioButton und
ComboBox ([ADR 0185](../adr/0185-m8-inline-text-style-authoring-persistence-coverage.md)). Der konkrete `text_style`-
Pfad nutzt den Public Value direkt mit TextStyle{}-Defaults, ohne Resource-Node oder Widget-weite Property. Writes ausschließlich
über Setter erhalten Presentation-Invalidierung ohne Measurement-Invalidierung und Same-value-No-op.
Keine Vererbung/Cascade; Backend-Fokus-/Disabled-/Selection-Overlays bleiben ausgeschlossen. Fonts,
RGB/Alpha, Themes und Shared Resources bleiben Zukunft. Kanonisches JSON v1 verwendet symbolische Color-Namen
statt Enum-Ordinals (ADR 0186). Der Review aller 29 Built-ins bestätigte die semantische Readiness.
Encoding, validiertes Parsing und Built-in-Rekonstruktion sind implementiert; Resources bleiben offen.

### Kanonisches JSON-Description-Encoding

`sasd/ui/ui_description_json.hpp` liegt oberhalb des eigenen Reference-Graph-Snapshots. Das Capture-
Modell bleibt formatneutral; JSON ist ausschließlich externes Encoding. Der dependency-freie
Component-Root-Writer erfasst einmal und erzeugt deterministisches kanonisches v1 mit allen
persistenten Werten explizit. Capture-, UTF-8- und Symbolfehler liefern nullopt. Writer-Bytes bleiben unverändert;
File-I/O bleibt Aufgabe der Anwendung. Siehe [Formatspezifikation](UI_DESCRIPTION_FORMAT.md) und
[ADR 0186](../adr/0186-m8-canonical-json-ui-description-encoding-v1.md).

### Validiertes Laden von UI Descriptions

Die Pipelines verwenden absichtlich verschiedene Werttypen:

```text
Runtime Graph -> Capture Snapshot -> Canonical JSON v1
Kanonisches/gültiges JSON v1 -> Parser -> Validated Document -> Rekonstruktion -> Runtime Graph
```

Das besitzende formatneutrale Dokument nutzt acht konkrete Property-Collections und die vorhandene
Reference-Kind-Enum. Capture-Snapshots enthalten is_default als Tooling-Metadatum; geladene Dokumente
lassen es weg, weil es nicht persistiert wurde. Keine Default-Ergänzung oder Neuberechnung. Der
dependency-freie Reader akzeptiert kosmetische JSON-/Member-/Array-Reihenfolgen, lehnt aber unbekannte/
doppelte Member, unvollständige Schemas, ungültiges UTF-8/Escapes und unexakte Integer ab.
Metadaten-/Wertvalidierung prüft Ownership-Preorder, Referenztypkompatibilität und ohne Konstruktion
prüfbare Rekonstruierbarkeit. Keine Component-Konstruktion, Setter/Callbacks, Resources oder Datei-API.
Siehe [ADR 0187](../adr/0187-m8-json-v1-parser-validated-document-model.md).

### Rekonstruktion von Built-in-Descriptions

`sasd/ui/ui_description_reconstruction.hpp` rekonstruiert einen vollständig besitzenden Built-in-
Graphen aus einem bereits decodierten `BuiltInUiDescriptionDocument`; weder JSON-Parsing noch File-I/O
gehören dazu. Weil das Dokument ein veränderliches öffentliches Aggregate ist, teilen Parser und
Rekonstruktion einen internen semantischen Validator. Vor jeder Component-Konstruktion wird eine
private Kopie validiert und normalisiert.

Die Konstruktion erfolgt in Phasen. Zuerst entstehen alle Non-RadioButtons; gruppierte RadioButtons
folgen über `RadioButton(RadioGroup&, std::string)`. Es gibt kein Serialization-only-`setGroup()`.
Command-Properties werden vor dem Snapshot durch `Button::bindCommand()` restauriert. SelectionModel-
zu-Model-, View-zu-Model-, View-zu-SelectionModel- und Button-zu-Command-References folgen einer
expliziten semantischen Reihenfolge unabhängig von Document-Arrays. Ein finaler Base-to-Derived-
Metadaten-Pass restauriert alle persistenten Properties, einschließlich des unabhängigen Button-
Snapshots und ComboBox `items` vor `selectedIndex`.

Alle Instanzen bleiben bis zum Erfolg der semantischen Wiederherstellung flach durch
`std::unique_ptr` gestaged. Erst danach bildet `Container::adopt()` jede gespeicherte Ownership-
Reihenfolge nach; Ownership, Parenting, References, Gruppenmitgliedschaft und abgeleitete Auswahl
werden vor Root-Transfer geprüft. Fehler geben keinen partiellen Graphen zurück. Description-IDs
existieren nur im Staging-Lookup und werden nicht in Runtime-Components gespeichert; zwischen
Ausgangs- und Zielgraph gibt es kein Identity Carry-over.

Der interne geschlossene Construction Adapter unterstützt alle 26 konkreten Katalogtypen. Exakte
abstrakte `ListModel`-, `TableModel`- und `TreeModel`-Nodes melden einen nicht konstruierbaren Typ,
statt substituiert zu werden. Konkrete String-Models starten leer, weil Payload Application Data ist.
Fresh Controls besitzen keine Application Callbacks; normale Setter benötigen daher keinen
Suppression-Modus. Fokus, Selection, Viewport, Expansion, Bounds und anderer transienter Zustand
bleiben ausgeschlossen. Siehe [ADR 0188](../adr/0188-m8-built-in-ui-description-reconstruction.md).

### Effektive Property-Defaults

Die getrennte Schicht `sasd/ui/component_property_defaults.hpp` beschreibt effektive Defaults für
einen bekannten Built-in-Zieltyp über acht typisierte Abfragefamilien. Defaults sind zieltypbezogen, weil geerbte Properties verschiedene
Konstruktionspolitiken besitzen können: `Widget.focusable` ist für `Widget` und strukturelle Widgets
false, während interaktive Built-ins den Wert auf true überschreiben. Die Abfrage validiert die
vollständige Property-Identität über den geerbten Schema-Lookup und liefert für unbekannte oder
inkompatible Anfragen `std::nullopt`. String-Defaults sind unveränderliche Views mit Prozesslebenszeit;
zur Abfrage wird keine Component konstruiert. Diese Schicht ergänzt weder Reset-Operationen noch
Validierung, Serialisierung oder Designer-Metadaten.

### Statische Property-Validation- und Normalization-Metadaten

Die separate Schicht `sasd/ui/component_property_validation_metadata.hpp` beschreibt instanzfreie,
inklusive Integer-Domänen für exakte Built-in-Property-Deskriptoren. Schema, effektive Defaults,
statische Validation Metadata und typisierter Runtime-Zugriff bleiben getrennt: Der Metadatenconsumer
konstruiert keine Component, während konkrete Setter die autoritative Runtime-Enforcement-Grenze
bleiben. Die vollständige Declaring-Type-/Property-/Kind-Identität wird als Wert statt über eine
Katalogadresse verglichen.

Alle sechs Signed-Layout-Spacing-Properties liefern `[0, Coordinate::max]`; der semantische Fixed-
Width-Property-Typ und nicht der weitere `int64_t`-Carrier bestimmt das nutzbare Maximum.
`GridLayout.columnCount` liefert `[1, UINT64_MAX]` statt `[1, size_t::max]`, damit portable
semantische Validierung nicht vom aktuellen Host abhängt. Typisierter Property-Zugriff und
Rekonstruktion lehnen weiterhin einen semantisch gültigen Wert ab, den der Host nicht darstellen
kann. Der gemeinsame formatneutrale Document Validator verwendet diese Bereiche, sodass negatives
oder außerhalb von Coordinate liegendes Spacing vor der Rekonstruktion abgelehnt wird. Runtime-Setter
behalten ihre unabhängigen Prüfungen. Dies ist nur die statische Integer-Grundlage und keine
generische Rule Engine; siehe
[ADR 0189](../adr/0189-m8-static-integer-property-validation-metadata-foundation.md).

Der gleiche Header stellt nun außerdem zwei bewusst konkrete Single-Property-Policy-Familien bereit.
`builtInStringPropertyNormalization()` liefert explizites `identity` für Command-, Label-, Button-,
CheckBox- und RadioButton-Text sowie `single_line_utf8` für `TextField.text`.
`builtInSizeConstraintsPropertyValidation()` identifiziert für den exakten Deskriptor
`Widget.sizeConstraints` die Minimum-not-greater-than-Maximum-Invariante. Preferred-Werte außerhalb
dieser Range und negative Dimensionen bleiben gültig. Eine vorhandene Identity-Policy unterscheidet
sich von einem ungültigen Deskriptor; jede zukünftige String-Property muss daher bewusst entscheiden.

Diese Metadata-Abfragen sind instanzfreie Beschreibungen und keine Setter-Implementierungen.
TextField normalisiert weiterhin durch `utf8::sanitizeSingleLine()`, Widget lehnt eine ungültige
Range weiterhin in seinem Runtime-Setter ab. Der formatneutrale Document Validator erzwingt zuerst
allgemeine UTF-8-Gültigkeit, verwendet danach exakte Deskriptor-Normalization/-Validation-Metadata
und arbeitet bei fehlender Policy fail-closed. Sein früherer hardcodierter TextField-Zweig und die
implizite Regel für jeden `size_constraints`-Kind entfallen.

Die konkrete Optional-Unsigned-Cross-Property-Metadata beschreibt `ComboBox.selectedIndex` als
Index in den exakten Same-Node-String-Collection-Deskriptor `ComboBox.items`. Null-Selection bleibt
auch für eine leere Collection gültig. Der formatneutrale Validator verwendet Deskriptoridentität
statt Nodetyp, Vectorposition oder Input-Reihenfolge und vergleicht den persistenten `uint64_t`
sicher mit der Collection-Größe. Fehlende Metadata oder Dependencies arbeiten fail-closed. Runtime-
Setter bleiben autoritativ; die Rekonstruktion folgt weiterhin unabhängig der Deklarationsreihenfolge.

Die separate Schicht `sasd/ui/component_reference_metadata.hpp` stellt für jeden der elf Built-in
Reference Kinds genau einen unveränderlichen Source-/Target-Typvertrag bereit. Tatsächliche Endpoints
dürfen exakt entsprechen oder über die Built-in-Metadata-Base-Chain ableiten; dadurch bleibt die
String-Model-Polymorphie erhalten. Jeder Kind ist pro Source und Kind eine optionale singuläre
Relation. JSON-Symbole bleiben encoding-spezifisch und gehören nicht in diesen formatneutralen
Tooling-Katalog.

Zusätzliche Graph-Constraints verwenden eine getrennte Null-bis-Viele-Metadata-Abfrage. Die einzige
heutige Regel besagt, dass pro `radio_button_group`-Target höchstens eine eingehende Source den
exakten Boolean-Deskriptor `RadioButton.selected` auf true setzen darf. Standalone RadioButtons und
verschiedene Group-Targets bleiben unabhängig. Der gemeinsame Document Validator verwendet beide
Reference-Ebenen ohne Component-Konstruktion; Runtime-Setter und Binding-APIs bleiben autoritativ.
Capture und Reconstruction behalten ihre konkreten typisierten Operationen.

Property Schema, effektive Defaults, statische numerische Domains, Single-Property-Normalisierung,
atomare Compound-Validierung, Same-Node-Cross-Property-Validierung, Reference-Endpoint-Verträge,
Graph-Constraints, typisierter Runtime-Zugriff, Document Validation und Runtime-Enforcement bleiben
getrennte Schichten. Es entsteht weder eine generische Validation Engine noch Graph-DSL, Registry
oder Reflection-Invocation. Die heutige Built-in-Validation-Metadata-Abdeckung ist vollständig;
siehe [ADR 0190](../adr/0190-m8-single-property-validation-normalization-metadata-foundation.md),
[ADR 0191](../adr/0191-m8-combo-box-cross-property-validation-metadata-foundation.md) und
[ADR 0192](../adr/0192-m8-built-in-reference-and-graph-validation-metadata-foundation.md).

### Property-Authoring-Kategorie-Metadaten

Die getrennte Schicht `sasd/ui/component_property_authoring_metadata.hpp` ordnet jedem exakten
Built-in-Property-Deskriptor eine stabile, nicht lokalisierte Authoring-Kategorie zu: Appearance,
Behavior, Content, Layout oder State. Das Property Schema bleibt die einzige öffentliche
Enumerationsquelle und beschreibt technische Identität/Value Kind. Defaults beschreiben
Konstruktionspolitik, Validation Metadata gültige Werte und Relationen, typisierter Runtime-Zugriff
liest/schreibt tatsächliche Werte und Authoring Metadata gruppiert Deskriptoren ausschließlich für
Tooling.

Die Abfrage validiert den vollständigen Declaring-Type-/Property-/Value-Kind-Deskriptor als Wert und
arbeitet bei fehlerhafter oder unbekannter Eingabe fail-closed. Geerbter Lookup behält den
deklarierenden Deskriptor; `Button.visible` verwendet deshalb die Behavior-Kategorie von
`Widget.visible` ohne Derived-Duplikat. Kategorien sind explizit statt aus Namen abgeleitet und
benötigen keine Component-Konstruktion. Sie sind weder lokalisierte Labels noch Property-Grid-
Reihenfolge oder Persistenzdaten; Display-/Help-Text, Editor Kind und Browsable-/Read-only-/Advanced-
Flags werden nicht definiert. Siehe
[ADR 0193](../adr/0193-m8-property-authoring-category-metadata-foundation.md).

### Built-in-Component-Property-Snapshots

Die separate Schicht `sasd/ui/component_property_snapshot.hpp` ist der erste kleine Consumer, der
exakte Runtime-Typidentifikation, geerbtes Property-Schema, semantischen Runtime-Zugriff und
effektive Defaults gemeinsam verwendet. `captureBuiltInComponentPropertySnapshot(const Component&)`
besitzt die stabile Typ-ID, Property-Identitäten und erfassten String-Werte in acht expliziten
Collections für Boolean-, String-, Signed-Integer-, Unsigned-Integer-, String-Collection-,
Optional-Unsigned-Integer-, Size-Constraints- und Text-Style-Properties. Die Metadatenkette des exakten Zieltyps wird von
der Basis zum Derived-Typ durchlaufen; innerhalb jeder Collection bleibt die bestehende deklarierte
Reihenfolge erhalten.

Werte werden ausschließlich über die bestehende Runtime-Access-Schicht gelesen und mit dem Default
für den exakten Zieltyp verglichen. Der Snapshot enthält jede aktuell im Schema vorhandene Property,
auch wenn ihr Wert dem Default entspricht; kanonisches JSON v1 schreibt ebenfalls alle Werte
explizit, ohne is_default-Metadaten zu persistieren. Unbekannter Derived-Typ, ungültige
Metadaten-Kette, fehlender Runtime-Read oder fehlender Default-Lookup lehnt den vollständigen
Snapshot ab, statt einen partiellen Zustand zurückzugeben.

Dies ist ein besitzender Snapshot der aktuell katalogisierten Properties, keine vollständige
Component- oder UI-Tree-Beschreibung. Das aktuelle Achtundzwanzig-Properties-Schema deckt nicht den gesamten
semantischen Zustand von Controls, Models, Layouts, Bindings oder Ownership/visuellen Kindern ab.
Keine Child-Rekursion, kein Ownership-Serializer, kein PropertyValue-Container, kein `std::variant`,
`std::any`, Persistenzformat, Serializer oder Deserializer wird hier eingeführt; die Schema-Abdeckung
bleibt in späteren Slices unabhängig erweiterbar.

### Abdeckung des serialisierbaren UI-Description-Zustands

ADR [0177](../adr/0177-m8-serializable-ui-description-state-coverage-policy.md) definiert die
Grenze einer späteren UI Description. Eine UI Description beschreibt dauerhafte deklarative
Konstruktion und Initialkonfiguration, nicht Session-Snapshot, Application-Data-Snapshot,
Heap-Snapshot oder Presentation-/Backend-Dump. Persistente skalare Properties sind nur ein Teil:
Ownership, Child-Reihenfolge, visuelles Parenting, Command-Bindings, RadioGroup-Mitgliedschaft und
View-/Model-Beziehungen sind strukturelle bzw. Referenzbeziehungen und dürfen nicht in skalare
Property-Metadaten gepresst werden.

Die Policy schließt Fokus, Hover, Pressed-/Capture-Gesten, Textauswahl/Caret, ComboBox-Popup und
Preview, Viewports, aktuelle Selection, Tree-Expansion, Layout-Caches, arrangierte Bounds,
Presentation-Snapshots und Backend-Handles ausdrücklich aus. `TextStyle` ist dauerhafter visueller
Authoring-Zustand und durch text_style inline erfasst (ADR 0185), nicht transient.
Zukünftiges Resource-basiertes Styling bleibt separat.
Model-Inhalte bleiben standardmäßig Application Data; die Model-Referenz einer View ist eine
separate zukünftige Beziehung. Ein Slice-6-Snapshot erfasst nur aktuell katalogisierte Properties
und niemals vollständige Component-Serialisierungsabdeckung. Vollständigkeit liegt erst vor, wenn
jeder relevante öffentliche semantische Zustand durch Property, Struktur-/Reference-/Resource-
Beziehung oder ausdrückliche Exklusion abgedeckt ist.

RadioButton-Metadaten, Zugriff, Defaults und Snapshots decken jetzt `text` (Default leer)
und `selected` (Default false) über bestehende String-/Boolean-Pfade ab (ADR 0179). Writes
delegieren an `setText()`/`setSelected()` und erhalten Gruppenexklusivität und Callbacks.
RadioGroup-Mitgliedschaft wird vor `selected` durch den bestehenden Konstruktor rekonstruiert;
`RadioGroup::selectedButton()` bleibt normal abgeleiteter Runtime-Zustand.

Der aktuelle Katalog enthält 29 Built-in-Typen und achtundzwanzig Property-Deskriptoren. ADR 0177
enthält die Coverage-Matrix; ADR [0178](../adr/0178-m8-numeric-property-value-kind-foundation.md)
ergänzt numerische Layout-Properties. Sechs Spacing-Properties (`HBox.spacing`, `VBox.spacing`,
Grid-/Form-`columnSpacing` und `rowSpacing`) verwenden `signed_integer` mit `std::int64_t` als
Tooling-Wert; `GridLayout.columnCount` verwendet `unsigned_integer` mit `std::uint64_t`.
Beide Konvertierungsrichtungen prüfen den Wertebereich; nicht darstellbare Writes liefern false
ohne Mutation. Semantische Setter-Exceptions für negatives Spacing bzw. null Spalten propagieren
unverändert. Die Runtime-Access-Schicht ersetzt keine Setter-Validierung; die getrennte statische
Metadatenschicht stellt dieselben nutzbaren Domänen für Tooling und Document Validation bereit.
Numerische Defaults sind
unveränderliche Konstanten: Spacing null außer Form-Spaltenabstand eins, Spaltenzahl eins.
Default-Abfragen konstruieren keine Component. Numerische Snapshots besitzen getrennte
`signed_integer_properties` und `unsigned_integer_properties`; jeder fehlende Read/Default verwirft
den gesamten Snapshot. Heutige Compound-Authoring-Werte sind durch ADRs 0184/0185 erfasst; Referenz-Edges nutzen ihre eigene Schicht. Kein
Serializer und kein generischer `PropertyValue`-Carrier wird eingeführt.

M8 Slice 13 ergänzt konkrete Kinds `string_collection` / `optional_unsigned_integer` für ComboBox
items / selectedIndex ([ADR 0183](../adr/0183-m8-combo-box-collection-and-optional-property-coverage.md)).
Runtime-Reads und Collection-Snapshots besitzen alle Strings. Optionale Reads/Defaults trennen mit
zwei Ebenen äußeren Fehler von gültigem innerem none/Wert. Semantische Setter erhalten Callbacks
und Selection-Clearing; geprüfte Integer-Konvertierung erfolgt vor Setter-Domain-Validierung.
Rekonstruktion folgt der Metadaten-Deklarationsreihenfolge und restauriert daher items vor selectedIndex.
selectedText bleibt abgeleitet; previewIndex/previewText/dropDownOpen/pressed und Callbacks sind ausgeschlossen.

### Beschreibungslokale Identität und Ownership-Snapshots

Die separate Schicht `sasd/ui/component_ownership_snapshot.hpp` komponiert bestehende Property-
Snapshots zu einem besitzenden Baum. Jeder Node besitzt eine beschreibungslokale `std::uint64_t`-ID,
seinen Property-Snapshot einschließlich exakter Typ-Identität und geordnete Owned-Component-IDs.
Root ist 0; dichte IDs folgen der Pre-order über `componentAt()`, einschließlich non-visueller
Commands, Gruppen und konkreter Models. Explizite Edge-Listen bewahren Ownership-Reihenfolge
unabhängig von der globalen Node-Vector-Reihenfolge. IDs sind keine permanente Runtime-Component-,
Application- oder Resource-Identität; Stabilität zwischen Captures wird nicht garantiert.
Capture verlangt einen unowned Root. Unbekannte exakte Typen, ungültige Properties, doppelte Nodes
und inkonsistente Owner-/Widget-Parent-Beziehungen verwerfen den gesamten Snapshot. Aktuelle
Container-Adoption leitet Widget-Parenting aus Ownership ab; geprüft wird ohne doppelte visuelle
Parent-Edges; siehe [ADR 0180](../adr/0180-m8-description-node-identity-ordered-ownership-snapshot.md).

### Component-Reference-Graph-Snapshots

`sasd/ui/component_reference_snapshot.hpp` komponiert den Ownership-Snapshot mit besitzenden Edges
aus beschreibungslokalen Source-/Target-Node-IDs und elf expliziten Kinds: Button -> Command,
RadioButton -> RadioGroup, List/Table/Tree View -> Model, View -> SelectionModel und
SelectionModel -> Model. Jedes lebende Ziel muss im selben Ownership-Snapshot liegen; externe Ziele verwerfen den gesamten Capture. Null-/ungebundene/abgelaufene
Getter erzeugen keine Edge. Pointer dienen nur temporärem Lookup und werden niemals persistiert.
Edge-Reihenfolge folgt Source-Nodes und darin fester Kind-Reihenfolge. RadioGroup-Selection bleibt
abgeleitet; Model-Payloads bleiben Application Data. SelectionModels sind getrennte non-visuelle
Components ohne generische SelectionModel-Hierarchie.
Normale Component-Ownership/-Identität erhält Sharing durch Node-IDs. View- und SelectionModel-Model-
Bindings bleiben unabhängig, ohne neue Gleichheitsinvariante. Aktuelle Zeilen-/Zellen-/Node-Selection
und Revision bleiben transient und ohne Scalar-Properties; siehe [ADR 0182](../adr/0182-m8-selection-model-component-lifecycle-and-reference-coverage.md).
Kanonisches JSON-v1-Encoding und Built-in-Rekonstruktion sind implementiert; siehe
[ADR 0181](../adr/0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md) und
[ADR 0188](../adr/0188-m8-built-in-ui-description-reconstruction.md).

### Ownership

Für den Core gilt als Startpunkt:

- Ein `Container` besitzt seine Kindkomponenten.
- Ownership soll vorzugsweise durch RAII und eindeutige Besitzverhältnisse dargestellt werden.
- Nicht-besitzende Referenzen dürfen Lebenszeiten nicht verlängern.
- Callback-Verbindungen müssen sicher getrennt werden können, wenn Sender oder Empfänger zerstört werden.

Die konkrete C++-API wird erst mit dem ersten Core-Prototyp festgezurrt; rohe owning pointer sind jedoch kein Ziel.

## Toolkit und Backend

Das `Toolkit` ist die Brücke zwischen semantischem UI-Baum und Plattformimplementierung. Es ist für globale Dienste zuständig, beispielsweise:

- Erzeugen und Zerstören von Top-Level-Fenstern
- Event-Pump / Event-Loop
- Ermitteln von Backend-Capabilities
- Clipboard- und Cursor-Dienste
- Theme-/Metric-Zugriff
- Plattformintegration

Ein Backend kann Widgets auf zwei Arten umsetzen:

### Native Peers

Das Backend ordnet einem SASD-Widget ein natives Betriebssystem-Control zu.

Beispiele:

- `Button` → Win32 Button
- `Button` → GTK Button
- `Button` → AppKit Button

### Rendered Peers

Das Backend zeichnet das Widget selbst auf eine Zeichenfläche.

Das ist insbesondere wichtig für:

- Terminal-Backends
- SDL-basierte Backends
- Widgets ohne natives Gegenstück
- konsistente Spezialkomponenten

Ein Backend darf beide Strategien kombinieren.

## BackendCapabilities

Plattformunterschiede sollen nicht durch Annahmen oder implizites Verhalten kaschiert werden. Ein Backend meldet Fähigkeiten, zum Beispiel:

```cpp
struct BackendCapabilities {
    bool mouse;
    bool clipboard;
    bool dragAndDrop;
    bool nativeMenus;
    bool multipleWindows;
    bool trueColor;
    bool accessibility;
};
```

Diese Struktur ist nur ein Konzeptentwurf. Die spätere API kann granularer werden.

## Eventmodell

Das Eventsystem soll backendunabhängig sein. Rohereignisse werden vom Backend in semantische Events übersetzt.

Geplante Eventfamilien:

- Tastatur
- Textinput
- Pointer/Maus
- Fokus
- Aktivierung/Click
- Fenster/Lifecycle
- Resize
- Command/Action

Wichtig ist die Trennung von **Key Events** und **Text Input**. Ein Zeichen ist nicht dasselbe wie eine physische Taste; dies ist für internationale Tastaturen, IMEs und Accessibility relevant.

Für UI-Events ist zunächst ein einzelner UI-Thread mit Event-Queue vorgesehen. Hintergrundarbeit soll über explizite Übergaben zurück in den UI-Thread integriert werden, statt Widgets beliebig thread-safe zu machen.

### Routing und Fokus im aktuellen M1-Core

Der implementierte M1-Core trennt Event-Pump, Zielauswahl, Routing und Fokus bewusst voneinander:

```text
Backend
   │
   ▼
Application
   │
   ├── QuitEvent → Application-Lifecycle
   │
   └── semantisches Eingabe-Event
             │
             ▼
      EventTargetResolver
             │
             ▼
        Ziel-Widget
             │
             ▼
      EventDispatcher
             │
             └── Target → Parent → ... → Root
```

`EventDispatcher` führt synchrones Bubbling über den **visuellen Parent-Pfad** aus. Ein Widget kann ein Event mit `EventResult::handled` konsumieren; bei `ignored` wird dasselbe Event dem visuellen Parent angeboten. Ownership-Beziehungen zu nicht-visuellen Komponenten gehören bewusst nicht zu diesem Routing-Pfad.

Der `FocusManager` ist eine davon getrennte Zielauswahl-Komponente für logischen Tastaturfokus. Er besitzt Widgets nicht, sondern hält genau eine nicht-besitzende Referenz auf das aktuell fokussierte Widget. Widget und FocusManager lösen diese Referenz beim jeweiligen Zerstören gegenseitig, damit kein dangling Pointer bestehen bleibt.

Widgets sind standardmäßig **nicht fokussierbar**. Für den M1-Core besteht die lokale Fokus-Eignung aus `focusable && visible && enabled`. Vererbte Zustände von späteren `Window`-/Container-Fokus-Scope-Regeln werden erst festgelegt, wenn reale Widgets und Backends diese Semantik validieren können.

Ein vom `FocusManager` erzeugtes `FocusEvent` ist eine direkte Zustandsbenachrichtigung an genau das betroffene Widget. Der Fokuszustand ist bereits geändert, wenn der Handler aufgerufen wird. Diese Benachrichtigung ist nicht veto-fähig und wird nicht als normales Eingabe-Bubbling interpretiert. Gewöhnliche Tastatur- und Texteingabe verwendet weiterhin den `EventDispatcher`.

Für M2 ergänzt `FocusTraversal` die zustandsbehaftete Fokusverwaltung um deterministische Tab-/Shift+Tab-Zielauswahl. Traversal läuft in visueller Preorder/Adoptionsreihenfolge, wrappt an beiden Enden und überspringt hidden/disabled Teilbäume vollständig. Es ist bewusst als Unhandled-Event-Policy ausgelegt: Ein Control darf Tab zuerst selbst konsumieren; nur ein unhandled Tab wird anschließend als Fokusnavigation interpretiert. `Application` und `EventDispatcher` bleiben dadurch weiterhin frei von Fokusordnungswissen.

## Layout

Layouts dürfen nicht direkt in Pixeln denken. Ein Terminal arbeitet mit Zellen, Desktop-UIs mit logischen Geräteeinheiten und Schriftmetriken.

### Measure/Arrange-Vertrag im M1-Core

Der implementierte Core verwendet einen zweiphasigen, backendneutralen Layout-Lebenszyklus:

```text
Parent
  │
  ├── measure(MeasureConstraints)
  │        │
  │        ▼
  │   onMeasure(...)
  │        │
  │        ▼
  │   desiredSize
  │
  └── arrange(final Rect)
           │
           ▼
      bounds werden gesetzt
           │
           ▼
      onArrange(...)
```

`SizeConstraints` gehören zum Widget selbst und beschreiben Minimum, Preferred und Maximum. `MeasureConstraints` kommen vom Parent und beschreiben ausschließlich den tatsächlich angebotenen Minimum-/Maximum-Bereich. Die intrinsische Größe aus `onMeasure()` wird zuerst durch die eigenen Widget-Hinweise und danach durch die Parent-Constraints begrenzt; damit kann ein Kind keinen Platz erzwingen, den der Parent nicht besitzt.

`measure()` cached `desiredSize()` für identische Parent-Constraints. Zusätzlich kann `measure(context, constraints)` einen backendneutralen `MeasurementContext` verwenden. Dadurch kann beispielsweise `Label` Textmetriken anfragen, ohne Terminal-, Font- oder native Typen zu kennen. Der Messcache unterscheidet contextlose und contextbasierte Messungen und identifiziert einen Context über dynamischen Typ plus `revision()`; das Widget hält keinen Context-Pointer. Der erste `TerminalMeasurementContext` liefert Spalten-/Zeilenmetriken aus derselben `TextMetrics`-Policy wie das Terminal-Rendering.

Größenrelevante Zustandsänderungen rufen `invalidateMeasure()` auf. Diese Invalidierung propagiert über den visuellen Parent-Pfad, sodass beispielsweise eine spätere Textänderung in einem `Label` auch `VBox` und `Window` als neu zu vermessen markieren kann. Auch das Hinzufügen oder Entfernen visueller Kinder invalidiert den Container.

`arrange()` weist das endgültige logische Rechteck zu. Die Bounds werden vor `onArrange()` gespeichert, damit ein Container beim Anordnen seiner Kinder seine eigene finale Geometrie verwenden kann. Nullgrößen sind erlaubt; negative Ausdehnungen werden abgelehnt. Direktes `setBounds()` läuft bewusst über denselben Arrange-Pfad.

Noch offen bleiben konkrete Regeln für `VBox`/`HBox`, Margin/Padding/Spacing und den Layout-Effekt unsichtbarer Widgets. Die Backend-Grenze für Textmessung ist inzwischen über `MeasurementContext` festgelegt; reichere Font-/Style-Metriken werden erst ergänzt, wenn weitere Backends sie praktisch benötigen. Diese Punkte sollen erst mit den ersten realen M2-Controls festgelegt werden.

Deshalb soll der Layoutprozess auf folgenden Konzepten beruhen:

- Minimum Size
- Preferred Size
- Maximum Size
- verfügbare Constraints
- Padding/Margins/Spacing
- backendabhängige Text- und Widget-Messung

Erste implementierte Layouts:

- `HBox`: sichtbare Kinder horizontal, Querachse wird gestreckt;
- `VBox`: sichtbare Kinder vertikal, Querachse wird gestreckt.

Beide reichen den aktiven `MeasurementContext` an ihre Kinder weiter, unterstützen nichtnegatives Spacing nur zwischen sichtbaren Kindern und clippen bei zu kleinem finalen Hauptachsenplatz deterministisch in Adoptionsreihenfolge. Margin, Padding, Alignment und Flex-Gewichte sind noch nicht Teil des ersten Vertrags.

Weitere geplante Layouts:

- `GridLayout`
- `FormLayout`
- `StackLayout`
- optional `AbsoluteLayout` für Sonderfälle

Absolute Positionierung soll möglich, aber nicht das Standardmodell sein.

## Model/View für datenreiche Widgets

Listen, Tabellen und Bäume sollen langfristig Daten nicht zwangsläufig kopieren. Dafür wird eine von Qt und anderen UI-Frameworks inspirierte Model/View-Trennung vorgesehen.

```text
Anwendungsdaten
     │
   Model
     │
    View
     │
 Delegate/Renderer/Editor
```

Ein `TableView` soll beispielsweise große Datenmengen virtualisiert darstellen können, ohne Millionen UI-Objekte erzeugen zu müssen.

## Commands und Actions

Ein Command beschreibt eine auslösbare Aktion unabhängig vom auslösenden Widget. Ein Command kann später gleichzeitig mit Menüeintrag, Toolbar-Button, Tastenkürzel oder Command-Palette verbunden sein.

Geplante Eigenschaften:

- Text/Label
- enabled/disabled
- checked/unchecked, wo sinnvoll
- Shortcut
- auszuführender Callback

Das vermeidet duplizierte Logik zwischen Menü, Button und Tastaturbedienung.

## Rendering

Der Core schreibt keinen bestimmten Renderer vor. Rendered Backends erhalten eine abstrakte Zeichen-/Surface-Schicht oder kapseln ihren Renderer vollständig hinter dem Backend.

### Visuelle Invalidierung im M1-Core

Layout-Neuberechnung und reine Darstellungsänderung sind bewusst getrennt:

```text
Größe/Inhalt beeinflusst Layout        Nur Darstellung geändert
             │                                  │
             ▼                                  ▼
   invalidateMeasure()                  invalidateVisual()
             │                                  │
             ▼                                  ▼
    measure() erneut nötig              Visual update pending
                                                │
                                                ▼
                                  Terminal / Renderer / Native Peer
                                                │
                                                ▼
                                  acknowledgeVisualUpdate()
```

Ein neues Widget gilt zunächst als visuell nicht synchronisiert. `invalidateVisual()` setzt den Pending-Zustand und propagiert über den visuellen Parent-Pfad. Fokus, Enabled-Zustand und eine geänderte finale Geometrie können deshalb eine Darstellung aktualisieren, ohne die gecachte Wunschgröße ungültig zu machen. Visibility sowie das Hinzufügen/Entfernen visueller Kinder betreffen dagegen sowohl Layout als auch Darstellung.

`acknowledgeVisualUpdate()` bestätigt nur das tatsächlich verarbeitete Widget und löscht nicht automatisch Dirty-Zustände von Nachfahren.

Geometrie- und Strukturänderungen verwenden zusätzlich einen stärkeren `Presentation-Subtree-Refresh`. `arrange()` fordert ihn bei geänderten Bounds an; das Entfernen eines visuellen Kindes fordert ihn am alten Container an. Der Request propagiert bis zum Presentation-Root. Nach erfolgreicher Synchronisation dieses Roots bietet der `PresentationCoordinator` auch sonst cleane Descendants erneut an. Damit können alte Positionen gelöscht und zuvor verdeckte Geschwister korrekt wieder aufgebaut werden, ohne bereits Dirty-Rectangle-/Occlusion-Logik in den Core einzubauen.

### PresentationCoordinator und PresentationSink

Der M1-Core besitzt inzwischen die backendneutrale Brücke, die diesen Pending-Zustand konsumiert:

```text
Widget-Baum
    │
    │ dirty/clean traversal
    ▼
PresentationCoordinator
    │
    │ nur pending Widgets
    ▼
PresentationSink
    │
    ├── synchronized → Widget bestätigen
    └── deferred     → Widget bleibt pending
```

Der Coordinator traversiert den vollständigen visuellen Teilbaum deterministisch in Preorder (Parent vor Kindern). Cleane Widgets werden nicht an die Senke geschickt, aber weiter traversiert, damit ein pending Descendant unter einem bereits bestätigten Parent nicht verloren geht. Unsichtbare pending Widgets werden ebenfalls angeboten, weil gerade das Entfernen ihrer bisherigen Darstellung ein notwendiges Update sein kann.

Die Senke erhält ausschließlich `const Widget&` und darf den visuellen Baum während eines Passes nicht strukturell verändern. Terminal-, Rendered- und Native-Backends können denselben Vertrag verwenden, obwohl ihre konkrete Synchronisation völlig unterschiedlich ausfällt.

Dirty Rectangles, Frame Scheduling, Clipping, Display Lists und konkrete Render-/Terminal-Diff-Algorithmen bleiben bewusst außerhalb dieses M1-Vertrags.

Ein optionales SDL3-Backend ist als früher grafischer Proof-of-Concept attraktiv, weil damit eine Render-/Input-Basis für mehrere Desktopplattformen verfügbar ist, ohne SDL zur öffentlichen SASD-API zu machen.

## Text und Unicode

Die öffentliche API soll Unicode von Anfang an berücksichtigen. Als Startpunkt wird UTF-8 an API-Grenzen bevorzugt. Plattformkonvertierungen, beispielsweise zu UTF-16 unter Windows, gehören in das jeweilige Backend.

Terminal-Rendering muss Zeichenbreiten, Combining Characters und später Grapheme Cluster berücksichtigen. Für `v0.1.0` darf der Umfang begrenzt sein, die Architektur darf aber nicht dauerhaft von „ein Byte = ein Zeichen = eine Zelle“ ausgehen.

Mit `TextField` besitzt der Core nun zusätzlich eine gemeinsame `sasd::ui::utf8`-Utility für deterministisches Scalar-Decoding. TextField hält gültiges einzeiliges UTF-8; malformed Bytes werden als U+FFFD normalisiert, Line-/Control-Sequenzen für das Single-Line-Control entfernt. Die aktuelle Cursorposition ist bewusst ein Unicode-Scalar-Index. Combining-/ZWJ-Sequenzen bleiben erhalten, sind aber noch nicht Grapheme-aware navigierbar. Terminal-Viewport und Hardware-Caret bleiben Presentation-State und werden nicht im semantischen TextField gespeichert.

## Accessibility

Accessibility wird als Architekturthema behandelt, nicht als spätes Zusatzfeature. Native Backends können Plattformdienste nutzen; gerenderte Backends benötigen langfristig eine semantische Accessibility-Bridge. Der Core soll daher semantische Rollen, Namen, Zustände und Aktionen nicht ausschließlich aus visuellen Details ableiten.

## Öffentliche API versus Backend-API

Diese Trennung ist strikt:

```text
include/sasd/ui/...              öffentliche API
src/core/...                     plattformneutraler Kern
src/backends/terminal/...        Terminal
src/backends/sdl/...             gerenderter Desktop
src/backends/win32/...           Windows native
src/backends/gtk/...             Linux native
src/backends/appkit/...          macOS native
```

Die genaue Verzeichnisstruktur wird beim ersten Code-Meilenstein angelegt, sobald die minimalen Schnittstellen feststehen.

## Stabilitätsregel

Während `0.x` darf sich die API ändern. Änderungen sollen aber begründet, dokumentiert und nach Möglichkeit migrationsfreundlich erfolgen. Ab einer späteren stabilen Version wird eine explizite API-/ABI-Policy notwendig.
