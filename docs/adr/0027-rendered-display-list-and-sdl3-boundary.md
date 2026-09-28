# ADR 0027 – Deterministic rendered display list and optional SDL3 adapter boundary

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

M2 proved that visible backend work becomes easier to test and reason about when semantic widget state
is not coupled directly to operating-system I/O. The terminal path deliberately separates:

```text
semantic widgets
      |
PresentationSink
      |
 ScreenBuffer
      |
ANSI encoder / TerminalSession
      |
 native terminal
```

M3 needs the same property for a rendered desktop backend. SDL3 is a strong candidate for window,
input and low-level rendering integration, but making semantic widgets or the Core emit SDL-specific
objects would couple the toolkit to one implementation too early. It would also make most graphical
presentation behavior require a live desktop window in tests.

### Decision

M3 introduces a separately linkable `SASD::UI::Rendered` target.

The first rendered presentation primitive is a deterministic `rendered::DisplayList`: an ordered,
value-type list of backend-neutral drawing commands expressed only with SASD geometry/style types and
standard C++ values.

The initial command vocabulary is intentionally small:

- fill a logical rectangle with a portable `Color`;
- stroke a logical rectangle with a portable `Color` and logical thickness;
- draw UTF-8 text at a logical origin with `TextStyle`.

A later rendered `PresentationSink` converts semantic `Window`, `Label`, `Button` and
`TextField` state into that display list. A platform adapter then executes the list.

SDL3, when introduced, is an **optional adapter below this boundary**. SDL handles, events, surfaces,
renderers, textures and other SDL types must not appear in Core headers or the generic rendered
command vocabulary.

The intended direction is:

```text
semantic widgets
      |
PresentationCoordinator
      |
RenderedPresentationSink
      |
   DisplayList
      |
 renderer/device adapter
      |
  SDL3 or another substrate
```

### Rationale

A small display list gives M3 a deterministic seam similar to the terminal `ScreenBuffer`:

- widget-to-drawing translation can be unit tested without a display server;
- SDL3 remains replaceable and optional;
- platform renderers can replay exactly the same ordered commands;
- clipping, theme mapping and DPI conversion can be added at a clear boundary;
- normal application code continues to use semantic SASD widgets rather than renderer APIs.

The display list is **not** a new general-purpose graphics engine. It is only the minimum presentation
intermediate representation required by current toolkit widgets.

### Validation and invariants

- command order is preserved exactly;
- malformed negative rectangle extents are rejected rather than normalized silently;
- zero-area rectangles and empty text produce no drawing command;
- command data owns its text payload so later widget mutations cannot alter an already-produced frame;
- logical coordinates remain backend-neutral; an SDL adapter performs device-pixel/DPI conversion.

### Deliberately deferred

This ADR does not yet define:

- an SDL3 dependency or package-discovery policy;
- font loading/rasterization;
- images, paths, gradients or arbitrary alpha blending;
- clipping stacks or transforms;
- retained GPU resources;
- animation;
- the final desktop event loop;
- pointer-event semantics;
- High-DPI policy beyond keeping logical coordinates separate from device pixels.

Those are added only when the first real rendered adapter makes their requirements concrete.

### Consequences

- M3 gains a testable rendered presentation layer before opening a real window;
- `SASD::UI::Rendered` can evolve independently from Terminal and Core;
- SDL3 can be added without contaminating normal application headers;
- the command vocabulary may grow during pre-1.0 development, but only for demonstrated widget needs;
- native-peer backends remain free to bypass this rendered path where native controls are preferable.

---

## Deutsch

### Kontext

M2 hat gezeigt, dass sichtbare Backends deutlich leichter testbar und verständlich bleiben, wenn der
semantische Widget-Zustand nicht direkt an Betriebssystem-I/O gekoppelt wird. Der Terminalpfad trennt
bewusst:

```text
semantische Widgets
       |
PresentationSink
       |
 ScreenBuffer
       |
ANSI-Encoder / TerminalSession
       |
 natives Terminal
```

