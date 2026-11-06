# Canonical JSON UI Description Format v2

UI Description JSON v2 adds durable, self-contained resource authoring to the format-neutral
`BuiltInUiDescriptionDocument`. It does not replace or reinterpret
[JSON v1](UI_DESCRIPTION_FORMAT.md): v1 remains the immutable historical Component-snapshot
contract, and its writer and reader remain explicitly v1. Toolkit version `0.2.0` and UI Description
format versions are independent. See [ADR 0197](../adr/0197-m8-resource-aware-ui-description-json-v2.md).

The public v2 adapter operates on documents because runtime Components do not store resource
references:

```cpp
std::optional<std::string>
encodeBuiltInUiDescriptionJsonV2(const BuiltInUiDescriptionDocument&);

BuiltInUiDescriptionJsonParseResult
parseBuiltInUiDescriptionJsonV2(std::string_view);
```

Both operations validate the complete format-neutral value. Encoding copies and normalizes the
publicly mutable aggregate and returns `nullopt` for an invalid document, never partial JSON.
Allocation failures propagate. Parsing constructs no Components and performs no file I/O.

## Document and canonical bytes

Top-level keys are emitted in this exact order:

1. `format`: exactly `"sasd.ui-description"`
2. `formatVersion`: exact integer `2`
3. `resources`: the document-local resource definitions
4. `root`
5. `nodes`
6. `references`

Formatting, UTF-8, string escaping, integer representation, ownership, node identity, component
references, inline property kinds, Color symbols and TextStyle field order retain the v1 contracts.
Unknown or duplicate members, missing members, invalid UTF-8, comments, trailing commas, imprecise
numbers, malformed escapes and trailing input fail closed. Object member order and semantically
irrelevant array order may differ on input; successful loading normalizes nodes and component
references according to the existing document contract.

## Document-local TextStyle resources

v2 defines exactly one closed document-local resource scope. Today its sole concrete family is
`TextStyleResource`:

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

`text-style` is a stable external symbol, not a C++ enum ordinal or RTTI name. The value uses exactly
the same Color and flag semantics as an inline `text-style` property. Resource keys use the existing
non-empty ASCII `ResourceKey` grammar: letters, digits, `_`, `-`, `.` and `/`; identity and sorting
are exact and case-sensitive.

Resource keys are globally unique within one document. Duplicate keys are invalid; no definition
wins by insertion order. Canonical output sorts definitions by `ResourceKey::value()` using ordinary
case-sensitive bytewise comparison, independent of `ResourceDictionary` insertion order.

## Resource-reference properties

A resource-capable property occupies its normal base-to-derived property-descriptor position. The
property entry changes only its kind and value representation:

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

`resource-reference` is an encoding tag. C++ retains the concrete
`UiDescriptionResourceReferenceProperty` and `ResourceReference` types; v2 does not introduce a
generic ResourceValue variant or schema registry.

Exactly these current descriptors accept a `text-style` reference:

- `sasd.ui.Label.textStyle`
- `sasd.ui.Button.textStyle`
- `sasd.ui.TextField.textStyle`
- `sasd.ui.CheckBox.textStyle`
- `sasd.ui.RadioButton.textStyle`
- `sasd.ui.ComboBox.textStyle`

For each effective property, the document contains exactly one authoring value. A resource-capable
slot is therefore inline XOR resource-reference: both is invalid, and neither is invalid. A reference
stores no inline fallback. Its kind must equal the slot's resource metadata and its key must resolve
to a same-kind definition in this document. Dangling, external, unknown-kind and wrong-property
references fail closed.

## Scope and reconstruction boundary

v2 does not persist application/window/container scopes, parent scopes, an ordered external scope
chain or M8.26 `scope_index` provenance. It has no URLs, imports, filesystem paths, cascade, merge,
fallback or live binding. All referenced values are complete and self-contained.

M8.27 is a document-format roundtrip only. Existing built-in Component reconstruction rejects any
resource-bearing document before constructing runtime objects; it never drops definitions or
references. Resource-aware resolution and materialization are the explicit M8.28 boundary.
