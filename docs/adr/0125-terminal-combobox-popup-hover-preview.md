# ADR 0125 – Terminal ComboBox popup row hit testing and hover preview

**Status:** Accepted  
**Date:** 2026-10-07  
**Follow-up:** ADR 0126 adds optional host-owned Primary press/release click completion while preserving
the stateless hover-only overload defined here.

## English

### Context

ADR 0123 established owned Terminal ComboBox popup presentation snapshots and ADR 0124 integrated them
into the real terminal demo. Pointer events were deliberately quarantined while the popup was open so
visible overlay rows could not click through to ordinary Widgets, but the user still had no pointer-
driven preview.

The next interaction step must not duplicate popup geometry. Presentation already owns the final
viewport-fitted popup rectangle and ADR 0122 already defines fixed row rectangles. Pointer code should
consume those exact values rather than re-measuring item text or independently calculating row offsets.

Click completion is a separate semantic problem because it needs press/release identity, cancellation,
outside dismissal and possibly capture. Adding those rules at the same time as basic hover would make a
simple stateless geometry slice unnecessarily stateful.

### Decision

Add two small Terminal presentation/input helpers.

`TerminalComboBoxPopupHitTest::rowIndexAt()` maps a terminal cell to the item row in an already-built
`ComboBoxPopupPresentationSnapshot`. It:

- requires coherent one-row-per-item geometry;
- uses `Rect::contains()` half-open bounds;
- computes the candidate row with widened arithmetic;
- validates the candidate again through ADR 0122 `fixedPopupRowBounds()`;
- returns no hit for empty/malformed snapshots or points outside the popup.

The hit-test helper knows nothing about `ComboBox` semantic state. It consumes presentation values only.

`TerminalComboBoxPopupPointerInteraction::handle()` is the semantic adapter for this first pointer
slice. An engaged result means the popup scope consumed the event. Before trusting any numeric row, it
revalidates that the snapshot still matches the current open/focused/enabled/visible ComboBox, copied
item texts, preview identity and measurable popup shape.

Current behavior is intentionally narrow:

- pointer motion over a row calls `setPreviewIndex(row)`;
- repeated motion over the existing preview is consumed but reports no semantic change;
- motion outside retains the previous preview;
- press/release are consumed but do not commit or cancel;
- no PointerRouter capture is created.

Because `setPreviewIndex()` is visual-only, hover never emits `SelectionChanged`; committed
application selection changes only through the existing Enter commit path.

The terminal demo rebuilds a fresh popup snapshot for an open ComboBox pointer sample using exactly the
same absolute anchor, content viewport and ambiguous-width policy used by popup composition. It then
passes that snapshot to the interaction helper. A stale/inconsistent result is treated as a demo
presentation/input error rather than falling through to Widgets underneath.

### Consequences

Positive:

- painting and pointer hit testing share one final row geometry contract;
- all-motion terminal reporting now gives immediate ComboBox preview feedback;
- hover does not accidentally commit application state;
- stale copied row identity fails closed;
- pointer click-through protection remains intact while click semantics are still absent;
- the interaction helper retains no Widget/frame pointer across events.

Trade-offs:

- moving outside the popup keeps the last preview for now;
- primary press/release have no ComboBox completion behavior yet;
- the demo rebuilds an owned snapshot for each popup pointer event; correctness is preferred over caching
  until interaction semantics stabilize.

### Deliberately deferred

- primary press/release gesture state;
- click-to-commit;
- outside-click cancellation;
- pointer capture policy;
- scrolling and partially visible row hit testing;
- Rendered popup pointer interaction.

---

## Deutsch

### Kontext

ADR 0123 hat owned Terminal-ComboBox-Popup-Snapshots eingeführt, ADR 0124 diese in das echte
Terminal-Demo integriert. Pointer-Events wurden bei offenem Popup zunächst bewusst quarantänisiert,
damit sichtbare Overlay-Rows nicht zu normalen Widgets darunter durchklicken. Eine pointer-gesteuerte
Preview gab es aber noch nicht.

Der nächste Interaktionsschritt darf die Popup-Geometrie nicht duplizieren. Die Presentation besitzt
bereits das finale viewport-angepasste Popup-Rechteck und ADR 0122 definiert bereits Fixed-Row-
Rechtecke. Pointer-Code soll genau diese Werte konsumieren, statt Item-Text erneut zu messen oder
Row-Offsets unabhängig zu berechnen.

