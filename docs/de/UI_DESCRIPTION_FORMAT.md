# Kanonisches JSON UI Description Format v1

Diese normative Spezifikation codiert den bestehenden formatneutralen Built-in-Snapshot.
Empfohlene Endung: `.sasd-ui.json`. Die dependency-freie Public API
`encodeBuiltInUiDescriptionJson(const Component&) -> std::optional<std::string>` erfasst einmal den
Referenzgraphen und codiert ausschließlich dessen eigene Werte. Keine File-I/O. Allocation-Fehler
propagieren; Capture-/Encoding-Fehler ergeben `nullopt`, niemals partielles JSON.
Siehe [ADR 0186](../adr/0186-m8-canonical-json-ui-description-encoding-v1.md).

## Dokument und kanonische Bytes

Top-Level-Key-Reihenfolge: `format`, `formatVersion`, `root`, `nodes`, `references`.
`format` ist exakt `sasd.ui-description`, `formatVersion` die positive Ganzzahl `1`.
Keine zusätzlichen Metadaten. Toolkit-Release und Formatversion sind unabhängig. Inkompatible
Formatänderungen benötigen eine neue Version. Der v1-Reader prüft Format/Version ausdrücklich
und lehnt unbekannte Versionen ab. Keine min/max-Version, Schema-URL oder JSON Schema.

Pro Ebene zwei Leerzeichen; nichtleere Objekte/Arrays mehrzeilig; leere Arrays `[]`; pro Array-Wert
eine Zeile; Komma gefolgt von LF; ein Leerzeichen nach Doppelpunkt; keine abschließenden Kommas
oder Leerzeichen; ausschließlich LF und genau ein finales LF. Leere Objekte kommen in v1 nicht vor.
Gültiges Nicht-ASCII-UTF-8 bleibt erhalten. Slash wird nicht escapet. Quote, Backslash, Backspace,
Form Feed, LF, CR und Tab verwenden `\"`, `\\`, `\b`, `\f`, `\n`, `\r`, `\t`;
übrige U+0000..U+001F verwenden kleingeschriebenes `\u00xx`. Kein BOM, Timestamp, Host-Metadatum,
Pointer oder absoluter Pfad. Gleiche Capture-Semantik erzeugt plattformübergreifend gleiche Bytes.

## Nodes, Ownership und Properties

`root` und IDs sind exakte unsigned JSON-Ganzzahlen: dichte Capture-IDs `0..N-1`, Root `0`.
Keine Neuvergabe. `nodes` bewahrt den Ownership-Preorder. Node-Key-Reihenfolge:
`id`, `type`, `owns`, `properties`. `type` ist die stabile Built-in-Type-ID.
`owns` bewahrt geordnete Owned-Child-IDs (ADR 0180); keine owner/parent/visualParent-Duplikation.
Visual Parenting folgt Container-Adoption.

`properties` ist eine geordnete Liste aus den acht C++-Value-Vektoren. Property-Key-Reihenfolge:
`declaringType`, `propertyId`, `kind`, `value`. Declaring-Type-ID plus Property-ID bilden die Identität.
Reihenfolge: base-to-derived, darin direkte `declaredBuiltInComponentProperties()`-Reihenfolge;
ComboBox items steht vor selectedIndex. Spätere Rekonstruktion braucht dennoch eine explizite
Dependency-Policy. Alle erfassten persistenten Werte werden geschrieben, auch Defaults, damit spätere
Default-Änderungen alte Dokumente nicht umdeuten. `is_default` / `isDefault` wird weder geschrieben
noch neu berechnet: Tooling-Metadaten sind keine zweite semantische Wahrheit. Fehlende Values
verwerfen das gesamte Encoding.

Die folgenden Tabellen definieren die vollständigen v1-Symbole und Value-Formen:

| Kind | JSON value |
|---|---|
| `boolean` | `true` / `false` |
| `string` | UTF-8 JSON string |
| `signed-integer` | exact signed decimal integer |
| `unsigned-integer` | exact unsigned decimal integer |
| `string-collection` | ordered array of strings; empty `[]` |
| `optional-unsigned-integer` | unsigned integer or `null` |
| `size-constraints` | object: `minimum`, `preferred`, `maximum`; each `width`, `height` |
| `text-style` | object: `foreground`, `bold`, `dim`, `underline`, `inverse` |

Color symbols (complete v1 vocabulary):
`default`, `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`,
`bright-black`, `bright-red`, `bright-green`, `bright-yellow`, `bright-blue`,
`bright-magenta`, `bright-cyan`, `bright-white`.
`Color::default_color` maps to `default`; other C++ names map underscores to hyphens.

