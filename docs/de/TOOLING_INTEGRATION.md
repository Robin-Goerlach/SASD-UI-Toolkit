# Headless-Tooling-Integration

## Zielgruppe und Grenze

Dieses Dokument beschreibt den installierten öffentlichen Core-Vertrag für externe Authoring-,
Inspektions- und IDE-nahe Tools. Ein solcher Consumer kann die Built-in-Verträge für Typen,
Properties, Validierung, References, Resources und UI Descriptions entdecken und komponieren, ohne
Runtime-Components zu erzeugen oder Terminal-, Rendered- bzw. native Backends zu initialisieren.

Dies ist ein Integrationspunkt, kein IDE-Plugin und kein visueller Designer. Ein vollständiger RAD-
Designer bleibt ein separates Schwesterprojekt. Der Core liefert stabile Maschinenidentitäten und
semantische Constraints; der Consumer verantwortet UI, Lokalisierung, Darstellung und Editing-
Policy.

## Built-in-Typen und Konstruierbarkeit

`builtInComponentTypes()` zählt den unveränderlichen `ComponentTypeMetadata`-Katalog auf. Jeder
Deskriptor enthält stabile `type_id`, direkte `base_type_id` und groben `ComponentTypeKind`. Exakter,
case-sensitiver Lookup erfolgt über `findBuiltInComponentType()`.

`builtInComponentTypeAuthoringMetadata()` klassifiziert einen exakten Deskriptor als
`constructible` oder `abstract_contract`. Dies bedeutet: „Darf dieser Typ als konkreter Built-in-UI-
Description-Node erzeugt werden?“ Es ist keine allgemeine Aussage über C++-Abstraktheit und stellt
keine Factory bereit. Aktuelle abstrakte Verträge sind `sasd.ui.ListModel`, `sasd.ui.TableModel` und
`sasd.ui.TreeModel`; Reference-Endpoint- und Schema-Tooling benötigt ihre Identitäten weiterhin.
Ihre konkreten `String*Model`-Derived-Typen sind konstruierbar.

Deskriptoridentität gilt über den vollständigen Wert aus `type_id`, `base_type_id` und `kind`. Eine
gültige Kopie funktioniert. Unbekannte ID, falsche Groß-/Kleinschreibung, veränderte Base oder
veränderter Kind liefern `std::nullopt`. Tools müssen eine fehlende Klassifikation als nicht
konstruierbar behandeln.

## Base Chains und effektive Properties

Das Property Schema besitzt bewusst keinen zweiten abgeflachten Katalog. Vom gewählten Typ folgt ein
Consumer `ComponentTypeMetadata::base_type_id` bis zur Root, kehrt die Kette um und hängt
`declaredBuiltInComponentProperties(type_id)` von Base nach Derived an. Daraus entsteht die effektive
Property-Reihenfolge von Snapshots, Dokumenten und Reconstruction.

Jede `ComponentPropertyMetadata` behält die ID ihres tatsächlich deklarierenden Typs. Die effektiven
Properties von `sasd.ui.Button` enthalten beispielsweise `visible`, der Deskriptor bleibt jedoch
`sasd.ui.Widget.visible`. Tools dürfen weder `Button.visible` erfinden noch geerbte Metadaten
duplizieren. `findBuiltInComponentProperty()` führt denselben geerbten Lookup für einzelne
Properties aus.

## Komposition des Property-Vertrags

Für jeden exakten Property-Deskriptor komponiert ein Headless-Consumer die bestehenden fokussierten
APIs:

- `ComponentPropertyMetadata::value_kind` wählt die konkrete Authoring-Wertrepräsentation;
- `builtInComponentPropertyAuthoringMetadata()` liefert eine nicht lokalisierte semantische
  Kategorie;
- die acht `builtIn*PropertyDefault()`-Familien liefern target-typbezogene Defaults ohne Instanz;
- `builtInSignedIntegerPropertyRange()` und `builtInUnsignedIntegerPropertyRange()` liefern
  statische Integer-Domänen;
- `builtInStringPropertyNormalization()` liefert Identity- oder Single-Line-UTF-8-Normalisierung;
- `builtInSizeConstraintsPropertyValidation()` liefert die Compound-Minimum-/Maximum-Invariante;
- `builtInOptionalUnsignedIntegerCrossPropertyValidation()` beschreibt die Abhängigkeit von
  `ComboBox.selectedIndex` zum exakten `ComboBox.items`-Deskriptor;
- `builtInComponentReferenceMetadata()` und
  `builtInComponentReferenceGraphValidations()` beschreiben Endpoint-Typen und die RadioGroup-
  Graphregel; und
- `builtInComponentPropertyResourceMetadata()` schaltet exakte Properties für einen konkreten
  `ResourceKind` frei.

Diese Informationen werden bewusst nicht in eine `ToolingPropertyDescriptor`-Mega-Struktur kopiert.
Getrennte Verträge für Schema, Defaults, Validierung, References, Resources und Authoring-Kategorien
halten Ownership und Weiterentwicklung explizit und vermeiden auseinanderlaufende Kataloge.

