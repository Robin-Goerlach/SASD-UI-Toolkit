# ADR 0131 – Rendered ComboBox popup pointer interaction uses final snapshots

**Status:** Accepted
**Date:** 2026-10-07

## English

### Context

ADR 0129 established owned Rendered ComboBox popup snapshots and ADR 0130 integrated their DisplayList
overlay into the SDL3 form demo. While open, the host had to quarantine every pointer event because the
transient rows are not Widget-tree children. Approximating rows from the ComboBox anchor or measuring
item text again would allow drawing and input geometry to diverge, especially with proportional fonts,
theme borders, above placement or a clamped horizontal origin.

### Decision

Rendered popup hit testing consumes only the exact final `RenderedComboBoxPopupPresentationSnapshot`
used by drawing. Border pixels do not belong to rows; `content_bounds`, `row_height` and the shared fixed
row helper determine half-open row geometry. Malformed geometry fails closed.

`RenderedComboBoxPopupPointerInteraction` validates copied item identity, preview identity, active Core
state and snapshot structure before every event. Motion previews a row. Primary press previews and arms
only its numeric row identity; matching release against a fresh snapshot commits through the existing
Core transaction. Primary press outside dismisses immediately and is consumed, forbidding click-through.
Mismatch, surface leave, keyboard takeover, resize or stale state retires the host-owned gesture.

Gesture state retains no Widget, ComboBox, metric, DisplayList or SDL pointer. It is reset before commit
or dismissal callbacks that may destroy the ComboBox. The SDL3 demo builds a fresh snapshot for every
open-popup pointer event and routes the overlay before ordinary Widget hit testing.

### Consequences

- Rendered and Terminal popup interaction now share semantic behavior without sharing backend geometry.
- Drawing and pointer identity cannot disagree through a second placement/measurement calculation.
- Covered Widgets never receive popup clicks or outside-dismiss presses.
- The helper stays SDL-independent and deterministically testable in `sasd_ui_rendered_tests`.
- Native pointer capture beyond the SDL window and popup scrolling remain deferred policies.

---

## Deutsch

### Kontext

ADR 0129 etablierte owned Rendered-ComboBox-Popup-Snapshots; ADR 0130 integrierte deren
DisplayList-Overlay in das SDL3-Form-Demo. Im offenen Zustand musste der Host bisher jedes Pointer-Event
quarantänisieren, weil die transienten Rows keine Widget-Tree-Kinder sind. Eine erneute Ableitung aus
ComboBox-Anchor oder Textmessung könnte Darstellungs- und Eingabegeometrie auseinanderlaufen lassen,
insbesondere bei proportionalen Fonts, Theme-Borders, Above-Placement oder geklemmtem X-Ursprung.

### Entscheidung

Rendered-Popup-Hit-Testing konsumiert ausschließlich den endgültigen
`RenderedComboBoxPopupPresentationSnapshot`, den auch das Drawing verwendet. Border-Pixel gehören zu
keiner Row; `content_bounds`, `row_height` und der gemeinsame Fixed-Row-Helper bestimmen die halb-offene
Zeilengeometrie. Fehlerhafte Geometrie schlägt fail-closed fehl.

`RenderedComboBoxPopupPointerInteraction` validiert vor jedem Event kopierte Item-Identität,
Preview-Identität, aktiven Core-State und Snapshot-Struktur. Motion previewt eine Row. Primary-Press
previewt und armt nur deren numerische Identität; ein passendes Release gegen einen frischen Snapshot
committet über die bestehende Core-Transaktion. Primary-Press außerhalb dismisses sofort und wird
konsumiert, sodass kein Click-through entsteht. Mismatch, Surface-Leave, Keyboard-Übernahme, Resize oder
staler State beendet die host-eigene Geste.

Der Gesture-State hält keine Widget-, ComboBox-, Metrik-, DisplayList- oder SDL-Pointer. Vor Commit- oder
Dismiss-Callbacks, die die ComboBox zerstören könnten, wird er zurückgesetzt. Das SDL3-Demo baut für
jedes Pointer-Event bei offenem Popup einen frischen Snapshot und routet das Overlay vor normalem
Widget-Hit-Testing.

### Konsequenzen

- Rendered und Terminal teilen nun die Popup-Interaktionssemantik ohne gemeinsame Backend-Geometrie.
- Drawing und Pointer-Identität können nicht durch eine zweite Placement-/Measurement-Rechnung abweichen.
- Überdeckte Widgets erhalten weder Popup-Klicks noch Outside-Dismiss-Presses.
- Der Helper bleibt SDL-unabhängig und in `sasd_ui_rendered_tests` deterministisch testbar.
- Native Pointer-Capture-Policy außerhalb des SDL-Fensters und Popup-Scrolling bleiben vertagt.