| C++ reference kind | v1 symbol |
|---|---|
| `button_command` | `button.command` |
| `radio_button_group` | `radio-button.group` |
| `list_view_model` | `list-view.model` |
| `table_view_model` | `table-view.model` |
| `tree_view_model` | `tree-view.model` |
| `list_view_selection_model` | `list-view.selection-model` |
| `table_view_selection_model` | `table-view.selection-model` |
| `tree_view_selection_model` | `tree-view.selection-model` |
| `list_selection_model_model` | `list-selection-model.model` |
| `table_selection_model_model` | `table-selection-model.model` |
| `tree_selection_model_model` | `tree-selection-model.model` |

Size-Koordinaten sind signed JSON-Ganzzahlen. `size-constraints` enthält in dieser Reihenfolge
`minimum`, `preferred`, `maximum`, jeweils mit `width`, `height`. `text-style` enthält in dieser
Reihenfolge `foreground`, `bold`, `dim`, `underline`, `inverse`; die vier Flags sind JSON-Booleans.
Beispielwerte: minimum 0/0, preferred 40/20, maximum 100/50 beziehungsweise bright-cyan mit
bold=true, dim=false, underline=true, inverse=false. Die vollständigen Compound-Beispiele stehen
in der [englischen Fassung](../en/UI_DESCRIPTION_FORMAT.md).

## Referenzen und Validierung

`references` bewahrt Source-ID-/Kind-Reihenfolge des Capture (ADRs 0181/0182).
Edge-Key-Reihenfolge: `source`, `kind`, `target`; Endpunkte sind exakte Node-IDs, Kind ist symbolisch.
`{"source": 2, "kind": "button.command", "target": 1}` verbindet etwa Button und owned Command;
das tatsächliche Objekt wird mehrzeilig ausgegeben. Keine Namen oder Runtime-Adressen als Identität.
Unbekannte Reference-/Property-Kinds und Color-Werte besitzen keine numerische/Fallback-Darstellung:
das gesamte Encoding schlägt fehl. Runtime-Color-Setter bleiben unverändert.

Alle persistenten Strings einschließlich Collection-Elementen müssen gültiges UTF-8 sein.
Ungültige, überlange, abgebrochene, Surrogat- oder Out-of-range-Sequenzen verwerfen alles; keine
Ersetzung, Byte-Auslassung, Normalisierung oder Latin-1-Annahme. Controls bleiben durch Escapes exakt.
Integer werden localeunabhängig als exakter Dezimaltext geschrieben: keine Quotes, Exponenten,
Gruppierung oder Float-Konvertierung. uint64 wird nicht auf JavaScript-Safe-Integer beschränkt;
der Reader liest uint64 exakt. Optionales Fehlen ist `null`, kein Sentinel.

## Ausschlüsse und Grenzen

Model-/SelectionModel-Nodes und Referenzen sind enthalten; Application-Payloads von StringListModel,
StringTableModel und StringTreeModel (Items, Rows, Headers, Tree-Nodes) bleiben ausgeschlossen.
Ebenso aktuelle Selection, Tree Expansion, Fokus, Hover, Pressed/Capture, TextField-Cursor/-Selection,
ComboBox-Preview/-Open-State, Viewports, Bounds, DesiredSize, Measurement-Validität, Dirty-State,
Backend-Handles, Callbacks, Observer und ausführbare Funktionen. Keine erfundenen Callback-Namen
oder Resource-Identitäten.

JSON ist ausschließlich externes Encoding, kein internes Component-/Property-/Ownership-/Reference-
Modell. Der Reader erzeugt ein validiertes Dokument; die getrennte Built-in-Reconstruction-API
konsumiert dieses Dokument. Kein Resource-System, keine Model-Payload-Persistence und keine Datei-API.
Alternative SASD-/Binary-/DFM-artige Encodings bleiben möglich. JSON Schema kann bei Bedarf für
externes Tooling folgen.

## Reader v1 und validiertes Dokument

`parseBuiltInUiDescriptionJson(std::string_view)` liefert ein besitzendes, formatneutrales
`BuiltInUiDescriptionDocument` (Public Header `sasd/ui/ui_description_document.hpp`). Capture-
Snapshots und geladene Dokumente sind verschiedene Typen: keine erfundene oder neu berechnete
`is_default`-Information. Alle acht konkreten Property-Collections besitzen Werte und Strings
unabhängig vom Input. Format-/Versionsmarker gehören nur zum Encoding.

Kanonische Ausgabe ist Writer-Vertrag. Der Reader akzeptiert erlaubten JSON-Whitespace, beliebige
Object-Member-Reihenfolge und beliebige Property-/Node-/Reference-Array-Reihenfolge. Nodes werden
nach ID ohne Umnummerierung normalisiert, References nach Source-ID und fester Kind-Reihenfolge,
konkrete Properties nach deklarierendem Typ/Property-ID. Ownership-Children und String-Collections
behalten ihre Reihenfolge. Jedes Object lehnt unbekannte und doppelte Member ab. Alle Member und
alle effektiven Properties bleiben erforderlich; Defaults werden nicht ergänzt. Nur exakte Built-in-
Typ-IDs, deklarierende Identitäten, Kind-/Color-/Reference-Symbole und Version 1 gelten.

