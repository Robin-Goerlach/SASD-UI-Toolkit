# Headless tooling integration

## Audience and boundary

This document describes the installed public Core contract for external authoring, inspection and
IDE-adjacent tools. Such a consumer can discover and compose the built-in type, property,
validation, reference, resource and UI-description contracts without creating runtime Components
or initializing Terminal, Rendered or native backends.

This is an integration point, not an IDE plugin or visual designer. A complete RAD designer remains
a separate sister project. The Core supplies stable machine identities and semantic constraints;
the consumer supplies its own user interface, localization, presentation and editing policy.

## Built-in types and constructibility

`builtInComponentTypes()` enumerates the immutable `ComponentTypeMetadata` catalog. Each descriptor
contains its stable `type_id`, direct `base_type_id` and coarse `ComponentTypeKind`. Exact,
case-sensitive lookup is available through `findBuiltInComponentType()`.

`builtInComponentTypeAuthoringMetadata()` classifies an exact descriptor as
`constructible` or `abstract_contract`. The classification means “may this type be authored as a
concrete built-in UI-description node?” It is not general C++ abstractness and does not expose a
factory. The current abstract contracts are `sasd.ui.ListModel`, `sasd.ui.TableModel` and
`sasd.ui.TreeModel`; they remain catalog members because reference-endpoint and schema tooling needs
their identities. Their concrete `String*Model` descendants are constructible.

Descriptor identity is by complete value: `type_id`, `base_type_id` and `kind`. A copied valid
descriptor succeeds. An unknown ID, wrong case, changed base or changed kind returns `std::nullopt`.
Tools must treat a missing classification as non-constructible.

## Base chains and effective properties

The property schema intentionally has no second flattened catalog. Starting at a chosen type, a
consumer follows `ComponentTypeMetadata::base_type_id` to the root, reverses that chain and appends
`declaredBuiltInComponentProperties(type_id)` from base to derived. This produces the effective
property order used by snapshots, documents and reconstruction.

Each `ComponentPropertyMetadata` keeps the ID of the type that actually declares it. For example,
the effective properties of `sasd.ui.Button` contain `visible`, but that descriptor remains
`sasd.ui.Widget.visible`. Tools must not fabricate `Button.visible` or duplicate inherited metadata.
`findBuiltInComponentProperty()` performs the same inherited lookup when one property is needed.

## Property contract composition

For each exact property descriptor, a headless consumer composes the existing focused APIs:

- `ComponentPropertyMetadata::value_kind` selects the concrete authoring value representation;
- `builtInComponentPropertyAuthoringMetadata()` returns a non-localized semantic category;
- the eight `builtIn*PropertyDefault()` families return target-type-relative defaults without an
  instance;
- `builtInSignedIntegerPropertyRange()` and `builtInUnsignedIntegerPropertyRange()` expose static
  integer domains;
- `builtInStringPropertyNormalization()` exposes identity or single-line UTF-8 normalization;
- `builtInSizeConstraintsPropertyValidation()` exposes the compound minimum/maximum invariant;
- `builtInOptionalUnsignedIntegerCrossPropertyValidation()` describes the
  `ComboBox.selectedIndex` dependency on the exact `ComboBox.items` descriptor;
- `builtInComponentReferenceMetadata()` and
  `builtInComponentReferenceGraphValidations()` describe endpoint types and the RadioGroup graph
  rule; and
- `builtInComponentPropertyResourceMetadata()` opts exact properties into one concrete
  `ResourceKind`.

These are deliberately not copied into a `ToolingPropertyDescriptor` mega-structure. Keeping schema,
defaults, validation, references, resources and authoring categories separate makes ownership and
evolution explicit and avoids divergent catalogs.

All descriptor-based queries validate complete descriptor identity and fail closed. A tool must not
infer categories, validation or resource capability from spelling. In the current catalog all 28
properties have authoring categories and exactly six `textStyle` properties are resource-capable.

## Resource capability

`ResourceKind`, `ResourceKey`, `ResourceReference` and
`builtInComponentPropertyResourceMetadata()` form the authoring discovery boundary. A property may
be assigned a resource reference only when its exact descriptor returns engaged metadata with the
same kind. Absence is a deliberate “not resource-capable” result, not a request to apply a name
heuristic.

`ResourceDictionary` and explicit caller-ordered scopes remain available to runtime consumers, but
metadata discovery performs no lookup or resolution. `TextStyle` is the only current resource
family. Further families remain deferred until a real backend or tooling consumer establishes a
concrete requirement.

## Format-neutral documents and JSON

`BuiltInUiDescriptionDocument` is the owning, format-neutral editing model. A tool may create and
edit nodes, ordered ownership, typed property collections, reference edges, resource definitions and
typed resource-reference properties directly. Because it is a public aggregate, mutation alone does
not preserve validity; v2 encoding and both parsers validate and fail atomically.

The JSON entry points remain version-specific and unchanged:

- `parseBuiltInUiDescriptionJson()` reads strict JSON v1 into a validated document;
- `encodeBuiltInUiDescriptionJson(const Component&)` captures a runtime graph and writes canonical
  v1;
- `parseBuiltInUiDescriptionJsonV2()` reads strict self-contained resource-aware v2; and
- `encodeBuiltInUiDescriptionJsonV2(document)` validates and writes a format-neutral document.

There is no format version 3 and no tooling metadata is persisted. Authoring tools that must retain
v2 resource identity keep the document as their source of truth rather than reconstructing that
identity from materialized runtime Components.

## Reconstruction boundary

`reconstructBuiltInUiDescription(document)` is an optional runtime/preview boundary after document
editing and validation. It owns the controlled construction policy, reference ordering, property
restoration and one-time resource materialization. Constructibility metadata does not call or
replace it. Discovery therefore has no Component construction, events, resource resolution,
registration or backend side effects.

## Typical external workflow

```text
Load installed SASD::UI public metadata
  -> enumerate built-in types
  -> retain only constructible authoring types
  -> traverse base chains and declared properties
  -> group by semantic category
  -> obtain defaults, validation, references and resource capability
  -> create or edit BuiltInUiDescriptionDocument
  -> encode JSON v2 or parse JSON v1/v2
  -> optionally reconstruct a runtime preview
```

The headless example `sasd_ui_tooling_metadata_demo` demonstrates the discovery half. The installed
package consumer additionally proves that a separate project needs only public installed headers and
the `SASD::UI` target.

## Intentionally deferred human-facing metadata

Core does not define `displayName`, localized names, descriptions, help text, tooltips, editor kinds
or widgets, Property Grid order, browsable/read-only/advanced/favorite flags, icons, palette groups,
documentation URLs, category display text or default visual sizes. No real consumer has yet defined
the localization, presentation and editing policies these fields would encode. Deferral is a
deliberate scope decision, not an open M8 blocker.

See [ADR 0199](../adr/0199-m8-headless-tooling-integration-contract.md) for the decision rationale.
