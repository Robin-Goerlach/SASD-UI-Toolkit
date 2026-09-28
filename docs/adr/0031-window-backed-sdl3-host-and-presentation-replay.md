# ADR 0031 – Window-backed SDL3 host, presentation replay and desktop event boundary

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

ADR 0030 proved the generic Rendered M3 contracts against a real SDL3 software renderer and SDL_ttf
without introducing a desktop window. The next requirement is a real window-backed path that can
reuse the same semantic Widgets, DisplayList commands and font metrics.

A desktop window adds window/renderer lifetime, main-thread affinity, back-buffer lifecycle, logical
versus physical size, native close/resize/key/text events, explicit text-input activation, and repaint
requests caused by native surface state even when no Widget is semantically dirty.

### Decision

The experimental build-tree-only SDL3 target gains
`rendered::sdl3::Sdl3WindowBackend`.

It implements the existing `Backend`, `RenderDevice` and
`RenderedMeasurementContext` contracts. SDL/native types remain behind the adapter boundary.

Headless `Sdl3SoftwareDevice` and window-backed `Sdl3WindowBackend` share one private
`detail::Sdl3RenderContext`, keeping fill/stroke, text rasterization, clipping, style realization,
UTF-8 measurement and shaped scalar-boundary policy identical across both devices.

### Transactional lifecycle

`initialize()` requires the process main thread, initializes SDL video/SDL_ttf, creates
Window/Renderer/Font in local RAII holders, configures logical presentation and commits ownership only
after all steps succeed. Failure leaves the backend uninitialized. `shutdown()` is `noexcept`,
idempotent and releases resources in reverse dependency order.

### Explicit full-tree presentation replay

`PresentationCoordinator` gains:

```cpp
PresentationCoordinator::replay(root, sink)
```

This offers the entire current visual subtree in deterministic preorder regardless of ordinary dirty
flags. It does not manufacture Widget invalidation or change layout state.

This cleanly separates semantic changes from presentation-surface recovery:

```text
semantic/geometry change             native presentation loss/expose
          |                                      |
Widget invalidation                        replay current tree
          |                                      |
PresentationCoordinator::synchronize      PresentationCoordinator::replay
```

If a clean Widget is deferred during replay, it becomes normally pending so a later incremental pass
cannot forget the incomplete presentation.

### Initial frame policy

`Sdl3WindowBackend::presentFrame()` deliberately presents complete frames: clear the native target,
replay the full DisplayList, then call `SDL_RenderPresent()`. Dirty regions/retained backing stores
remain later optimizations.

### Logical coordinates and high-DPI foundation

Widget layout remains in logical coordinates. The backend separately exposes logical
`windowSize()`, physical `pixelSize()`, and `displayScale()`. Resize creates the existing
`ResizeEvent`; expose/pixel-size/display-scale changes request a fresh frame without inventing a
semantic Widget event.

### Event translation

The first window boundary translates only semantics already present in Core:

- close -> `QuitEvent`;
- resize -> `ResizeEvent`;
- supported key down/up -> `KeyEvent`;
- committed Unicode text -> `TextInputEvent`.

Printable text is not reconstructed from key codes. Native window focus is not Widget focus.
Pointer/mouse and richer composition remain deferred until corresponding semantic Core contracts
exist.

### Text input

SDL text input is enabled/disabled explicitly. The desktop demo mirrors logical FocusManager state:
text input is enabled while an eligible `TextField` owns focus and disabled otherwise.

### Validation

The experimental `sasd_ui_sdl3_demo` reuses the same semantic `Window`, `VBox`, `Label`,
`TextField` and `Button` classes as the terminal path.

Dedicated CI builds adapter and demo, creates a hidden real SDL window through SDL's offscreen video
driver, validates lifecycle/frame/key/text/resize/expose/close behavior, and exercises the end-to-end
Widget -> `PresentationCoordinator::replay()` -> `RenderedPresentationSink` -> `DisplayList` ->
`Sdl3WindowBackend` path.

### Consequences

