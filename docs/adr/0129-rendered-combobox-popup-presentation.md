# ADR 0129 – Rendered ComboBox popup presentation snapshot and DisplayList overlay

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

Core ComboBox semantics, generic anchored popup geometry and the Terminal popup path are now mature
enough to establish the equivalent backend-neutral Rendered popup presentation. Rendered already has a
collapsed ComboBox and SDL3 proves the generic DisplayList on real pixels, but an open ComboBox still has
no graphical item-row overlay.

The Rendered popup must not move geometry into Core or embed SDL/native types. It also needs stronger
metric ownership than Terminal: proportional fonts, line height and theme border geometry determine
row dimensions. Re-measuring through a borrowed font service during later command replay would weaken
the snapshot boundary and could disagree with geometry if the environment changed.

### Decision

Add `RenderedComboBoxPopupPresentationSnapshot` plus build/render/compose functions in the generic
Rendered layer.

#### Owned item/geometry snapshot

Each popup item owns:

- copied UTF-8 text;
- the logical `Size` measured for that exact text.

The popup snapshot additionally owns:

- final outer bounds;
- exact inner fixed-row content bounds;
- explicit above/below placement side;
- optional preview index;
- copied base `TextStyle`;
- fixed row height;
- theme-derived padding;
- normalized border thickness.

No ComboBox, Widget, measurement-context, SDL object, font handle or string view is retained.

#### Measurement and placement

Snapshot construction samples the current open/focused/visible/enabled ComboBox and measures every
copied item through the supplied `RenderedMeasurementContext`.

One fixed row height is used for the whole popup. Its content height is the maximum of positive font line
height and every measured item height, with symmetric theme-derived padding. Natural popup width uses the
widest measured item plus horizontal padding and border, then expands to at least the collapsed anchor
width.

The complete row stack plus outer border is passed to ADR 0122 `placeAnchoredPopup()`. Below is
preferred, above is fallback, and no implicit clipping/scrolling occurs. The popup content rectangle is
the exact border inset and its height must equal `row_count * row_height`.

A zero-item open ComboBox returns a valid no-overlay snapshot with empty bounds. No placeholder item is
invented.

Malformed negative metric extents, arithmetic overflow, stale preview identity or placement failure
returns `std::nullopt` before drawing.

#### DisplayList rendering

`renderComboBoxPopupPresentation()` preflights the complete snapshot before appending the first
command:

- preview identity;
- exact outer/content border relation;
- exact fixed-row stack height;
- item text fit within padded rows;
- every row through ADR 0122 `fixedPopupRowBounds()`;
- all text origins through widened/narrowed coordinate arithmetic.

After successful preflight the popup emits:

1. opaque outer background fill using host-provided background color;
2. optional outer border using ComboBox foreground color;
3. one row text command per item;
4. for the preview row, a one-unit full-row outline and toggled `TextStyle::inverse`.

The current style vocabulary has no semantic background brush. The row outline exposes full-row preview
geometry without hard-coding a palette choice, while toggled inverse text remains distinguishable even
if the application base style was already inverse.

#### Value composition

`composeComboBoxPopupDisplayList()` treats the base DisplayList as immutable input. Closed state
returns a value-preserving copy. Open state builds/validates the snapshot first, copies base and appends
the overlay to the copy. Failure returns `std::nullopt` without mutating base.

The copy is correctness-first. A later retained command arena/backing-store optimization may remove it
without changing the semantic/presentation contract.

### Consequences

Positive:

- Rendered now has real backend-neutral open ComboBox row presentation;
- proportional-font/theme metrics are captured once into owned presentation geometry;
- Terminal and Rendered share ADR 0122 placement/fixed-row semantics without sharing backend units;
- popup overlay commands remain SDL/native independent;
- preview and committed selection remain distinct;
- malformed/stale snapshots fail before partial DisplayList mutation.

Trade-offs:

- no Rendered demo/SDL host integration yet;
- no Rendered popup pointer hit testing yet;
- no scrolling/max-visible-row policy yet;
- preview row uses existing outline/inverse vocabulary rather than a richer themed selection brush.

### Deliberately deferred

- SDL/rendered form demo integration;
- Rendered popup row hit testing and pointer commit/dismissal;
- scrolling and viewport windows for long lists;
- richer popup theme/background/shadow primitives;
- native ComboBox peers.

---

## Deutsch

### Kontext

Core-ComboBox-Semantik, generische Anchored-Popup-Geometrie und der Terminal-Popup-Pfad sind inzwischen
stabil genug, um die entsprechende backend-neutrale Rendered-Popup-Presentation aufzubauen. Rendered
besitzt bereits die collapsed ComboBox und SDL3 beweist die generische DisplayList auf echten Pixeln;
einer geöffneten ComboBox fehlt aber noch das grafische Item-Row-Overlay.

Die Rendered-Popup-Geometrie darf weder in Core wandern noch SDL-/Native-Typen aufnehmen. Sie benötigt
außerdem stärkere Metrik-Ownership als Terminal: proportionale Fonts, Line Height und Theme-Border
bestimmen die Row-Abmessungen. Spätes erneutes Messen über einen geliehenen Font-Service würde die
Snapshot-Grenze schwächen und könnte bei veränderter Umgebung nicht mehr zur entschiedenen Geometrie
passen.

### Entscheidung

