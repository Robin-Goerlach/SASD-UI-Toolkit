# ADR 0030 – Headless SDL3 software adapter before desktop-window lifecycle

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

ADR 0027 defined the SDL-independent `rendered::DisplayList`, ADR 0028 added the rendered text metric
contract needed by `TextField`, and ADR 0029 introduced the `RenderDevice` replay boundary below the
DisplayList.

M3 now needs evidence that those abstractions survive contact with a real rendering/text stack. Going
directly from the headless command model to a complete desktop application would combine several
independent unknowns in one change:

- command execution;
- color realization;
- clipping;
- UTF-8 font rasterization;
- shaped text metrics and caret boundaries;
- window creation/destruction;
- event-loop ownership;
- resize/high-DPI policy;
- input translation;
- frame presentation/swap timing.

If all of those are introduced together, failures are harder to localize and the window lifecycle can
accidentally dictate interfaces that really belong to the lower drawing layer.

The second visible/rendered path also makes one previously hidden ambiguity concrete:
`Color::default_color` on a filled rectangle can mean either default **background** (surface/widget
erasure) or default **foreground** (for example the TextField insertion caret). A concrete RGB
renderer cannot infer that semantic role from the color value alone.

### Decision

M3 first introduces an **experimental, headless SDL3 software-rendering adapter** below the existing
`RenderDevice` boundary.

The initial adapter is intentionally build-tree-only:

- target: `SASD::UI::Rendered::SDL3`;
- implementation: `rendered::sdl3::Sdl3SoftwareDevice`;
- SDL3 and SDL_ttf remain optional dependencies;
- the target is not installed/exported yet;
- normal Core/Terminal/Rendered builds remain dependency-free from SDL;
- no SDL type is added to the generic public Core or Rendered headers.

`Sdl3SoftwareDevice` implements both:

- `RenderDevice`, for actual execution of `FillRectCommand`, `StrokeRectCommand` and
  `DrawTextCommand`;
- `RenderedMeasurementContext`, so layout, TextField viewport and caret calculations use the same
  real font policy as drawing.

The device renders into an `SDL_Surface` through `SDL_CreateSoftwareRenderer`. This gives the
project a real SDL renderer without requiring X11, Wayland, Win32 or an AppKit window in the first
slice. A small diagnostic `pixelAt()` API exists only on this experimental adapter so tests can
inspect results. It is explicitly not a generic bitmap/screenshot API for the toolkit.

Text rendering uses SDL_ttf. The adapter:

- measures UTF-8 with the active font;
- rasterizes UTF-8 text;
- honors per-command text clipping;
- handles explicit line breaks on the same logical line-height grid used by measurement;
- asks SDL_ttf for positions in a shaped **complete text run** when mapping TextField scalar
  boundaries;
- returns `std::nullopt` if a requested Unicode-scalar boundary lies inside a shaped cluster and
  therefore cannot be represented faithfully by the current monotonic left-to-right caret contract.

This preserves ADR 0028: the adapter must not obtain caret positions by independently measuring UTF-8
prefixes whose shaping context can differ from the complete run.

### Default fill role

`FillRectCommand` gains a small semantic `FillRole`:

- `background` (the default);
- `foreground`.

The role only affects how `Color::default_color` is resolved. Explicit named colors remain explicit
palette choices.

Existing calls such as `fillRect(bounds)` continue to mean default background. The rendered
TextField caret records `FillRole::foreground`, so a default-colored caret resolves to foreground ink
instead of disappearing into the default background.

A full semantic background-color/theme model is still deliberately deferred. `FillRole` solves the
specific ambiguity demonstrated by a second concrete rendering path without expanding `TextStyle`
into a premature theme system.

### Styling policy in the first adapter

ADR 0025 says the current `TextStyle` attributes affect presentation but not intrinsic measurement.
The first SDL3 implementation follows that constraint:

- named colors map to an adapter-local initial RGB palette;
- `dim` reduces the realized foreground intensity;
- `inverse` paints the measured text run with foreground as its background and uses the adapter
  default background for glyph ink;
- `bold` is an additional one-logical-unit draw pass rather than a different metric/font contract;
- `underline` is a presentation overlay.

These are deliberately initial visual policies, not a desktop theme contract.

### Dependency and CI policy

The adapter can consume system-provided CMake packages or, when
`SASD_UI_FETCH_SDL3=ON`, fetch pinned upstream release tags.

The dedicated adapter CI currently pins:

- SDL 3.4.16;
- SDL_ttf 3.2.2.

SDL_ttf uses FreeType and HarfBuzz for the enabled initial path. PlutoSVG support is deliberately
disabled for this slice because color-SVG/emoji rendering is not needed to validate the current
DisplayList/TextField contracts.

