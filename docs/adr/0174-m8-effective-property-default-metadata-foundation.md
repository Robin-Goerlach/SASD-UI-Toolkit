# ADR 0174: M8 Effective Property Default Metadata Foundation

## Status

Accepted

## Predecessors

- [0171](0171-m8-component-type-metadata-foundation.md)
- [0172](0172-m8-component-property-metadata-schema-foundation.md)
- [0173](0173-m8-typed-runtime-property-value-access-foundation.md)

## English

### Context

The property schema and typed runtime access contracts describe and access current instance values,
but tooling also needs to query the normal effective value of a property on a newly constructed
built-in type. A single `default_value` member on `ComponentPropertyMetadata` would be incorrect:
an inherited property can have a different effective default on a derived type. `Widget::focusable`
is false for `Widget` and structural widgets such as `Label`, but true for interactive built-ins such
as `Button`, `TextField`, `CheckBox`, `RadioButton`, `ComboBox`, `ListView`, `TableView` and
`TreeView`.

### Decision

Add a separate immutable default-metadata layer through
`sasd/ui/component_property_defaults.hpp`:

- `builtInBooleanPropertyDefault(target_type_id, property)` returns an optional boolean;
- `builtInStringPropertyDefault(target_type_id, property)` returns an optional string view.

`target_type_id` is the exact built-in type whose effective construction policy is being queried.
The query first verifies that the target is a known built-in type and that the supplied descriptor is
the exact semantic descriptor resolved for that target: declaring type ID, property ID and value kind
must all match. This accepts copied valid descriptors and rejects fabricated, case-mismatched or
sibling descriptors such as `Label::text` queried for `Button`.

Defaults are represented by a small explicit immutable policy. General Widget defaults are
`visible == true`, `enabled == true` and `focusable == false`; the interactive built-ins listed
above override only `focusable` to true. Direct defaults are empty text for the five text properties,
`Command::enabled == true` and `CheckBox::checked == false`.

String default views refer only to immutable built-in storage and have process lifetime. No Component
is instantiated to answer a metadata query. The existing schema descriptor remains schema-only and
does not gain a `default_value` member.

### Explicit boundaries

This slice adds no reset/apply-default operation, generic value carrier, `std::variant`, `std::any`,
`void*`, mutable registry, validation, designer/editor metadata, serialization, resource persistence,
runtime type enumeration or third-party default registration. Defaults are metadata about normal
built-in construction policy, not current instance values, user configuration, theme/platform values,
factory parameters or serialized values.

### Consequences

Headless tooling can query effective defaults without allocation, backend startup, ownership changes
or construction side effects. The target type is explicit wherever inheritance can change semantics,
while the contract remains small and value-kind-specific. Applying a queried default, validation and
third-party participation remain separate future decisions.

## Deutsch

### Kontext

Das Property-Schema und der typisierte Runtime-Zugriff beschreiben beziehungsweise lesen aktuelle
Instanzwerte. Tooling muss zusätzlich den normalen effektiven Wert einer Property für einen neu
erzeugten Built-in-Typ abfragen können. Ein einzelnes `default_value`-Feld in
`ComponentPropertyMetadata` wäre falsch: Eine geerbte Property kann abhängig vom Zieltyp einen
anderen effektiven Default besitzen. `Widget::focusable` ist für `Widget` und strukturelle Widgets
wie `Label` false, aber für interaktive Built-ins wie `Button`, `TextField`, `CheckBox`,
`RadioButton`, `ComboBox`, `ListView`, `TableView` und `TreeView` true.

### Entscheidung

Der Core erhält mit `sasd/ui/component_property_defaults.hpp` eine getrennte unveränderliche
Default-Metadaten-Schicht:

- `builtInBooleanPropertyDefault(target_type_id, property)` liefert ein optionales Boolean;
- `builtInStringPropertyDefault(target_type_id, property)` liefert einen optionalen String-View.

`target_type_id` bezeichnet den exakten Built-in-Zieltyp, dessen effektive Konstruktionspolitik
abgefragt wird. Der Zugriff prüft zuerst den bekannten Built-in-Typ und danach den vollständigen
semantischen Deskriptor: deklarierende Typ-ID, Property-ID und Value-Kind müssen mit dem für diesen
Zieltyp aufgelösten Descriptor übereinstimmen. Kopien gültiger Deskriptoren funktionieren; fabrizierte,
case-sensitive falsche oder Geschwister-Deskriptoren wie `Label::text` für `Button` werden abgewiesen.

Die Defaults werden durch eine kleine explizite unveränderliche Policy beschrieben. Allgemeine
Widget-Defaults sind `visible == true`, `enabled == true` und `focusable == false`; die oben
genannten interaktiven Built-ins überschreiben ausschließlich `focusable` auf true. Direkte Defaults
sind leerer Text für die fünf Text-Properties, `Command::enabled == true` und
`CheckBox::checked == false`.

String-Default-Views referenzieren ausschließlich unveränderliche Built-in-Speicherung und besitzen
Prozesslebenszeit. Für eine Metadatenabfrage wird keine Component instanziiert. Der bestehende
Schema-Descriptor bleibt ausschließlich Schema und erhält kein `default_value`-Feld.

### Ausdrückliche Grenzen und Folgen

Dieser Slice ergänzt keine Reset-/Apply-Defaults-Operation, keinen generischen Value-Carrier,
`std::variant`, `std::any`, `void*`, keine veränderliche Registry, Validierung, Designer-/Editor-
Metadaten, Serialisierung, Resource-Persistenz, Runtime-Typenumeration oder Third-Party-Default-
Registrierung. Defaults beschreiben die normale Built-in-Konstruktionspolitik, nicht aktuelle
Instanzwerte, Benutzerkonfiguration, Theme-/Plattformwerte, Factory-Parameter oder serialisierte Werte.

Headless-Tooling kann effektive Defaults ohne Allokation, Backend-Start, Ownership-Änderung oder
Konstruktionsseiteneffekte abfragen. Der Zieltyp bleibt dort explizit, wo Vererbung die Semantik
ändert; Anwenden, Validierung und Third-Party-Beteiligung bleiben getrennte spätere Entscheidungen.
