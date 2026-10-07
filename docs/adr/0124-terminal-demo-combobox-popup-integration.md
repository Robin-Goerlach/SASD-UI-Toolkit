# ADR 0124 – Terminal demo host integration for ComboBox popup overlays

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0123 made Terminal ComboBox popup rows measurable, placeable and renderable as an owned overlay
frame. The terminal form demo still presented only the ordinary Widget frame plus menu composition, so
the new popup pipeline was not exercised by the real TerminalBackend/session loop.

The demo also reserves terminal row zero for persistent menu chrome. Treating the whole ScreenBuffer as
available popup space would let an above-opening ComboBox occupy that reserved row and then be partially
covered by the menu composition that correctly paints last.

Finally, pointer row interaction is deliberately not implemented yet. Once popup rows become visible in
the real demo, allowing pointer events to continue to ordinary Widget hit testing would create unsafe
click-through: a user could click a visible popup row and activate a different Widget underneath it.

### Decision

Integrate one ComboBox into `terminal_form_demo.cpp` and compose its popup between the captured
application frame and the existing menu overlay.

The host pipeline becomes:

`Widget presentation -> captureFrame() -> ComboBox popup overlay -> menu overlay -> TerminalSession`.

The ComboBox is a normal Core Widget in the VBox and therefore participates in normal measurement,
arrangement, focus traversal and keyboard event routing. Its popup remains a transient presentation
overlay and is not inserted into the Widget tree.

The demo defines one explicit content viewport below menu row zero. Layout and ComboBox popup placement
consume the same rectangle. To support that boundary without duplicating composition logic,
`composeComboBoxPopupFrame()` gains an explicit-popup-viewport overload; the existing full-buffer
signature remains as a convenience wrapper.

The host resolves the ComboBox's arranged bounds to absolute terminal coordinates immediately before
composition. Parent offsets are accumulated in widened arithmetic and unrepresentable origins fail
closed. This remains host/presentation-tree knowledge; `ComboBox` itself still contains no absolute
geometry.

#### Overlay-scope rules

The demo keeps menu mode and ComboBox popup mode mutually exclusive:

- F10 cancels an open ComboBox preview before entering menu mode;
- terminal resize cancels an open ComboBox transaction because its placement belonged to the old
  viewport; committed selection is preserved and reopening reseeds preview from it.

The first popup pointer slice now supports passive row hover: the host rebuilds the same owned popup
snapshot used by presentation, hit-tests its final row rectangles and updates only `previewIndex()` on
motion. Press/release completion and outside dismissal remain deferred; those samples are still consumed
by the popup scope so visible overlay rows cannot click through to unrelated Widgets. Enter, Escape and
F4 remain the completion/cancellation paths.

The demo adds a small "ComboBox demo" control with Portable/Terminal/Rendered items. Its
`SelectionChanged` callback updates the existing status label only after semantic commit, making the
preview-versus-commit contract observable without adding backend-specific application logic.

### Consequences

Positive:

- the real TerminalBackend/session path now displays keyboard-driven ComboBox popup rows;
- popup placement respects persistent menu chrome;
- closing the popup naturally restores the captured application frame by recomposition;
- the demo exercises Core focus/navigation and Terminal overlay presentation together;
- pointer hover preview uses the same final row geometry as painting while click-through remains prevented.

Trade-offs:

- popup press/release input is temporarily consumed rather than committed/dismissed;
- resize cancels an open preview instead of trying to preserve presentation placement;
- absolute anchor resolution is still host-local until a broader reusable visual-geometry seam is
  justified by more consumers.

### Deliberately deferred

- primary-click commit and outside-click dismissal;
- pointer capture policy;
- scrolling/maximum visible rows;
- Rendered popup overlay integration.

---

## Deutsch

### Kontext

ADR 0123 hat Terminal-ComboBox-Popup-Zeilen als owned Overlay-Frame messbar, platzierbar und renderbar
gemacht. Das Terminal-Form-Demo präsentierte bisher jedoch nur den normalen Widget-Frame plus
Menü-Komposition; die neue Popup-Pipeline lief dadurch noch nicht durch den echten
TerminalBackend-/Session-Loop.

Zusätzlich reserviert das Demo Terminalzeile null für persistentes Menü-Chrome. Würde das komplette
`ScreenBuffer` als Popup-Fläche gelten, könnte eine nach oben öffnende ComboBox diese reservierte Zeile
belegen und anschließend teilweise vom korrekt zuletzt gezeichneten Menü überdeckt werden.