Der gesamte Input muss gültiges UTF-8 ohne BOM sein. Alle JSON-Escapes einschließlich Surrogat-
Paaren werden ohne Ersatz oder Normalisierung zu UTF-8 decodiert; einzelne Surrogate und rohe
Controls sind ungültig. Kommentare, Trailing Commas, Single Quotes, unvollständige Tokens und
Trailing Garbage scheitern. Ganzzahlen sind exakt int64/uint64/Coordinate, ohne Float-Konvertierung,
führendes Plus/Nullen, Fraction oder Exponent. Keine JavaScript-Safe-Integer-Grenze.

Validierung verlangt nichtleere dichte IDs 0..N-1, Root 0, Container-Owner, einen erreichbaren,
zyklusfreien Baum mit genau einem Owner pro Non-root und exakter DFS-Preorder entlang owns.
References benötigen vorhandene verschiedene Endpunkte, höchstens eine Edge pro Source/Kind und
metadatenkompatible Typen einschließlich Model-Base-Chains. View/model und selection/model bleiben
unabhängig. SizeConstraints müssen eine gültige Range haben; negative Werte und Preferred außerhalb
min/max bleiben gültig. ComboBox selectedIndex ist null oder in items, GridLayout columnCount positiv,
TextField-Text unverändert durch sanitizeSingleLine; je referenzierter RadioGroup höchstens ein
selected Member.

Erfolg: document vorhanden und error.code == none. Fehler: kein Dokument und stabiler
`UiDescriptionJsonParseErrorCode`: invalid_utf8, syntax_error, duplicate_member, unknown_member,
missing_member, wrong_value_type, unsupported_format, unsupported_format_version,
unknown_component_type, unknown_property, property_kind_mismatch, invalid_property_value,
invalid_ownership oder invalid_reference. `byte_offset` ist ein 0-basierter Input-Byteoffset an/nahe
der Fehlererkennung; EOF darf input.size() sein, semantische Fehler dürfen den enthaltenden Node/Edge
markieren. Allocation-Fehler propagieren. Keine Components, Factory, Property-Restore, Ownership-
Adoption, Referenzauflösung, Callback-Suppression oder File-I/O. Siehe
[ADR 0187](../adr/0187-m8-json-v1-parser-validated-document-model.md).

## Built-in-Rekonstruktion

`reconstructBuiltInUiDescription(const BuiltInUiDescriptionDocument&)` kann erfolgreich geparste
v1-Dokumente für alle unterstützten konkreten Built-in-Typen rekonstruieren. Das veränderliche
Dokument wird vor Runtime-Konstruktion erneut validiert; der besitzende
`std::unique_ptr<Component>`-Root wird erst nach vollständigem Property-, Reference- und geordnetem
Ownership-Restore übergeben.

Exakte Einträge `sasd.ui.ListModel`, `sasd.ui.TableModel` und `sasd.ui.TreeModel` bleiben bekannte
Schema-Typen, sind aber abstrakt und nicht direkt konstruierbar. Ihre konkreten String-Model-Varianten
werden leer konstruiert: Items, Rows, Headers und Tree Nodes bleiben ausgeschlossene Application-
Payloads. Die Reconstruction-API parst kein JSON, führt kein File-I/O aus, lädt keine Resources,
restauriert keinen transienten Zustand und überträgt Description-Node-Identität nicht in Runtime-
Components. Siehe [ADR 0188](../adr/0188-m8-built-in-ui-description-reconstruction.md).

## Vollständiges normatives Beispiel

Ein frisch konstruierter Label ergibt exakt (mit finalem LF):

```json
{
  "format": "sasd.ui-description",
  "formatVersion": 1,
  "root": 0,
  "nodes": [
    {
      "id": 0,
      "type": "sasd.ui.Label",
      "owns": [],
      "properties": [
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "visible",
          "kind": "boolean",
          "value": true
        },
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "enabled",
          "kind": "boolean",
          "value": true
        },
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "focusable",
          "kind": "boolean",
          "value": false
        },
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "sizeConstraints",
          "kind": "size-constraints",
          "value": {
            "minimum": {
              "width": 0,
              "height": 0
            },
            "preferred": {
              "width": 0,
              "height": 0
            },
            "maximum": {
              "width": 2147483647,
              "height": 2147483647
            }
          }
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "text",
          "kind": "string",
          "value": ""
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "textStyle",
          "kind": "text-style",
          "value": {
            "foreground": "default",
            "bold": false,
            "dim": false,
            "underline": false,
            "inverse": false
          }
        }
      ]
    }
  ],
  "references": []
}
```
