# Canonical JSON UI Description Format v1

This normative format encodes the existing format-neutral built-in description snapshot.
Recommended extension: `.sasd-ui.json`. The dependency-free public function
`encodeBuiltInUiDescriptionJson(const Component&) -> std::optional<std::string>` captures the
reference graph once and encodes its owned values. It performs no file I/O. Allocation failures
propagate; capture/encoding failures return `nullopt`, never partial JSON.
See [ADR 0186](../adr/0186-m8-canonical-json-ui-description-encoding-v1.md).

## Document and canonical bytes

Top-level keys, in order: `format`, `formatVersion`, `root`, `nodes`, `references`.
`format` is exactly `sasd.ui-description`; `formatVersion` is the positive integer `1`.
No additional metadata fields are emitted. Toolkit release and format versions are independent.
Incompatible format changes require a new format version. The v1 reader explicitly checks
format/version and rejects unknown versions. No min/max version, schema URL or JSON Schema is provided.

Canonical formatting uses two spaces per nesting level, multiline nonempty objects and arrays,
empty arrays `[]`, one array element per line, comma followed by LF, one space after colons,
no trailing commas/whitespace, LF only, and exactly one final LF. Empty objects cannot occur in v1.
Strings retain valid non-ASCII UTF-8; slash is not escaped. Quote, backslash, backspace, form feed,
LF, CR and tab use `\"`, `\\`, `\b`, `\f`, `\n`, `\r`, `\t`; other U+0000..U+001F
use lowercase `\u00xx`. No BOM, timestamps, host metadata, pointers or absolute paths.
The same captured semantics produce byte-identical output across platforms.

## Nodes, ownership and properties

`root` and node IDs are exact JSON unsigned integers, captured dense `0..N-1` with root `0`.
The writer never renumbers. `nodes` retain capture's ownership preorder.
Each node has keys `id`, `type`, `owns`, `properties`, in that order. `type` is the stable built-in
type ID. `owns` contains ordered owned child IDs (ADR 0180); no owner/parent/visualParent duplicate
is stored. Visual parenting follows Container adoption.

`properties` is one ordered list, flattening the eight C++ value vectors. Each property has keys
`declaringType`, `propertyId`, `kind`, `value`, in that order. Identity is the declaring type ID plus
property ID. Order is base-to-derived declaration order, then direct
`declaredBuiltInComponentProperties()` order. Thus ComboBox items precedes selectedIndex.
Future reconstruction must still establish an explicit dependency policy.
All captured persistent values are written, including defaults: old documents retain explicit
semantics if future toolkit defaults change. `is_default` / `isDefault` is never written or
recomputed: it is tooling metadata, not a second semantic truth. Missing captured values fail closed.

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

Size coordinates are signed JSON integers; TextStyle flags are JSON booleans. For example:

```json
{
  "minimum": {
    "width": 0,
    "height": 0
  },
  "preferred": {
    "width": 40,
    "height": 20
  },
  "maximum": {
    "width": 100,
    "height": 50
  }
}
```

```json
{
  "foreground": "bright-cyan",
  "bold": true,
  "dim": false,
  "underline": true,
  "inverse": false
}
```

## References and validation

`references` retains capture's source-ID then reference-kind order (ADRs 0181/0182).
Each edge has keys `source`, `kind`, `target`, in that order: exact node IDs and a symbolic kind.
For example `{"source": 2, "kind": "button.command", "target": 1}` connects a Button to an owned
Command; names and runtime addresses never identify targets. The actual output object is multiline.
Unknown reference/property kinds and unknown Color values have no numeric/fallback representation:
encoding fails closed. Runtime Color setter behavior remains unchanged.

Every persisted string, including collection elements, must be valid UTF-8. Malformed, overlong,
truncated, surrogate and out-of-range sequences reject the entire encoding; no replacement,
byte dropping, normalization or Latin-1 fallback occurs. JSON escaping preserves controls exactly.
Integers use locale-independent exact decimal text, without quotes, exponent, grouping or floating
conversion. uint64 values are not restricted to JavaScript safe integers; the reader parses
uint64 exactly. Optional absence is `null`, never a sentinel.

## Exclusions and boundaries

Model and SelectionModel nodes/references are supported, but StringListModel/StringTableModel/
StringTreeModel application items, rows, headers and tree payloads are excluded. Current selection
and tree expansion are excluded. Also excluded: focus, hover, pressed/capture state, TextField cursor
and selection, ComboBox preview/open state, viewports, bounds, desired size, measurement validity,
dirty state, backend handles, callbacks, observers and executable functions. No symbolic callback
names or resource identities are invented.

JSON is only an external encoding, never the internal Component/property/ownership/reference model.
The reader produces a validated document. No live deserializer, factory, resolver, reconstruction,
resource system, model-payload persistence or file API exists. Alternative SASD/binary/DFM-like encodings remain possible.
A JSON Schema may follow when external tooling needs it.

## Reader v1 and validated document

`parseBuiltInUiDescriptionJson(std::string_view)` returns an owning, format-neutral
`BuiltInUiDescriptionDocument` (public header `sasd/ui/ui_description_document.hpp`). Capture
snapshots and loaded documents are distinct: no `is_default` information is invented or recomputed.
All eight typed property collections own their values and strings independently of the input.
Format/version markers belong only to this encoding, not the semantic document.

Canonical output is a writer contract. The reader accepts permitted JSON whitespace, arbitrary
object-member order and arbitrary property/node/reference array order. Nodes normalize by ascending
ID without renumbering, references by source ID then fixed reference-kind order, and typed properties
by declaring-type/property identity. Ownership child order and string-collection order are preserved.
Every object rejects unknown and duplicate members; all members and all effective properties remain
required. No defaults are filled. Only exact built-in type IDs, declaring identities, kind/color/
reference symbols and version 1 are accepted.

The full input must be valid UTF-8 without BOM. All JSON escapes, including surrogate pairs, decode
to UTF-8 without replacement or normalization; lone surrogates and raw controls fail. Comments,
trailing commas, single quotes, incomplete tokens and trailing garbage fail. Integers are exact
int64/uint64/Coordinate decimal values without floating conversion, leading plus/zeros, fraction or
exponent. There is no JavaScript safe-integer limit.

Validation requires nonempty dense IDs 0..N-1, root 0, Container owners, a complete unique-owner
acyclic reachable ownership tree and exact DFS preorder along owns. References require existing
distinct endpoints, one edge per source/kind and metadata-compatible endpoint types, including model
base chains. View/model and selection/model references remain independent. SizeConstraints must
have a valid range; negative coordinates and preferred outside min/max remain valid. ComboBox
selectedIndex must be null or in items, GridLayout columnCount positive, TextField text unchanged by
sanitizeSingleLine, and each referenced RadioGroup may have at most one selected member.

Success engages document with error.code == none. Failure returns no document and a stable
`UiDescriptionJsonParseErrorCode`: invalid_utf8, syntax_error, duplicate_member, unknown_member,
missing_member, wrong_value_type, unsupported_format, unsupported_format_version,
unknown_component_type, unknown_property, property_kind_mismatch, invalid_property_value,
invalid_ownership or invalid_reference. `byte_offset` is a zero-based input byte position at/near
error detection; EOF may equal input.size(), and semantic errors may point to the containing
node/edge. Allocation failures propagate. No Components, factory, property restoration, ownership
adoption, reference resolution, callback suppression or file I/O are involved. See
[ADR 0187](../adr/0187-m8-json-v1-parser-validated-document-model.md).

## Normative complete example

A freshly constructed Label encodes exactly as follows (including a final LF):

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
