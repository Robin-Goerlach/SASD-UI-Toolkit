# M8 Developer Productivity Foundations Exit Review

Status: completed on 2026-11-17

## Reviewed baseline and scope

This review audited `main` at `6811f6acc6f860e8fdda1158857c7153e1a1132b`, the merge of M8.29.
The closure record is the merge commit that contains this document and
[ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md); Git history is the
authoritative identifier for that commit.

M8 covers the backend-neutral, machine-readable foundations needed by external developer tools:
built-in type and property metadata, typed property access and defaults, validation metadata,
owning UI descriptions, canonical JSON v1/v2 persistence, reconstruction, the current resource
pipeline and an installed headless tooling consumer contract. This review did not add or change a
runtime, public API, persistence format or backend contract.

## Exit criteria

| Area | Result | Evidence and boundary |
|---|---|---|
| A. Type system | **PASS** | The immutable catalog contains 29 stable built-in IDs. Exact runtime identification is separate from persistence. Authoring metadata classifies 26 types as constructible and `ListModel`, `TableModel` and `TreeModel` as abstract contracts. |
| B. Property system | **PASS** | The catalog contains 28 descriptors, typed read/write access, target-relative defaults, inheritance without copied derived descriptors, owned snapshots and every current scalar, collection, optional and compound value kind. |
| C. Validation | **PASS** | Instance-free numeric ranges, string normalization, compound validation, same-node cross-property validation, component-reference metadata and graph validation are consumed by format-neutral document validation. |
| D. Description and persistence | **PASS** | Owning nodes, stable node IDs, ordered ownership and component references are represented by the format-neutral document. Strict validated JSON v1 and resource-aware JSON v2 retain deterministic canonical bytes and normalize cosmetic input ordering. |
| E. Reconstruction | **PASS** | Phased construction restores constructible components, ownership, references, properties and resources. Validation or construction failure returns no partial root; abstract contracts fail explicitly. |
| F. Resource system | **PASS** | `ResourceKey`, the owning typed `TextStyle` dictionary, typed references, six exact property capabilities, explicit caller-ordered scoping, JSON v2 persistence and one-time reconstruction/materialization form a complete vertical pipeline for the current M8 scope. |
| G. Tooling | **PASS** | All 28 properties have an authoring category. Type constructibility, defaults, validation, references, resources and document editing are discoverable without Component instances. The demo and installed consumer use public headers and `SASD::UI` only. |
| H. Packaging | **PASS** | Installation exports public headers and the `SASD::UI`, `SASD::UI::Terminal` and `SASD::UI::Rendered` CMake targets. The external Debug/Release package consumer exercises the M8 contract through `find_package(SASDUIToolkit 0.2 CONFIG REQUIRED)`. |
| I. Portability | **PASS** | The reviewed baseline passed GCC, Clang, MSVC, AppleClang, Linux, Windows, macOS, sanitizer, SDL3, GTK Native, AppKit Native and Release CI. The visible human macOS/Cocoa M3 validation remains a separate external gate. |

No M8 blocker was found.

## Implemented foundations

The 29 implementation slices establish one coherent composition path:

1. stable built-in type identity and exact runtime identification;
2. complete current built-in property schema, typed access, defaults and snapshots;
3. machine-readable normalization, validation, reference, graph and authoring metadata;
4. owning UI descriptions with strict canonical JSON v1 and resource-aware JSON v2;
5. atomic reconstruction of components, ownership, references, properties and resources;
6. resource identity, ownership, capability, resolution, persistence and materialization, proven with
   the concrete `TextStyle` family; and
7. instance-free discovery and document editing through the installed public API.

## Deferred non-blockers

The following work is deliberately outside the completed M8 scope:

- localized display names, labels, help text and tooltips;
- icons, editor kinds, Property Grid ordering/policy and palette presentation;
- additional resource families such as fonts, images, icons, brushes, themes, binary data,
  animation or localization resources;
- a visual RAD designer, IDE extension, language server, reflection registry, generic factory or
  object activator; and
- further milestones, which will be defined only from concrete product or consumer requirements.

These are consumer-driven future decisions, not missing parts of the proven machine-readable M8
contract. In particular, the visual RAD designer remains a separate consumer or sister project.

## External gates and CI evidence

The reviewed baseline had successful Main CI, GTK Native CI and AppKit Native CI runs, including the
GCC/Clang/MSVC/AppleClang Debug matrix, Clang ASan+UBSan, SDL3, the three-platform Release matrix,
installed-package consumers, release-install verification and repository hygiene. Local exit
validation repeated WSL/GCC Debug and Release, complete CTest suites, demos, installed consumers,
install/hygiene checks and documentation-link checks. The exact closure head was then required to
pass the full CI matrix, including MSVC Debug and Release, before the closure PR was accepted.

The visible manual macOS/Cocoa validation required by M3 is still **OPEN**. Automated AppleClang and
AppKit CI do not replace that human-visible platform gate. It is an external platform/release gate,
not a reason to extend M8.

Open side-track/documentation PRs #30, #32, #63, #100, #103 and #105 were reviewed read-only. The
required contracts are already present on `main`; those PRs do not block the M8 Core exit.

## Conclusion and next boundary

M8 Developer Productivity Foundations is **completed**. The current resource-system parent is
closed because its complete identity-to-materialization pipeline is proven; richer resource families
remain deferred beyond M8 until a real consumer requires one.

The project version remains 0.2.0 and the changelog remains Unreleased. This decision creates no RC,
tag or release. The next work, neither started nor implied by this review, is:

1. the visible macOS/Cocoa M3 validation; then
2. a separate v0.2.0 release-readiness review.
