# ADR 0062 – Headless terminal popup rendering / Headless-Terminal-Popup-Rendering

- **Status:** Accepted
- **Date:** 2026-10-02

## English

### Context

ADR 0059 introduced owned menu-presentation snapshots so a backend can complete a presentation transaction without retaining live `MenuModel`, `MenuItem`, or `Command` pointers. ADR 0060 added deterministic shortcut display text, and ADR 0061 established terminal-cell measurement for menu bars and popup snapshots using the same Unicode width policy as the existing Terminal backend.

The next step is to make popup presentation observable in the existing deterministic `ScreenBuffer` before involving ANSI/VT serialization or an operating-system console. Doing that directly in the terminal device/session layer would mix semantic snapshot consumption, geometry, Unicode cell occupancy and transport concerns. Conversely, introducing a Widget for popup menus would incorrectly force transient menu interaction state into the normal visual Component tree.

### Decision

Extend `sasd::ui::terminal::menu_presentation.hpp` with a headless popup renderer:

`renderMenuPopupPresentation(ScreenBuffer&, Point origin, const MenuPopupPresentationSnapshot&, AmbiguousWidthMode)`.

The renderer consumes only the owned snapshot and terminal presentation primitives. It does not retain semantic model pointers, mutate `MenuInteractionController`, emit ANSI/VT sequences, own focus, or introduce a popup Widget.

The complete snapshot is measured before any `ScreenBuffer` mutation. If measurement rejects multiline, zero-width/control semantics, saturated geometry, or another unsupported representation, rendering returns `false` and the previous buffer contents are left untouched. Once preflight succeeds, each logical popup row is cleared/filled across the measured popup width before glyphs are written.

Initial terminal presentation conventions are deliberately simple and portable:

- the selected row uses inverse video across the complete row, including padding and accelerator gaps;
- disabled command rows use dim text;
- separators are ASCII `-` runs between the outer padding cells;
- command shortcuts are right-aligned before the trailing padding cell;
- submenu rows place an ASCII `>` marker in the penultimate cell;
- popup content is clipped to `ScreenBuffer` without range errors;
- a two-cell Unicode glyph is emitted only when both its lead and continuation cells are visible, so clipping can never create half of a wide glyph.

The renderer keeps arithmetic widened to `int64_t` while applying the caller-supplied origin and clipping to the buffer. This avoids signed `Coordinate` overflow for extreme origins before the clipping decision is made.

`measureMenuPopupPresentation()` is no longer declared `noexcept`. Shortcut formatting creates a temporary `std::string`; an allocation failure should propagate normally rather than terminate because of an overly strong `noexcept` promise.

### Consequences

Terminal menu presentation now has a complete deterministic path from semantic state to visible cell data:

`MenuModel / Command -> owned presentation snapshot -> terminal measurement -> ScreenBuffer`.

This path is testable on all CI platforms and establishes row clearing, selection style, separator expansion, accelerator alignment, submenu indication, Unicode occupancy and clipping semantics before any terminal transport code is added.

The first renderer intentionally does not draw borders, shadows, menu-bar chrome, pointer hover, mnemonic underlines, scrolling, viewport placement policy, or ANSI-specific colors. Those are separate presentation decisions and can be added incrementally without changing the semantic menu model.

Some helper logic for UTF-8 cell writing is currently local to terminal menu presentation even though the existing Widget terminal renderer has equivalent private helpers. We deliberately avoid a premature shared utility refactor in this slice. Once a second or third presentation path demonstrates a stable common contract, those helpers can be extracted without changing public API.

## Deutsch

### Kontext

ADR 0059 führte besitzende Menü-Presentation-Snapshots ein, damit ein Backend eine Presentation-Transaktion abschließen kann, ohne Live-Pointer auf `MenuModel`, `MenuItem` oder `Command` zu behalten. ADR 0060 ergänzte deterministische Shortcut-Anzeigetexte, und ADR 0061 definierte die Terminal-Zellvermessung für Menüleisten und Popup-Snapshots auf Basis derselben Unicode-Breitenregeln wie der bestehende Terminal-Backend.

Der nächste Schritt ist eine beobachtbare Popup-Darstellung im bereits deterministischen `ScreenBuffer`, bevor ANSI/VT-Serialisierung oder eine Betriebssystem-Konsole beteiligt werden. Eine direkte Implementierung in der Terminal-Device-/Session-Schicht würde Snapshot-Semantik, Geometrie, Unicode-Zellbelegung und Transport vermischen. Ein eigenes Popup-Widget würde umgekehrt transienten Menü-Interaktionszustand fälschlich in den normalen visuellen Component-Baum zwingen.

