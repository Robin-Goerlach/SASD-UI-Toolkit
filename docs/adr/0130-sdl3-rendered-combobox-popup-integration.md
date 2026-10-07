# ADR 0130 – SDL3 form demo integration for Rendered ComboBox popup overlays

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0129 introduced an SDL-independent Rendered ComboBox popup snapshot and DisplayList composition
pipeline. The real SDL3 form demo still replayed only the ordinary Widget tree, so an open ComboBox
changed its collapsed indicator but did not yet show graphical item rows in the actual desktop window.

The integration must preserve the architecture already established by the Terminal path:

- ComboBox Core owns semantic items, committed selection, preview and open state;
- VBox/Window own ordinary Widget layout;
- the Rendered layer owns popup measurement/placement/DisplayList commands;
- SDL3 remains the concrete metric/event/render-device adapter.

The overlay also creates an input-boundary problem. Until Rendered popup row hit testing exists, ordinary
Widget hit testing cannot be allowed to continue underneath a visible popup: the Widget tree does not
contain the transient rows and could activate controls visually covered by them.

### Decision

Add the same Core ComboBox demonstration used by the Terminal form to
`rendered_sdl3_form_demo.cpp` with items Portable/Terminal/Rendered.

#### Full-frame composition

The host's full-frame pipeline becomes:

`Widget replay -> base DisplayList -> optional ComboBox popup composition -> Sdl3WindowBackend::presentFrame()`.

The base Widget frame is still rebuilt through `PresentationCoordinator::replay()` because the SDL
back buffer is not treated as retained state. When the ComboBox is open, the host resolves its arranged
bounds to root logical coordinates, then calls ADR 0129
`composeComboBoxPopupDisplayList()` with:

- that absolute anchor;
- the current root Window bounds as logical viewport;
- `Sdl3WindowBackend` as the active RenderedMeasurementContext;
- the demo's black surface background.

The returned DisplayList is a value-owned overlay frame. The semantic Widget tree is not modified and
no SDL/native ComboBox control is created.

Absolute Widget origins are accumulated by the demo in widened arithmetic and fail closed if they cannot
be represented by public `Coordinate`. This remains host/presentation-tree knowledge rather than
moving absolute screen geometry into Core ComboBox.

#### Interaction boundary

Keyboard behavior works immediately because the existing focused Core ComboBox already owns F4,
Alt+Down/Alt+Up, arrow-preview, Enter commit and Escape cancel.

Collapsed Primary-click opening also works through the existing Core/PointerRouter path while the popup
is closed.

Popup-row pointer behavior is deliberately not approximated in this slice. While the Rendered popup is
open, the SDL host consumes pointer samples before ordinary Widget routing and calls
`PointerRouter::leaveRoot()` plus the rendered TextField gesture reset. This prevents both click-through
and an older captured/hovered Widget gesture from surviving behind the modal overlay.

A later Rendered hit-test/interaction slice will replace this quarantine with geometry derived from the
exact ADR 0129 snapshot.

#### Resize

A live logical window resize cancels an open ComboBox transaction before re-layout. Popup placement was
computed against the old viewport and may no longer fit either side afterward. Core cancellation clears
preview/open state while preserving committed selection; reopening reseeds preview from that committed
value.

### Consequences

Positive:

- the real SDL3 desktop demo now displays open ComboBox item rows;
- Terminal and Rendered demos use the same Core ComboBox semantics with backend-specific presentation;
- SDL-specific code still contains no semantic ComboBox state;
- overlay restoration is naturally achieved by rebuilding the ordinary base frame after close;
- pointer click-through is prevented before Rendered popup hit testing exists;
- resize cannot leave an obsolete popup placement alive.

Trade-offs:

- open popup rows are keyboard-only in the SDL demo for this slice;
- moving/clicking the pointer while the popup is open is consumed rather than interpreted;
- absolute Widget-to-root resolution is duplicated at the demo host boundary until a broader reusable
  visual-geometry API is justified;
- full-frame and popup composition favor correctness/value ownership over command-buffer optimization.

### Deliberately deferred

- Rendered popup row hit testing;
- Rendered pointer hover preview, click commit and outside dismissal;
- stronger/native pointer capture policy for Rendered popup rows;
- scrolling/maximum visible rows;
- richer selection/theme/shadow primitives;
- native ComboBox peers.

---

## Deutsch

### Kontext

ADR 0129 hat einen SDL-unabhängigen Rendered-ComboBox-Popup-Snapshot samt DisplayList-Komposition
eingeführt. Das echte SDL3-Form-Demo replayte bisher aber nur den normalen Widget-Tree. Eine geöffnete
ComboBox änderte damit zwar ihren collapsed Indicator, zeigte im tatsächlichen Desktopfenster jedoch
noch keine grafischen Item-Rows.

Die Integration soll die bereits im Terminalpfad etablierte Architektur bewahren:

- ComboBox-Core besitzt Items, committed Selection, Preview und Open-State;
- VBox/Window besitzen normales Widget-Layout;
- die Rendered-Schicht besitzt Popup-Measurement/-Placement und DisplayList-Commands;
- SDL3 bleibt konkreter Metric-/Event-/RenderDevice-Adapter.