- M3 now has a real window-backed SDL3 path using the existing semantic Widgets;
- presentation-surface recovery has backend-neutral `PresentationCoordinator::replay()`;
- SDL lifecycle/rendering remains adapter-local and main-thread-bound;
- KeyEvent/TextInputEvent semantics stay consistent with Terminal/Core;
- full-frame rendering is correctness-first and intentionally not optimized yet;
- pointer/hit-testing, richer IME composition, retained/dirty-region rendering and wider desktop
  validation remain follow-up work.

---

## Deutsch

### Kontext

ADR 0030 hat die generischen M3-Rendered-Verträge bereits mit echtem SDL3/SDL_ttf ohne Desktopfenster
bewiesen. Ein reales Fenster bringt zusätzlich Lifecycle, Main-Thread-Affinität, Backbuffer,
logische/physische Größe, native Events, Textinput-Aktivierung und Repaint-Bedarf durch Surface-
Zustand mit.

### Entscheidung

Das experimentelle Build-Tree-Target erhält
`rendered::sdl3::Sdl3WindowBackend`. Es implementiert `Backend`, `RenderDevice` und
`RenderedMeasurementContext`; SDL-Typen bleiben hinter der Adaptergrenze.

Headless- und Window-Pfad teilen `detail::Sdl3RenderContext`, damit Rendering- und Font-Policies
nicht auseinanderlaufen.

### Transaktionaler Lifecycle

`initialize()` erzeugt native Ressourcen zunächst in lokalen RAII-Haltern und übernimmt Ownership
erst nach vollständigem Erfolg. Fehler lassen das Backend uninitialisiert. `shutdown()` ist
`noexcept`, idempotent und räumt in umgekehrter Abhängigkeitsreihenfolge auf.

### Expliziter Full-Tree-Replay

`PresentationCoordinator::replay(root, sink)` bietet den vollständigen aktuellen visuellen Teilbaum
unabhängig von Dirty-Flags an, ohne Widget-Invalidierung oder Layoutzustand künstlich zu verändern.

Damit bleiben semantische Änderungen und native Presentation-Surface-Recovery sauber getrennt. Wird
ein zuvor cleanes Widget deferred, wird es normal pending und beim nächsten inkrementellen Pass erneut
versucht.

### Erste Frame-Policy

`presentFrame()` präsentiert bewusst vollständige Frames: Ziel löschen, vollständige DisplayList
abspielen, danach `SDL_RenderPresent()`. Dirty Regions/Retained Rendering sind spätere
Optimierungsschritte.

### Logische Koordinaten / High-DPI-Grundlage

Layout bleibt logisch. `windowSize()`, `pixelSize()` und `displayScale()` sind getrennt.
Resize erzeugt `ResizeEvent`; Expose/Pixel-/Scale-Änderungen fordern nur einen neuen Frame an.

### Event-Übersetzung

Zunächst werden nur vorhandene Core-Semantiken übersetzt: Close -> `QuitEvent`, Resize ->
`ResizeEvent`, unterstützte Tasten -> `KeyEvent`, committed Unicode -> `TextInputEvent`.
Druckbarer Text wird nicht aus Keycodes rekonstruiert. Window-Focus ist nicht Widget-Fokus.
Pointer/Maus und reichhaltigere Composition folgen später.

### Textinput

SDL-Textinput wird explizit aktiviert/deaktiviert und im Demo an den logischen Fokus eines geeigneten
`TextField` gekoppelt.

### Validierung

Das SDL3-Demo verwendet dieselben semantischen Widgets wie der Terminalpfad. Die dedizierte CI baut
Adapter und Demo, testet ein reales verstecktes SDL-Fenster über den Offscreen-Treiber und prüft den
End-to-End-Pfad Widget -> `PresentationCoordinator::replay()` -> `RenderedPresentationSink` ->
`DisplayList` -> `Sdl3WindowBackend`.

### Konsequenzen

M3 besitzt jetzt einen echten fenstergebundenen SDL3-Pfad. Surface-Recovery ist backendneutral
modelliert. Der Frame-Pfad ist bewusst korrekt statt vorzeitig optimiert. Pointer/Hit-Testing,
reichhaltigere IME-Composition, Dirty-Region-/Retained-Optimierung und breitere Desktopvalidierung
bleiben Folgearbeiten.