### Entscheidung

`sasd::ui::terminal::menu_presentation.hpp` wird um einen headless Popup-Renderer erweitert:

`renderMenuPopupPresentation(ScreenBuffer&, Point origin, const MenuPopupPresentationSnapshot&, AmbiguousWidthMode)`.

Der Renderer konsumiert ausschließlich den besitzenden Snapshot und Terminal-Presentation-Primitiven. Er behält keine semantischen Modellpointer, verändert den `MenuInteractionController` nicht, erzeugt keine ANSI/VT-Sequenzen, besitzt keinen Fokus und führt kein Popup-Widget ein.

Der vollständige Snapshot wird vor jeder Änderung am `ScreenBuffer` vermessen. Verwirft die Vermessung Mehrzeiligkeit, Zero-Width-/Control-Semantik, saturierte Geometrie oder eine andere aktuell nicht darstellbare Repräsentation, liefert der Renderer `false` und lässt den vorherigen Buffer-Inhalt unverändert. Erst nach erfolgreichem Preflight wird jede logische Popup-Zeile über die gemessene Breite gelöscht bzw. gefüllt und anschließend beschrieben.

Die anfänglichen Terminal-Darstellungskonventionen bleiben bewusst einfach und portabel:

- die ausgewählte Zeile verwendet Inverse Video über die gesamte Zeile einschließlich Padding und Accelerator-Abständen;
- deaktivierte Command-Zeilen werden gedimmt;
- Separatoren sind ASCII-`-`-Linien zwischen den äußeren Padding-Zellen;
- Command-Shortcuts werden vor dem rechten Padding rechtsbündig ausgerichtet;
- Submenu-Zeilen erhalten ein ASCII-`>` in der vorletzten Zelle;
- Popup-Inhalt wird sicher am `ScreenBuffer` geclippt;
- ein zweizelliges Unicode-Zeichen wird nur geschrieben, wenn sowohl Lead- als auch Continuation-Zelle sichtbar sind, sodass Clipping niemals ein halbes Wide-Glyph erzeugt.

Die Koordinatenarithmetik bleibt beim Anwenden des Caller-Ursprungs und beim Clipping in `int64_t`. Dadurch kann ein extremer Ursprung nicht bereits vor der Clipping-Entscheidung einen signed Overflow des `Coordinate`-Typs verursachen.

`measureMenuPopupPresentation()` ist nicht länger als `noexcept` deklariert. Die Shortcut-Formatierung erzeugt temporär einen `std::string`; ein Allokationsfehler soll regulär propagiert werden und nicht wegen eines zu starken `noexcept`-Versprechens den Prozess terminieren.

### Konsequenzen

Die Terminal-Menü-Presentation besitzt nun einen vollständigen deterministischen Pfad vom semantischen Zustand zu sichtbaren Zellen:

`MenuModel / Command -> besitzender Presentation-Snapshot -> Terminal-Vermessung -> ScreenBuffer`.

Dieser Pfad ist auf allen CI-Plattformen testbar und legt Clearing, Selection-Stil, Separator-Ausdehnung, Accelerator-Ausrichtung, Submenu-Indikator, Unicode-Zellbelegung und Clipping fest, bevor Terminal-Transportcode hinzukommt.

Der erste Renderer zeichnet bewusst noch keine Rahmen, Schatten, Menüleisten-Chrome, Pointer-Hover-Zustände, Mnemonic-Unterstreichungen, Scrolling, Viewport-Platzierungsregeln oder ANSI-spezifische Farben. Diese Punkte sind eigenständige Presentation-Entscheidungen und können später inkrementell ergänzt werden, ohne das semantische Menümodell zu verändern.

Ein Teil der UTF-8-Zellschreiblogik ist derzeit lokal in der Terminal-Menü-Presentation vorhanden, obwohl der bestehende Widget-Terminal-Renderer ähnliche private Helper besitzt. In diesem Slice vermeiden wir absichtlich einen vorschnellen gemeinsamen Utility-Refactor. Sobald mehrere Presentation-Pfade einen stabilen gemeinsamen Vertrag belegen, können die Helper ohne Änderung der öffentlichen API extrahiert werden.