No font binary is copied into the SASD UI Toolkit repository. Adapter tests receive a font path from
the build environment. Font licensing therefore remains separate from the toolkit source package.

The normal cross-platform compiler/release matrix does not enable the SDL3 adapter. A dedicated Linux
job fetches/builds the pinned adapter and exercises it headlessly. This preserves fast,
dependency-light Core builds while ensuring the concrete adapter is continuously compiled and tested.

### Threading and lifetime

SDL renderer drawing operations are treated as main-thread operations. The experimental device:

- must be created on the process main thread;
- records the creating thread;
- rejects later calls from a different thread;
- owns its SDL surface, software renderer and font through RAII;
- pairs successful SDL_ttf initialization with shutdown.

The class is non-copyable and non-movable so resource/thread affinity cannot be transferred
accidentally.

### Why no desktop window yet

This slice intentionally proves:

```text
RenderedPresentationSink
        |
    DisplayList
        |
DisplayListExecutor
        |
   RenderDevice
        |
Sdl3SoftwareDevice
        |
 SDL_Surface + SDL_ttf
```

before introducing:

```text
SDL_Window
SDL event pump
resize / DPI
input translation
present/swap lifecycle
```

The next M3 step is a window-backed SDL3 host/device that reuses the already-tested command and metric
contracts. Window/event lifecycle should be designed from that real implementation rather than added
speculatively to `RenderDevice`.

### Alternatives considered

#### Build the complete SDL3 window backend immediately

Rejected for this step. It combines rendering, text, window lifetime, events, input, resize and DPI in
one debugging surface and makes architectural mistakes harder to isolate.

#### Use SDL debug text instead of SDL_ttf

Rejected. The M3 TextField contract requires real UTF-8 font metrics and shaped caret boundaries; a
debug-text facility is not an adequate typography contract.

#### Put SDL types into the generic RenderDevice API

Rejected. It would reverse the dependency direction established by ADRs 0027 and 0029.

#### Install/export the SDL3 adapter immediately

Deferred. Dependency discovery, redistribution/runtime loading, font policy and desktop-window
lifecycle are not mature enough to become an installed compatibility promise.

#### Vendor a font in the repository for tests

Rejected. The adapter only needs a supplied font resource; bundling one would create an unnecessary
font-license/distribution commitment in the toolkit repository.

#### Add a complete background/theme system to distinguish the caret from erasure

Rejected. The concrete problem is only the meaning of default color for filled geometry.
`FillRole` records that semantic distinction without designing a theme system prematurely.

### Consequences

- M3 now has a real SDL3/SDL_ttf command-and-metric implementation, not only mocks/value objects;
- headless CI can validate actual rendered pixels, clipping, UTF-8 rendering and text metrics;
- generic Core/Rendered APIs remain SDL-free;
- normal builds remain free of SDL dependencies unless the experimental adapter is explicitly
  enabled;
- `FillRectCommand` now preserves foreground/background intent for default-color fills;
- SDL3 package/export policy is still intentionally not stable;
- the next architectural unknown is narrowed to desktop-window/frame/event/input lifecycle rather
  than basic command execution or font metrics.

---

## Deutsch

### Kontext

ADR 0027 definierte die SDL-unabhängige `rendered::DisplayList`, ADR 0028 ergänzte den für
`TextField` nötigen Rendered-Textmetrikvertrag und ADR 0029 führte unterhalb der DisplayList die
`RenderDevice`-Replay-Grenze ein.

M3 benötigt jetzt den Nachweis, dass diese Abstraktionen auch mit einem realen Rendering-/Text-Stack
funktionieren. Ein direkter Sprung vom headless Command-Modell zu einer vollständigen
Desktop-Anwendung würde mehrere voneinander unabhängige Unbekannte gleichzeitig einführen:

- Ausführung der Zeichenbefehle;
- Farbauflösung;
- Clipping;
- UTF-8-Fontrasterung;
- geformte Textmetriken und Caret-Grenzen;
- Fenster-Lifecycle;
- Event-Loop;
- Resize-/High-DPI-Policy;
- Input-Übersetzung;
- Frame-/Present-Lifecycle.

Dann wären Fehler schwerer einzugrenzen und der Window-Lifecycle könnte versehentlich Schnittstellen
prägen, die eigentlich zur darunterliegenden Zeichenschicht gehören.

Der zweite konkrete Renderpfad macht außerdem eine bisher verborgene Mehrdeutigkeit sichtbar:
`Color::default_color` bei einem gefüllten Rechteck kann Default-**Hintergrund** (Surface-/Widget-
Löschen) oder Default-**Vordergrund** (beispielsweise TextField-Caret) bedeuten. Ein RGB-Renderer kann
diese semantische Rolle nicht allein aus dem Farbwert ableiten.

### Entscheidung