M3 benötigt dieselbe Eigenschaft für ein gerendertes Desktop-Backend. SDL3 ist ein geeigneter Kandidat
für Fenster, Eingabe und Low-Level-Rendering. Würden Core oder semantische Widgets jedoch direkt
SDL-Objekte erzeugen, wäre das Toolkit zu früh an eine konkrete Implementierung gebunden und viele
Darstellungstests würden ein echtes Desktopfenster benötigen.

### Entscheidung

M3 erhält ein separat linkbares Target `SASD::UI::Rendered`.

Das erste Rendered-Primitiv ist eine deterministische `rendered::DisplayList`: eine geordnete Liste
von Value-Type-Zeichenbefehlen, die ausschließlich SASD-Geometrie-/Style-Typen und Standard-C++-Werte
verwendet.

Das erste Befehlsvokabular bleibt bewusst klein:

- logisches Rechteck mit einer portablen `Color` füllen;
- logisches Rechteck mit `Color` und logischer Linienstärke umranden;
- UTF-8-Text an einem logischen Ursprung mit `TextStyle` zeichnen.

Ein späterer gerenderter `PresentationSink` übersetzt den semantischen Zustand von `Window`,
`Label`, `Button` und `TextField` in diese DisplayList. Ein Plattformadapter führt die Liste aus.

SDL3 wird dabei als **optionaler Adapter unterhalb dieser Grenze** eingeführt. SDL-Handles, Events,
Surfaces, Renderer, Texturen oder andere SDL-Typen dürfen weder in Core-Header noch in das generische
Rendered-Befehlsvokabular gelangen.

Die Zielrichtung lautet:

```text
semantische Widgets
       |
PresentationCoordinator
       |
RenderedPresentationSink
       |
   DisplayList
       |
 Renderer-/Device-Adapter
       |
  SDL3 oder Alternative
```

### Begründung

Eine kleine DisplayList gibt M3 eine deterministische Testgrenze ähnlich dem Terminal-`ScreenBuffer`:

- Widget-zu-Zeichenbefehl-Übersetzung ist ohne Display Server testbar;
- SDL3 bleibt optional und austauschbar;
- Plattformrenderer spielen dieselben geordneten Befehle ab;
- Clipping, Theme-Mapping und DPI-Umrechnung erhalten eine klare Grenze;
- normaler Anwendungscode verwendet weiterhin semantische SASD-Widgets statt Renderer-APIs.

Die DisplayList ist **keine** neue allgemeine Grafikengine. Sie bildet nur die minimale
Zwischendarstellung ab, die die aktuellen Toolkit-Widgets benötigen.

### Validierung und Invarianten

- Befehlsreihenfolge bleibt exakt erhalten;
- negative Rechteckausdehnungen werden abgewiesen und nicht still normalisiert;
- Rechtecke ohne Fläche und leerer Text erzeugen keinen Zeichenbefehl;
- Befehle besitzen ihre Textdaten selbst, sodass spätere Widget-Änderungen einen bereits erzeugten
  Frame nicht verändern;
- logische Koordinaten bleiben backendneutral; Device-Pixel-/DPI-Umrechnung gehört in den Adapter.

### Bewusst später

Diese ADR legt noch nicht fest:

- SDL3-Abhängigkeit oder Package-Discovery;
- Font-Laden/Rasterisierung;
- Bilder, Pfade, Verläufe oder beliebiges Alpha-Blending;
- Clip-Stacks oder Transformationen;
- gehaltene GPU-Ressourcen;
- Animation;
- finalen Desktop-Eventloop;
- Pointer-Event-Semantik;
- High-DPI-Regeln über die Trennung logischer Koordinaten von Device-Pixeln hinaus.

Diese Punkte folgen erst aus den konkreten Anforderungen des ersten realen Rendered-Adapters.

### Konsequenzen

- M3 erhält eine testbare Rendered-Schicht, bevor ein echtes Fenster geöffnet wird;
- `SASD::UI::Rendered` kann unabhängig von Terminal und Core wachsen;
- SDL3 kann ergänzt werden, ohne normale Anwendung-Header zu verunreinigen;
- das Befehlsvokabular darf sich während Pre-1.0 anhand realer Widget-Anforderungen erweitern;
- Native-Peer-Backends können diesen Rendered-Pfad weiterhin umgehen, wenn native Controls sinnvoller
  sind.
