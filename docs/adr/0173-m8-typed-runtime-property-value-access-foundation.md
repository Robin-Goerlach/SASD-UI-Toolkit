# ADR 0173: M8 Typed Runtime Property Value Access Foundation

## Status

Accepted

## Predecessor

[0172](0172-m8-component-property-metadata-schema-foundation.md)

## English

### Context

The property schema foundation identifies ten built-in properties, but a known schema descriptor is
not yet useful to a consumer that needs to inspect or update a real Component. The next slice must
add value access without turning schema metadata into a general reflection engine or a generic value
container.

### Decision

The Core exposes a separate `sasd/ui/component_property_access.hpp` runtime layer with four explicit
typed operations:

- `readBuiltInBooleanProperty()` returns `std::optional<bool>`;
- `readBuiltInStringProperty()` returns an owned `std::optional<std::string>`;
- `writeBuiltInBooleanProperty()` accepts `bool`; and
- `writeBuiltInStringProperty()` accepts an owned `std::string`.

The schema descriptor remains immutable schema. Access validates the descriptor semantically against
the built-in catalog, so a copied valid descriptor is usable while unknown, contradictory, or
case-mismatched identities fail closed. Dispatch is built-in-only and uses the existing semantic
getter/setter APIs. It does not write private members, add getter/setter pointers to metadata, or
change Component ownership. Internal safe `dynamic_cast` checks confirm that the concrete instance is
compatible with the descriptor's declaring built-in type. RTTI names, `typeid(T).name()` and any
persistent RTTI identity remain outside the contract.

Reads return no value for unsupported access. Writes return whether the property was recognized and
the compatible semantic setter could be addressed; they do not report whether the value changed.
This preserves setter side effects such as Command observation, TextField sanitization, invalidation,
and CheckBox callbacks. In particular, a same-value CheckBox write is a successful access even though
`setChecked()` reports no change.

### Explicit boundaries

This slice introduces no `std::variant`, `std::any`, `void*`, union value carrier, template-based
reflection replacement, mutable registry, serialization, defaults, validation, design-time
attributes, binding/observation framework, factory, clone operation, or third-party runtime
registration. Generic runtime type enumeration remains deferred; applying a known descriptor to a
component is the only instance question answered here.

### Consequences

Headless tooling can perform small, type-safe reads and writes across the ten existing properties
without backend dependencies or lifetime transfer. The public contract stays explicit and easy to
extend only when a real consumer requires another value kind. String reads clearly own their result,
and the existing Component APIs remain the owners of semantic side effects.

## Deutsch

### Kontext

Die Property-Schema-Grundlage beschreibt zehn Built-in-Properties, ist für einen Consumer aber noch
nicht ausreichend, der einen bekannten Deskriptor auf einer echten Component-Instanz anwenden möchte.
Dieser Slice ergänzt Wertzugriff, ohne Schema-Metadaten in eine allgemeine Reflection-Engine oder
einen generischen Wertcontainer umzuwandeln.

### Entscheidung

Der Core stellt mit `sasd/ui/component_property_access.hpp` eine getrennte Runtime-Schicht mit vier
explizit typisierten Operationen bereit:

- `readBuiltInBooleanProperty()` liefert `std::optional<bool>`;
- `readBuiltInStringProperty()` liefert einen besitzenden `std::optional<std::string>`;
- `writeBuiltInBooleanProperty()` akzeptiert `bool`; und
- `writeBuiltInStringProperty()` akzeptiert einen besitzenden `std::string`.

Der Schema-Deskriptor bleibt unveränderliches Schema. Der Zugriff validiert den Deskriptor semantisch
gegen den Built-in-Katalog; deshalb funktioniert auch eine Kopie eines gültigen Deskriptors, während
unbekannte, widersprüchliche oder case-sensitive falsche Identitäten fail-closed abgewiesen werden.
Der Dispatch ist ausschließlich Built-in und benutzt die bestehenden semantischen Getter/Setter. Er
schreibt keine privaten Member, ergänzt keine Getter-/Setter-Pointer in den Metadaten und verändert
kein Component-Ownership. Interne sichere `dynamic_cast`-Prüfungen stellen die Kompatibilität der
konkreten Instanz mit dem deklarierenden Built-in-Typ fest. RTTI-Namen, `typeid(T).name()` und
persistente RTTI-Identitäten bleiben außerhalb des Vertrags.

Reads liefern bei nicht unterstütztem Zugriff keinen Wert. Writes melden, dass die Property erkannt,
die Component kompatibel und der semantische Setter ansprechbar war; sie melden nicht, ob sich der
Wert geändert hat. Dadurch bleiben Command-Observer, TextField-Sanitization, Invalidierung und
CheckBox-Callbacks erhalten. Ein Same-Value-Write auf `CheckBox` ist insbesondere ein erfolgreicher
Zugriff, auch wenn `setChecked()` selbst keine Änderung meldet.

### Ausdrückliche Grenzen und Folgen

Dieser Slice führt kein `std::variant`, `std::any`, `void*`, keinen Union-Value-Carrier, keine
Template-Ersatz-Reflection, keine veränderliche Registry, Serialisierung, Defaults, Validierung,
Design-Time-Attribute, Binding-/Observation-Framework, Factory, Clone-Operation oder Third-Party-
Runtime-Registrierung ein. Eine generische Runtime-Typenumeration bleibt zurückgestellt; nur die
Anwendung eines bekannten Deskriptors auf eine Component-Instanz wird beantwortet.

Headless-Tooling kann damit kleine typsichere Reads und Writes für die zehn bestehenden Properties
ohne Backend-Abhängigkeit oder Lebenszeitübernahme durchführen. String-Reads besitzen ihr Ergebnis,
und die bestehenden Component-APIs bleiben für alle semantischen Seiteneffekte zuständig.