M3 führt zunächst einen **experimentellen headless SDL3-Software-Rendering-Adapter** unterhalb der
bestehenden `RenderDevice`-Grenze ein.

Der erste Adapter ist bewusst nur im Build-Tree verfügbar:

- Target: `SASD::UI::Rendered::SDL3`;
- Implementierung: `rendered::sdl3::Sdl3SoftwareDevice`;
- SDL3 und SDL_ttf bleiben optionale Abhängigkeiten;
- das Target wird noch nicht installiert/exportiert;
- normale Core-/Terminal-/Rendered-Builds bleiben SDL-frei;
- kein SDL-Typ wird in generische öffentliche Core-/Rendered-Header getragen.

`Sdl3SoftwareDevice` implementiert gleichzeitig:

- `RenderDevice` für die reale Ausführung von `FillRectCommand`, `StrokeRectCommand` und
  `DrawTextCommand`;
- `RenderedMeasurementContext`, damit Layout, TextField-Viewport und Caret dieselbe reale Font-Policy
  wie die Zeichnung verwenden.

Das Device rendert über `SDL_CreateSoftwareRenderer` in eine `SDL_Surface`. Damit wird ein echter
SDL-Renderer getestet, ohne bereits X11, Wayland, Win32 oder ein AppKit-Fenster zu benötigen. Eine
kleine diagnostische `pixelAt()`-API existiert nur auf diesem experimentellen Adapter, damit Tests
das Ergebnis untersuchen können. Sie ist ausdrücklich keine allgemeine Bitmap-/Screenshot-API des
Toolkits.

Für Text wird SDL_ttf verwendet. Der Adapter:

- misst UTF-8 mit dem aktiven Font;
- rastert UTF-8-Text;
- beachtet Text-Clipping pro Command;
- behandelt explizite Zeilenumbrüche auf demselben logischen Zeilenraster wie Measurement;
- fragt SDL_ttf nach Positionen im geformten **vollständigen Textlauf**, wenn TextField-Scalar-Grenzen
  auf Caret-Positionen abgebildet werden;
- liefert `std::nullopt`, wenn eine angeforderte Unicode-Scalar-Grenze innerhalb eines geformten
  Clusters liegt und sich deshalb mit dem heutigen monotonen Links-nach-rechts-Caret-Vertrag nicht
  korrekt ausdrücken lässt.

Damit bleibt ADR 0028 erhalten: Caret-Positionen werden nicht durch unabhängiges Vermessen von
UTF-8-Präfixen ermittelt, deren Shaping-Kontext vom vollständigen Textlauf abweichen könnte.

### Default-Fill-Rolle

`FillRectCommand` erhält die kleine semantische `FillRole`:

- `background` (Default);
- `foreground`.

Die Rolle beeinflusst nur die Auflösung von `Color::default_color`. Explizite benannte Farben bleiben
explizite Palette-Werte.

Bestehende Aufrufe wie `fillRect(bounds)` bedeuten weiterhin Default-Hintergrund. Das gerenderte
TextField-Caret zeichnet `FillRole::foreground` auf. Dadurch wird ein default-farbiges Caret als
Vordergrund dargestellt, statt im Default-Hintergrund zu verschwinden.

Ein vollständiges semantisches Hintergrundfarben-/Theme-Modell bleibt bewusst später. `FillRole`
löst die durch einen zweiten konkreten Renderpfad nachgewiesene Mehrdeutigkeit, ohne `TextStyle`
vorzeitig zu einem Theme-System auszubauen.

### Style-Policy im ersten Adapter

ADR 0025 legt fest, dass die heutigen `TextStyle`-Attribute nur die Presentation und nicht die
intrinsische Messung verändern. Der erste SDL3-Adapter hält diese Grenze ein:

- benannte Farben werden auf eine anfängliche adapterlokale RGB-Palette abgebildet;
- `dim` reduziert die realisierte Vordergrundintensität;
- `inverse` füllt den gemessenen Textlauf mit der Vordergrundfarbe als Hintergrund und verwendet
  den Default-Hintergrund als Glyphenfarbe;
- `bold` ist ein zusätzlicher um eine logische Einheit versetzter Zeichenpass und kein anderer
  Metrik-/Fontvertrag;
- `underline` ist ein Presentation-Overlay.

Das sind bewusst anfängliche Darstellungsregeln und kein Desktop-Theme-Vertrag.

### Dependency- und CI-Policy

Der Adapter kann systemseitig bereitgestellte CMake-Packages nutzen oder bei
`SASD_UI_FETCH_SDL3=ON` gepinnte Upstream-Releases beziehen.

Die dedizierte Adapter-CI pinnt aktuell:

- SDL 3.4.16;
- SDL_ttf 3.2.2.

