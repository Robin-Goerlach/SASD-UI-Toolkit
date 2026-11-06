# Kanonisches JSON UI Description Format v2

UI Description JSON v2 ergänzt das formatneutrale `BuiltInUiDescriptionDocument` um dauerhafte,
self-contained Resource-Authoring-Semantik. Es ersetzt oder interpretiert
[JSON v1](UI_DESCRIPTION_FORMAT.md) nicht: v1 bleibt der unveränderliche historische
Component-Snapshot-Vertrag; Writer und Reader bleiben ausdrücklich v1. Toolkit-Version `0.2.0` und
UI-Description-Formatversion sind unabhängig. Siehe
[ADR 0197](../adr/0197-m8-resource-aware-ui-description-json-v2.md).

Der öffentliche v2-Adapter arbeitet mit Dokumenten, weil Runtime Components keine Resource
References halten:

```cpp
std::optional<std::string>
encodeBuiltInUiDescriptionJsonV2(const BuiltInUiDescriptionDocument&);

BuiltInUiDescriptionJsonParseResult
parseBuiltInUiDescriptionJsonV2(std::string_view);
```

Beide Operationen validieren den vollständigen formatneutralen Value. Der Encoder kopiert und
normalisiert das öffentlich veränderliche Aggregat und liefert für ein ungültiges Dokument
`nullopt`, niemals partielles JSON. Allocation-Fehler propagieren. Der Parser konstruiert keine
Components und führt kein File-I/O aus.

## Dokument und kanonische Bytes

Top-Level-Keys werden exakt in dieser Reihenfolge ausgegeben:

1. `format`: exakt `"sasd.ui-description"`
2. `formatVersion`: exakte Ganzzahl `2`
3. `resources`: dokumentlokale Resource-Definitionen
4. `root`
5. `nodes`
6. `references`

Formatierung, UTF-8, String-Escaping, Ganzzahldarstellung, Ownership, Node-Identität,
Component-References, Inline-Property-Kinds, Color-Symbole und TextStyle-Feldreihenfolge behalten die
v1-Verträge. Unbekannte oder doppelte Member, fehlende Member, ungültiges UTF-8, Kommentare,
Trailing Commas, ungenaue Zahlen, fehlerhafte Escapes und Restinput schlagen fail-closed fehl.
Object-Member und semantisch irrelevante Array-Reihenfolgen dürfen im Input abweichen; erfolgreiches
Laden normalisiert Nodes und Component References nach dem bestehenden Dokumentvertrag.

## Dokumentlokale TextStyle Resources

v2 definiert exakt einen geschlossenen dokumentlokalen Resource Scope. Seine einzige konkrete
Familie ist heute `TextStyleResource`:

```json
{
  "kind": "text-style",
  "key": "theme.heading",
  "value": {
    "foreground": "bright-cyan",
    "bold": true,
    "dim": false,
    "underline": false,
    "inverse": false
  }
}
```

`text-style` ist ein stabiles externes Symbol, kein C++-Enum-Ordinal oder RTTI-Name. Der Value nutzt
exakt dieselbe Color-/Flag-Semantik wie eine Inline-`text-style`-Property. Resource Keys verwenden
die bestehende nichtleere ASCII-Grammatik aus Buchstaben, Ziffern, `_`, `-`, `.` und `/`; Identität
und Sortierung sind exakt und case-sensitive.

Resource Keys sind dokumentweit eindeutig. Duplicate Keys machen das Dokument ungültig; keine
Definition gewinnt durch Einfügereihenfolge. Kanonische Ausgabe sortiert Definitionen nach
`ResourceKey::value()` mit case-sensitive byteweiser Ordnung, unabhängig von der
`ResourceDictionary`-Einfügereihenfolge.

## Resource-Reference-Properties

Eine Resource-fähige Property bleibt an ihrer normalen base-to-derived Descriptor-Position. Nur
Kind und Value-Darstellung ändern sich:

```json
{
  "declaringType": "sasd.ui.Label",
  "propertyId": "textStyle",
  "kind": "resource-reference",
  "value": {
    "resourceKind": "text-style",
    "key": "theme.heading"
  }
}
```

`resource-reference` ist ein Encoding-Tag. C++ behält die konkreten Typen
`UiDescriptionResourceReferenceProperty` und `ResourceReference`; v2 führt weder generische
ResourceValue-Variant noch Schema Registry ein.

Exakt diese heutigen Descriptoren akzeptieren eine `text-style`-Reference:

- `sasd.ui.Label.textStyle`
- `sasd.ui.Button.textStyle`
- `sasd.ui.TextField.textStyle`
- `sasd.ui.CheckBox.textStyle`
- `sasd.ui.RadioButton.textStyle`
- `sasd.ui.ComboBox.textStyle`

Für jede effektive Property enthält das Dokument exakt einen Authoring Value. Ein Resource-fähiger
Slot ist daher inline XOR resource-reference: beides ist ungültig, keines ebenfalls. Eine Reference
speichert keinen Inline-Fallback. Ihr Kind muss den Resource-Metadaten des Slots entsprechen, und ihr
Key muss auf eine gleichartige Definition in diesem Dokument auflösen. Dangling, externe,
Unknown-Kind- und Wrong-Property-References schlagen fail-closed fehl.

## Scope- und Rekonstruktionsgrenze

v2 persistiert weder Application-/Window-/Container-Scopes noch Parent Scopes, eine geordnete externe
Scope Chain oder M8.26-`scope_index`-Provenienz. Es gibt keine URLs, Imports, Filesystem-Pfade,
Cascade, Merge, Fallbacks oder Live Bindings. Alle referenzierten Values sind vollständig und
self-contained.

M8.27 ist ausschließlich ein Dokumentformat-Roundtrip. Die bestehende Built-in-Component-
Rekonstruktion lehnt jedes Resource-tragende Dokument vor der Konstruktion von Runtime-Objekten ab;
sie verwirft niemals Definitionen oder References. Resource-aware Resolution und Materialisierung
bilden ausdrücklich die M8.28-Grenze.