Alle deskriptorbasierten Abfragen prüfen die vollständige Identität und arbeiten fail-closed. Ein
Tool darf Kategorie, Validierung oder Resource-Fähigkeit nicht aus Namen ableiten. Im aktuellen
Katalog besitzen alle 28 Properties eine Authoring-Kategorie; exakt sechs `textStyle`-Properties
sind resource-fähig.

## Resource-Fähigkeit

`ResourceKind`, `ResourceKey`, `ResourceReference` und
`builtInComponentPropertyResourceMetadata()` bilden die Authoring-Discovery-Grenze. Eine Property
darf nur dann eine Resource Reference erhalten, wenn ihr exakter Deskriptor Metadaten mit demselben
Kind liefert. Ein nicht gesetztes Ergebnis bedeutet bewusst „nicht resource-fähig“ und ist keine
Aufforderung zu einer Namensheuristik.

`ResourceDictionary` und explizite caller-geordnete Scopes bleiben für Runtime-Consumer verfügbar;
Metadaten-Discovery löst jedoch keine Resource auf. `TextStyle` ist die einzige aktuelle Familie.
Weitere Familien bleiben aufgeschoben, bis ein realer Backend- oder Tooling-Consumer einen konkreten
Bedarf etabliert.

## Formatneutrale Dokumente und JSON

`BuiltInUiDescriptionDocument` ist das besitzende formatneutrale Editing-Modell. Ein Tool kann Nodes,
geordnetes Ownership, typisierte Property-Collections, Reference Edges, Resource-Definitionen und
typisierte Resource-Reference-Properties direkt erzeugen und bearbeiten. Da es ein öffentliches
Aggregate ist, erhält Mutation allein nicht automatisch die Gültigkeit; v2-Encoding und beide Parser
validieren und schlagen atomar fehl.

Die versionsspezifischen JSON Entry Points bleiben unverändert:

- `parseBuiltInUiDescriptionJson()` liest striktes JSON v1 in ein validiertes Dokument;
- `encodeBuiltInUiDescriptionJson(const Component&)` erfasst einen Runtime-Graphen und schreibt
  kanonisches v1;
- `parseBuiltInUiDescriptionJsonV2()` liest striktes self-contained resource-aware v2; und
- `encodeBuiltInUiDescriptionJsonV2(document)` validiert und schreibt ein formatneutrales Dokument.

Es gibt weder Formatversion 3 noch persistierte Tooling-Metadaten. Authoring-Tools, die v2-Resource-
Identität erhalten müssen, behalten das Dokument als Source of Truth, statt diese Identität aus
materialisierten Runtime-Components zurückzuerraten.

## Reconstruction-Grenze

`reconstructBuiltInUiDescription(document)` ist eine optionale Runtime-/Preview-Grenze nach Editing
und Validierung. Dort bleiben kontrollierte Construction Policy, Reference-Reihenfolge, Property-
Restore und einmalige Resource-Materialisierung. Konstruierbarkeitsmetadaten rufen Reconstruction
weder auf noch ersetzen sie diese. Discovery hat daher keine Component-Konstruktion, Events,
Resource Resolution, Registrierung oder Backend-Side-Effects.

## Typischer externer Workflow

```text
Installierte öffentliche SASD::UI-Metadaten laden
  -> Built-in-Typen aufzählen
  -> nur konstruierbare Authoring-Typen behalten
  -> Base Chains und deklarierte Properties traversieren
  -> nach semantischer Kategorie gruppieren
  -> Defaults, Validierung, References und Resource-Fähigkeit beziehen
  -> BuiltInUiDescriptionDocument erzeugen oder bearbeiten
  -> JSON v2 encodieren oder JSON v1/v2 parsen
  -> optional eine Runtime-Preview rekonstruieren
```

Das Headless-Beispiel `sasd_ui_tooling_metadata_demo` demonstriert die Discovery-Hälfte. Der
installierte Package Consumer beweist zusätzlich, dass ein separates Projekt nur installierte
öffentliche Header und das Target `SASD::UI` benötigt.

## Bewusst aufgeschobene menschenlesbare Metadaten

Der Core definiert weder `displayName`, lokalisierte Namen, Beschreibungen, Help-Text, Tooltips,
Editor-Kinds oder -Widgets, Property-Grid-Reihenfolge, Browsable-/Read-only-/Advanced-/Favorite-
Flags, Icons, Palette Groups, Dokumentations-URLs, Category Display Text noch visuelle Default-
Größen. Noch kein realer Consumer hat die darin codierten Lokalisierungs-, Darstellungs- und
Editing-Policies definiert. Das Aufschieben ist eine bewusste Scope-Entscheidung und kein offener
M8-Blocker.

Siehe [ADR 0199](../adr/0199-m8-headless-tooling-integration-contract.md) für die Begründung.