Click-Completion ist ein eigenes semantisches Problem, weil dafür Press-/Release-Identität,
Cancellation, Outside-Dismissal und möglicherweise Capture benötigt werden. Diese Regeln gleichzeitig
mit einfachem Hover einzubauen würde einen stateless Geometry-Slice unnötig stateful machen.

### Entscheidung

Zwei kleine Terminal-Presentation-/Input-Helfer werden ergänzt.

`TerminalComboBoxPopupHitTest::rowIndexAt()` bildet eine Terminalzelle auf die Item-Row eines bereits
gebauten `ComboBoxPopupPresentationSnapshot` ab. Die Funktion:

- verlangt kohärente One-Row-per-Item-Geometrie;
- verwendet die half-open Bounds von `Rect::contains()`;
- berechnet die Kandidaten-Row mit verbreiterter Arithmetik;
- validiert den Kandidaten erneut über `fixedPopupRowBounds()` aus ADR 0122;
- liefert bei leeren/malformed Snapshots oder Punkten außerhalb keinen Hit.

Der Hit-Test-Helfer kennt keinen semantischen `ComboBox`-Zustand, sondern nur Presentation-Werte.

`TerminalComboBoxPopupPointerInteraction::handle()` ist der semantische Adapter für diesen ersten
Pointer-Slice. Ein vorhandenes Ergebnis bedeutet, dass der Popup-Scope das Event konsumiert hat. Vor
Verwendung eines numerischen Row-Index wird erneut bewiesen, dass der Snapshot noch zur aktuell offenen,
fokussierten, aktivierten und sichtbaren ComboBox sowie zu Item-Texten, Preview-Identität und messbarer
Popup-Form passt.

Das aktuelle Verhalten bleibt bewusst klein:

- Pointer-Motion über einer Row ruft `setPreviewIndex(row)` auf;
- wiederholte Motion über der bestehenden Preview wird konsumiert, meldet aber keine Zustandsänderung;
- Motion außerhalb behält die letzte Preview;
- Press/Release werden konsumiert, committen/canceln aber noch nicht;
- es wird kein `PointerRouter`-Capture erzeugt.

Da `setPreviewIndex()` ausschließlich visuell ist, sendet Hover niemals `SelectionChanged`;
committed Anwendungsauswahl ändert sich weiterhin nur über den vorhandenen Enter-Commit-Pfad.

Das Terminal-Demo baut für jedes Pointer-Sample einer offenen ComboBox einen frischen Popup-Snapshot
mit exakt demselben absoluten Anchor, Content-Viewport und Ambiguous-Width-Modus wie die Popup-
Komposition. Dieser Snapshot wird an den Interaktionshelfer übergeben. Ein stale/inkonsistentes Ergebnis
wird als Demo-Presentation-/Input-Fehler behandelt und darf nicht zu Widgets darunter durchfallen.

### Konsequenzen

Positiv:

- Painting und Pointer-Hit-Testing teilen einen finalen Row-Geometrievertrag;
- All-Motion-Terminalreporting liefert jetzt unmittelbares ComboBox-Preview-Feedback;
- Hover committed keinen Anwendungszustand versehentlich;
- stale kopierte Row-Identität schlägt fail-closed fehl;
- Click-through-Schutz bleibt bestehen, solange Click-Semantik noch fehlt;
- der Interaktionshelfer hält keine Widget-/Frame-Pointer über Events hinweg.

Abwägungen:

- Motion außerhalb des Popups behält vorerst die letzte Preview;
- Primary Press/Release besitzen noch kein ComboBox-Completion-Verhalten;
- das Demo baut für jedes Popup-Pointer-Event einen owned Snapshot neu auf; Korrektheit hat Vorrang vor
  Caching, bis die Interaktionssemantik stabil ist.

### Bewusst vertagt

- Primary-Press-/Release-Gesture-State;
- Click-to-Commit;
- Outside-Click-Cancellation;
- Pointer-Capture-Policy;
- Scrolling und Hit-Testing teilweise sichtbarer Rows;
- Rendered-Popup-Pointer-Interaktion.