Pointer-Interaktion für Popup-Rows ist außerdem bewusst noch nicht implementiert. Sobald die Rows im
echten Demo sichtbar sind, dürfte Pointer-Input deshalb nicht einfach zum normalen Widget-Hit-Test
durchfallen: Ein Klick auf eine sichtbare Popup-Row könnte sonst ein anderes Widget darunter aktivieren.

### Entscheidung

Eine ComboBox wird in `terminal_form_demo.cpp` integriert und ihr Popup zwischen dem erfassten
Application-Frame und dem bestehenden Menü-Overlay komponiert.

Die Host-Pipeline lautet damit:

`Widget-Presentation -> captureFrame() -> ComboBox-Popup-Overlay -> Menü-Overlay -> TerminalSession`.

Die ComboBox selbst ist ein normales Core-Widget im VBox und nimmt damit an Measurement, Arrangement,
Focus-Traversal und Keyboard-Event-Routing teil. Das Popup bleibt transiente Presentation und wird nicht
in den Widget-Tree eingefügt.

Das Demo definiert einen expliziten Content-Viewport unterhalb der Menüzeile null. Layout und
ComboBox-Popup-Placement verwenden dasselbe Rechteck. Damit diese Grenze ohne duplizierte
Composition-Logik nutzbar ist, erhält `composeComboBoxPopupFrame()` einen Overload mit explizitem
Popup-Viewport; die bisherige Full-Buffer-Signatur bleibt als Convenience-Wrapper erhalten.

Der Host löst die arrangierten ComboBox-Bounds direkt vor der Komposition in absolute
Terminalkoordinaten auf. Parent-Offsets werden mit verbreiterter Arithmetik addiert; nicht darstellbare
Ursprünge schlagen fail-closed fehl. Dieses Wissen bleibt Host-/Presentation-Tree-Verantwortung;
`ComboBox` selbst erhält weiterhin keine absolute Geometrie.

#### Overlay-Scope-Regeln

Das Demo hält Menümodus und ComboBox-Popup-Modus gegenseitig exklusiv:

- F10 verwirft eine offene ComboBox-Preview, bevor der Menümodus beginnt;
- ein Terminal-Resize verwirft eine offene ComboBox-Transaktion, weil ihre Platzierung zum alten
  Viewport gehörte; committed Selection bleibt erhalten und erneutes Öffnen initialisiert Preview daraus.

Der erste Popup-Pointer-Slice unterstützt jetzt passives Row-Hover: Der Host baut denselben owned
Popup-Snapshot wie die Presentation auf, führt Hit-Testing gegen dessen finale Row-Rechtecke durch und
ändert bei Motion ausschließlich `previewIndex()`. Press-/Release-Completion und Outside-Dismissal
bleiben vertagt; diese Samples werden weiterhin vom Popup-Scope konsumiert, sodass sichtbare
Overlay-Rows nicht zu fremden Widgets durchklicken. Enter, Escape und F4 bleiben die Completion-/
Cancel-Wege.

Das Demo erhält eine kleine "ComboBox demo" mit den Items Portable/Terminal/Rendered. Ihr
`SelectionChanged`-Callback aktualisiert das vorhandene Status-Label erst nach semantischem Commit und
macht damit den Unterschied zwischen Preview und Commit sichtbar, ohne backend-spezifische
Anwendungslogik einzuführen.

### Konsequenzen

Positiv:

- der echte TerminalBackend-/Session-Pfad zeigt nun keyboard-gesteuerte ComboBox-Popup-Rows;
- Popup-Placement respektiert persistentes Menü-Chrome;
- Schließen stellt den erfassten Application-Frame natürlich durch Neukomposition wieder her;
- das Demo testet Core-Focus/-Navigation und Terminal-Overlay-Presentation gemeinsam;
- Pointer-Hover-Preview verwendet dieselbe finale Row-Geometrie wie das Painting, während Click-through
  weiterhin verhindert wird.

Abwägungen:

- Popup-Press-/Release-Input wird vorläufig konsumiert statt committed/dismissed;
- Resize verwirft eine offene Preview, statt die Presentation-Platzierung zu erhalten;
- absolute Anchor-Auflösung bleibt host-lokal, bis mehrere Verbraucher eine allgemeinere
  Visual-Geometry-Naht rechtfertigen.

### Bewusst vertagt

- Primary-Click-Commit und Outside-Click-Dismissal;
- Pointer-Capture-Policy;
- Scrolling/maximale sichtbare Rows;
- Rendered-Popup-Overlay-Integration.
