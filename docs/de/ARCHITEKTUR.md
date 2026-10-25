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
`string_collection` oder `optional_unsigned_integer`. Direkt
deklarierte Properties werden unabhängig vom
Typkatalog abgefragt; der geerbte Lookup folgt der vorhandenen direkten `base_type_id`-Kette und
liefert den ursprünglichen deklarierenden Deskriptor zurück. Geerbte Properties werden nicht in
abgeleiteten Typen dupliziert.

Dies bleibt ausschließlich Schema-Metadaten. Die getrennte Schicht
`sasd/ui/component_property_access.hpp` stellt für die katalogisierten Built-ins zwölf explizite
typisierte Read-/Write-Funktionen bereit. Sie validiert
Deskriptoren semantisch, liefert
besitzende Strings, benutzt die bestehenden semantischen Getter/Setter und arbeitet bei unbekannten
Kinds, Identitäten oder inkompatiblen Components fail-closed. Writes melden Zugriffserfolg statt
Wertänderung. Die Zugriffsschicht verwendet intern sichere RTTI-Prüfungen, exponiert aber weder RTTI-
Namen noch einen Runtime-Typenumerationsvertrag. Sie ergänzt kein `std::variant`, `std::any`, `void*`,
keine Getter-/Setter-Pointer, veränderliche Registry, Defaults, Validierung, Serialisierung oder
Drittanbieter-Registrierung und bleibt backendneutral.

### Effektive Property-Defaults

Die getrennte Schicht `sasd/ui/component_property_defaults.hpp` beschreibt effektive Defaults für
einen bekannten Built-in-Zieltyp. Defaults sind zieltypbezogen, weil geerbte Properties verschiedene
Konstruktionspolitiken besitzen können: `Widget.focusable` ist für `Widget` und strukturelle Widgets
false, während interaktive Built-ins den Wert auf true überschreiben. Die Abfrage validiert die
vollständige Property-Identität über den geerbten Schema-Lookup und liefert für unbekannte oder
inkompatible Anfragen `std::nullopt`. String-Defaults sind unveränderliche Views mit Prozesslebenszeit;
zur Abfrage wird keine Component konstruiert. Diese Schicht ergänzt weder Reset-Operationen noch
Validierung, Serialisierung oder Designer-Metadaten.

### Built-in-Component-Property-Snapshots

Die separate Schicht `sasd/ui/component_property_snapshot.hpp` ist der erste kleine Consumer, der
exakte Runtime-Typidentifikation, geerbtes Property-Schema, semantischen Runtime-Zugriff und
effektive Defaults gemeinsam verwendet. `captureBuiltInComponentPropertySnapshot(const Component&)`
besitzt die stabile Typ-ID, Property-Identitäten und erfassten String-Werte in sechs expliziten
Collections für Boolean-, String-, Signed-Integer-, Unsigned-Integer-, String-Collection- und
Optional-Unsigned-Integer-Properties. Die Metadatenkette des exakten Zieltyps wird von
der Basis zum Derived-Typ durchlaufen; innerhalb jeder Collection bleibt die bestehende deklarierte
Reihenfolge erhalten.

Werte werden ausschließlich über die bestehende Runtime-Access-Schicht gelesen und mit dem Default
für den exakten Zieltyp verglichen. Der Snapshot enthält jede aktuell im Schema vorhandene Property,
auch wenn ihr Wert dem Default entspricht; das Weglassen von Defaults ist eine spätere Encoding-
Policy eines Serializers und nicht Snapshot-Policy. Unbekannter Derived-Typ, ungültige
Metadaten-Kette, fehlender Runtime-Read oder fehlender Default-Lookup lehnt den vollständigen
Snapshot ab, statt einen partiellen Zustand zurückzugeben.

Dies ist ein besitzender Snapshot der aktuell katalogisierten Properties, keine vollständige
Component- oder UI-Tree-Beschreibung. Das aktuelle Einundzwanzig-Properties-Schema deckt nicht den gesamten
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
Authoring-Zustand und nicht transient, gehört aber in eine spätere Style-/Resource-Policy.
Model-Inhalte bleiben standardmäßig Application Data; die Model-Referenz einer View ist eine
separate zukünftige Beziehung. Ein Slice-6-Snapshot erfasst nur aktuell katalogisierte Properties
und niemals vollständige Component-Serialisierungsabdeckung. Vollständigkeit liegt erst vor, wenn
jeder relevante öffentliche semantische Zustand durch Property, Struktur-/Reference-/Resource-
Beziehung oder ausdrückliche Exklusion abgedeckt ist.

RadioButton-Metadaten, Zugriff, Defaults und Snapshots decken jetzt `text` (Default leer)
und `selected` (Default false) über bestehende String-/Boolean-Pfade ab (ADR 0179). Writes
delegieren an `setText()`/`setSelected()` und erhalten Gruppenexklusivität und Callbacks.
RadioGroup-Mitgliedschaft bleibt eine persistente Graph-/Referenzbeziehung. Ein skalarer Snapshot
impliziert keine rekonstruierbare Gruppensemantik: Mitgliedschaft/Reference Reconstruction und
deren Reihenfolge bleiben zu definieren.

Der aktuelle Katalog enthält 29 Built-in-Typen und einundzwanzig Property-Deskriptoren. ADR 0177
enthält die Coverage-Matrix; ADR [0178](../adr/0178-m8-numeric-property-value-kind-foundation.md)
ergänzt numerische Layout-Properties. Sechs Spacing-Properties (`HBox.spacing`, `VBox.spacing`,
Grid-/Form-`columnSpacing` und `rowSpacing`) verwenden `signed_integer` mit `std::int64_t` als
Tooling-Wert; `GridLayout.columnCount` verwendet `unsigned_integer` mit `std::uint64_t`.
Beide Konvertierungsrichtungen prüfen den Wertebereich; nicht darstellbare Writes liefern false
ohne Mutation. Semantische Setter-Exceptions für negatives Spacing bzw. null Spalten propagieren
unverändert. Die Property-Schicht dupliziert keine Domain-Validierung. Numerische Defaults sind
unveränderliche Konstanten: Spacing null außer Form-Spaltenabstand eins, Spaltenzahl eins.
Default-Abfragen konstruieren keine Component. Numerische Snapshots besitzen getrennte
`signed_integer_properties` und `unsigned_integer_properties`; jeder fehlende Read/Default verwirft
den gesamten Snapshot. Compound-Authoring-Werte bleiben offen; Referenz-Edges nutzen ihre eigene Schicht. Kein
Serializer und kein generischer `PropertyValue`-Carrier wird eingeführt.

M8 Slice 13 ergänzt konkrete Kinds `string_collection` / `optional_unsigned_integer` für ComboBox
items / selectedIndex ([ADR 0183](../adr/0183-m8-combo-box-collection-and-optional-property-coverage.md)).
Runtime-Reads und Collection-Snapshots besitzen alle Strings. Optionale Reads/Defaults trennen mit
zwei Ebenen äußeren Fehler von gültigem innerem none/Wert. Semantische Setter erhalten Callbacks
und Selection-Clearing; geprüfte Integer-Konvertierung erfolgt vor Setter-Domain-Validierung.
Zukünftige Rekonstruktion muss items vor selectedIndex setzen; getrennte Kind-Vektoren enthalten
keine globale Write-Reihenfolge. Dies ist nur dokumentiert, keine Rekonstruktion implementiert.
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
Encoding und Rekonstruktion bleiben spätere Arbeit; siehe [ADR 0181](../adr/0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md).

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