Das Overlay erzeugt zusätzlich eine Input-Grenze. Solange Rendered-Popup-Row-Hit-Testing fehlt, darf
normales Widget-Hit-Testing unter einem sichtbaren Popup nicht weiterlaufen: Die transienten Rows stehen
nicht im Widget-Tree und könnten Controls aktivieren, die optisch vom Popup bedeckt sind.

### Entscheidung

`rendered_sdl3_form_demo.cpp` erhält dieselbe Core-ComboBox-Demonstration wie das Terminalformular mit
den Items Portable/Terminal/Rendered.

#### Full-Frame-Komposition

Die Host-Pipeline lautet nun:

`Widget-Replay -> Base-DisplayList -> optionale ComboBox-Popup-Komposition -> Sdl3WindowBackend::presentFrame()`.

Der normale Widget-Frame wird weiterhin über `PresentationCoordinator::replay()` komplett aufgebaut,
weil der SDL-Backbuffer nicht als retained State betrachtet wird. Ist die ComboBox offen, löst der Host
ihre arrangierten Bounds in logische Root-Koordinaten auf und ruft
`composeComboBoxPopupDisplayList()` aus ADR 0129 auf mit:

- diesem absoluten Anchor;
- den aktuellen Root-Window-Bounds als logischem Viewport;
- `Sdl3WindowBackend` als aktivem `RenderedMeasurementContext`;
- dem schwarzen Demo-Surface-Background.

Die zurückgegebene DisplayList ist ein value-owned Overlay-Frame. Der semantische Widget-Tree wird
nicht verändert und es entsteht kein SDL-/Native-ComboBox-Control.

Absolute Widget-Ursprünge werden im Demo mit verbreiterter Arithmetik akkumuliert und schlagen
fail-closed fehl, falls sie nicht in `Coordinate` darstellbar sind. Dieses Wissen bleibt
Host-/Presentation-Tree-Verantwortung und wandert nicht in die Core-ComboBox.

#### Interaction-Grenze

Keyboard-Bedienung funktioniert sofort, weil die fokussierte Core-ComboBox bereits F4,
Alt+Down/Alt+Up, Arrow-Preview, Enter-Commit und Escape-Cancel besitzt.

Auch Primary-Click-Opening der collapsed ComboBox läuft bei geschlossenem Popup bereits über den
vorhandenen Core-/PointerRouter-Pfad.

Popup-Row-Pointer-Verhalten wird in diesem Slice bewusst nicht approximiert. Solange das Rendered-Popup
offen ist, konsumiert der SDL-Host Pointer-Samples vor normalem Widget-Routing und ruft
`PointerRouter::leaveRoot()` plus Reset des Rendered-TextField-Gesture-State auf. Dadurch entstehen
weder Click-through noch alte Capture-/Hover-Gesten hinter dem modalen Overlay.

Ein späterer Rendered-Hit-Test-/Interaction-Slice ersetzt diese Quarantäne durch Geometrie aus exakt
dem ADR-0129-Snapshot.

#### Resize

Ein Live-Resize des logischen Fensters cancelt eine offene ComboBox-Transaktion vor dem Re-Layout.
Popup-Placement wurde gegen den alten Viewport berechnet und könnte danach auf keiner Seite mehr
vollständig passen. Core-Cancellation löscht Preview/Open-State, erhält aber committed Selection;
erneutes Öffnen seedet Preview wieder aus diesem committed Wert.

### Konsequenzen

Positiv:

- das echte SDL3-Desktop-Demo zeigt nun offene ComboBox-Item-Rows;
- Terminal- und Rendered-Demo verwenden dieselbe Core-ComboBox-Semantik mit backend-spezifischer
  Presentation;
- SDL-spezifischer Code enthält weiterhin keinen semantischen ComboBox-State;
- Overlay-Restoration entsteht natürlich durch Neuaufbau des normalen Base-Frames nach dem Schließen;
- Pointer-Click-through wird verhindert, bevor Rendered-Popup-Hit-Testing existiert;
- Resize kann kein veraltetes Popup-Placement am Leben halten.

Abwägungen:

- offene Popup-Rows sind in diesem Slice im SDL-Demo nur per Keyboard bedienbar;
- Pointer-Move/-Click bei offenem Popup wird konsumiert statt interpretiert;
- absolute Widget-zu-Root-Auflösung ist vorerst am Demo-Host dupliziert, bis mehrere Verbraucher eine
  allgemeinere Visual-Geometry-API rechtfertigen;
- Full-Frame- und Popup-Komposition bevorzugen Korrektheit/Value-Ownership vor Command-Buffer-
  Optimierung.

### Bewusst vertagt

- Rendered-Popup-Row-Hit-Testing;
- Rendered Pointer-Hover-Preview, Click-Commit und Outside-Dismissal;
- stärkere/native Pointer-Capture-Policy für Rendered-Popup-Rows;
- Scrolling/maximal sichtbare Rows;
- reichere Selection-/Theme-/Shadow-Primitiven;
- native ComboBox-Peers.