SDL_ttf verwendet für den aktivierten ersten Pfad FreeType und HarfBuzz. PlutoSVG-Unterstützung bleibt
bewusst abgeschaltet, weil Color-SVG-/Emoji-Rendering für die heutigen DisplayList-/TextField-
Verträge noch nicht benötigt wird.

Es wird kein Font-Binary in das SASD-UI-Toolkit-Repository kopiert. Adaptertests erhalten einen
Fontpfad aus der Build-Umgebung. Die Fontlizenz bleibt damit getrennt vom Source-Paket des Toolkits.

Die normale plattformübergreifende Compiler-/Release-Matrix aktiviert den SDL3-Adapter nicht. Ein
dedizierter Linux-Job baut die gepinnten Abhängigkeiten und testet den Adapter headless. So bleiben
Core-Builds schnell und abhängigkeitsarm, während der konkrete Adapter kontinuierlich kompiliert und
getestet wird.

### Threading und Lifetime

SDL-Rendereroperationen werden als Main-Thread-Operationen behandelt. Das experimentelle Device:

- muss auf dem Prozess-Main-Thread erzeugt werden;
- merkt sich den erzeugenden Thread;
- lehnt spätere Aufrufe von einem anderen Thread ab;
- besitzt SDL-Surface, Software-Renderer und Font per RAII;
- paart erfolgreiche SDL_ttf-Initialisierung mit Shutdown.

Die Klasse ist nicht kopierbar und nicht verschiebbar, damit Ressourcen-/Thread-Affinität nicht
versehentlich übertragen wird.

### Warum noch kein Desktopfenster

Dieser Schnitt beweist absichtlich zuerst:

```text
RenderedPresentationSink
        |
    DisplayList
        |
DisplayListExecutor
        |
   RenderDevice
        |
Sdl3SoftwareDevice
        |
 SDL_Surface + SDL_ttf
```

bevor zusätzlich eingeführt werden:

```text
SDL_Window
SDL Event Pump
Resize / DPI
Input-Übersetzung
Present-/Swap-Lifecycle
```

Der nächste M3-Schritt ist ein fenstergebundener SDL3-Host/Device, der die bereits getesteten Command-
und Metrikverträge wiederverwendet. Der Window-/Event-Lifecycle soll aus dieser realen Implementierung
abgeleitet und nicht spekulativ in `RenderDevice` eingebaut werden.

### Betrachtete Alternativen

#### Sofort das vollständige SDL3-Window-Backend bauen

Für diesen Schritt verworfen. Rendering, Text, Window-Lifetime, Events, Input, Resize und DPI würden
gleichzeitig zu einer einzigen Debugging-Fläche.

#### SDL-Debugtext statt SDL_ttf verwenden

Verworfen. Der M3-TextField-Vertrag benötigt echte UTF-8-Fontmetriken und geformte Caret-Grenzen; eine
Debugtext-Funktion ist dafür kein ausreichender Typografie-Vertrag.

#### SDL-Typen in die generische RenderDevice-API aufnehmen

Verworfen. Das würde die in ADR 0027 und ADR 0029 festgelegte Abhängigkeitsrichtung umkehren.

#### SDL3-Adapter sofort installieren/exportieren

Verschoben. Dependency Discovery, Redistribution/Runtime Loading, Font-Policy und
Desktop-Window-Lifecycle sind noch nicht reif genug für ein installiertes Kompatibilitätsversprechen.

#### Einen Font für Tests im Repository vendoren

Verworfen. Der Adapter benötigt lediglich eine bereitgestellte Fontressource; ein mitgelieferter Font
würde eine unnötige zusätzliche Lizenz-/Distributionszusage im Toolkit-Repository schaffen.

#### Vollständiges Background-/Theme-System für Caret versus Erasure einführen

Verworfen. Das konkrete Problem betrifft nur die Bedeutung von Default-Farbe bei gefüllter Geometrie.
`FillRole` hält diese Semantik fest, ohne vorzeitig ein Theme-System zu entwerfen.

### Konsequenzen

- M3 besitzt nun eine reale SDL3-/SDL_ttf-Command- und Metrikimplementierung statt nur Mocks und
  Value-Objekten;
- headless CI prüft echte Pixel, Clipping, UTF-8-Rendering und Textmetriken;
- generische Core-/Rendered-APIs bleiben SDL-frei;
- normale Builds bleiben ohne SDL-Abhängigkeit, solange der experimentelle Adapter nicht explizit
  aktiviert wird;
- `FillRectCommand` bewahrt jetzt Vordergrund-/Hintergrundabsicht für Default-Fill-Farbe;
- SDL3-Package-/Export-Policy bleibt bewusst noch instabil;
- die nächste offene Architekturfrage ist auf Desktop-Window-/Frame-/Event-/Input-Lifecycle
  eingegrenzt, statt weiterhin grundlegende Command-Ausführung oder Fontmetriken zu betreffen.