Die generische Rendered-Schicht erhält
`RenderedComboBoxPopupPresentationSnapshot` sowie Build-/Render-/Compose-Funktionen.

#### Owned Item-/Geometrie-Snapshot

Jedes Popup-Item besitzt:

- kopierten UTF-8-Text;
- die für genau diesen Text gemessene logische `Size`.

Der Popup-Snapshot besitzt zusätzlich:

- finale äußere Bounds;
- exakte innere Fixed-Row-Content-Bounds;
- explizite Above-/Below-Placement-Seite;
- optionalen Preview-Index;
- kopierten Basis-`TextStyle`;
- feste Row-Höhe;
- Theme-basiertes Padding;
- normalisierte Border-Dicke.

Es werden weder ComboBox/Widget noch Measurement-Context, SDL-Objekt, Font-Handle oder String-View
festgehalten.

#### Measurement und Placement

Der Snapshot-Builder liest die aktuell offene/fokussierte/sichtbare/aktivierte ComboBox und misst jedes
kopierte Item über den gelieferten `RenderedMeasurementContext`.

Das gesamte Popup verwendet eine feste Row-Höhe. Deren Content-Höhe ist das Maximum aus positiver Font-
Line-Height und allen gemessenen Item-Höhen, ergänzt um symmetrisches Theme-basiertes Padding. Die
natürliche Popup-Breite verwendet das breiteste Item plus horizontales Padding und Border und wird
anschließend mindestens auf die collapsed Anchor-Breite erweitert.

Der vollständige Row-Stack plus Outer-Border wird an `placeAnchoredPopup()` aus ADR 0122 übergeben.
Below wird bevorzugt, Above ist der Fallback; implizites Clipping/Scrolling findet nicht statt. Das
Popup-Content-Rechteck ist das exakte Border-Inset und seine Höhe muss
`row_count * row_height` entsprechen.

Eine offene ComboBox ohne Items liefert einen gültigen No-Overlay-Snapshot mit leeren Bounds; es wird
kein künstliches Placeholder-Item erzeugt.

Negative fehlerhafte Metric-Extents, Arithmetic-Overflow, stale Preview-Identität oder Placement-Fehler
liefern vor dem Zeichnen `std::nullopt`.

#### DisplayList-Rendering

`renderComboBoxPopupPresentation()` prüft den vollständigen Snapshot vor dem ersten angehängten
Command:

- Preview-Identität;
- exakte Outer-/Content-Border-Beziehung;
- exakte Fixed-Row-Stack-Höhe;
- Text-Fit in die gepaddeten Rows;
- jede Row über `fixedPopupRowBounds()` aus ADR 0122;
- alle Text-Origins über verbreiterte/narrowed Koordinatenarithmetik.

Nach erfolgreichem Preflight entstehen:

1. opaker Outer-Background-Fill mit der vom Host gelieferten Hintergrundfarbe;
2. optionaler Outer-Border mit ComboBox-Foreground;
3. ein Row-Text-Command pro Item;
4. für die Preview-Row ein einheitenbreiter Full-Row-Outline plus getoggeltes
   `TextStyle::inverse`.

Das aktuelle Style-Vokabular besitzt keinen semantischen Background-Brush. Der Row-Outline macht
Full-Row-Preview-Geometrie sichtbar, ohne eine Palette hart zu codieren; getoggelter Inverse-Text bleibt
auch dann unterscheidbar, wenn der Application-Basisstyle bereits invers ist.

#### Value Composition

`composeComboBoxPopupDisplayList()` behandelt die Base-DisplayList als unveränderliche Eingabe.
Geschlossen liefert eine wertgleiche Kopie. Offen wird zuerst der Snapshot vollständig aufgebaut/
validiert, danach Base kopiert und das Overlay an die Kopie angehängt. Fehler liefern
`std::nullopt`, ohne Base zu verändern.

Die Kopie ist correctness-first. Eine spätere Retained-Command-Arena-/Backing-Store-Optimierung kann sie
entfernen, ohne den semantischen/Presentation-Vertrag zu ändern.

### Konsequenzen

Positiv:

- Rendered besitzt jetzt echte backend-neutrale Open-ComboBox-Rows;
- Proportional-Font-/Theme-Metriken werden einmalig in owned Presentation-Geometrie eingefroren;
- Terminal und Rendered teilen ADR-0122-Placement-/Fixed-Row-Semantik ohne gemeinsame Backend-Einheit;
- Popup-Overlay-Commands bleiben SDL-/Native-unabhängig;
- Preview und committed Selection bleiben getrennt;
- malformed/stale Snapshots scheitern vor partieller DisplayList-Mutation.

Abwägungen:

- Rendered-Demo-/SDL-Host-Integration fehlt noch;
- Rendered-Popup-Pointer-Hit-Testing fehlt noch;
- Scrolling/Max-Visible-Row-Policy fehlt noch;
- Preview-Row verwendet vorhandenes Outline-/Inverse-Vokabular statt eines reicheren Theme-Selection-
  Brushes.

### Bewusst vertagt

- SDL-/Rendered-Form-Demo-Integration;
- Rendered-Popup-Row-Hit-Testing und Pointer-Commit/Dismissal;
- Scrolling und Viewport-Windows für lange Listen;
- reichere Popup-Theme-/Background-/Shadow-Primitiven;
- native ComboBox-Peers.
