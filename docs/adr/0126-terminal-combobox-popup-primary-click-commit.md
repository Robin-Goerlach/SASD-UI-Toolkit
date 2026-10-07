# ADR 0126 – Terminal ComboBox popup Primary click commit transaction

**Status:** Accepted  
**Date:** 2026-10-07  
**Follow-up:** ADR 0127 adds Primary outside-press dismissal while preserving the press/release row
commit transaction defined here.

## English

### Context

ADR 0125 added shared popup-row hit testing and passive pointer-motion preview. It deliberately consumed
press/release without completion semantics because a click needs identity across two physical events.
Now that the final painted row geometry and stale-snapshot validation are stable, the Terminal host can
add a small click transaction without moving pointer state into Core `ComboBox`.

A naive "release over any row = commit" rule would be unsafe. The release could belong to a press that
started outside the popup, in another row, before keyboard takeover, before resize, or against an older
item collection. Likewise, storing a `ComboBox*`, item `string_view` or presentation-frame pointer
across events would create avoidable lifetime coupling.

Outside-click dismissal and pointer capture remain separate policy questions. They should not be hidden
inside the first completion contract.

### Decision

Extend `TerminalComboBoxPopupPointerInteraction` with an optional host-owned `GestureState`.

The state stores only:

`std::optional<std::size_t> pressed_row`.

It owns no Widget, ComboBox, item text, frame, callback, terminal device or timestamp.

The existing stateless `handle()` overload is preserved exactly as a hover/modal-consumption contract.
A new overload accepting `GestureState&` adds Primary click completion.

#### Press

A fresh pointer press first clears any older armed identity.

For a Primary press on a valid popup row:

1. the row is hit-tested against the supplied owned snapshot;
2. the row becomes the current preview using `ComboBox::setPreviewIndex()`;
3. the numeric row identity is armed in `GestureState`.

Press does not change committed selection and does not invoke `SelectionChanged`.

Press outside the rows or with another button is consumed and leaves no armed row. The open popup is not
dismissed yet.

#### Motion

Motion keeps ADR 0125 behavior: hovering a row updates preview, including while a press is armed. Motion
never rewrites `pressed_row`. This separation lets visual hover follow the pointer while click
completion continues to prove the physical press origin.

#### Release

A release can commit only when all of the following are true:

- current ComboBox/snapshot state passes the existing semantic stale-snapshot proof;
- the release is Primary;
- a Primary row is armed;
- the release hit-tests to exactly that same row.

The gesture identity is reset before semantic completion.

If motion changed preview after press and release later returns to the originally pressed row, preview is
realigned to that armed row immediately before `commitPreviewSelection()`. The click therefore commits
the row that owns both press and release, not whichever unrelated row happened to be previewed in
between.

`commitPreviewSelection()` remains the sole Core authority for selection publication and drop-down
closing. It establishes final state before callbacks and may synchronously run application code. The
pointer helper performs no ComboBox member access after calling it.

A mismatched/outside release merely retires the gesture. It does not commit, cancel the ComboBox
transaction or dismiss the popup.

#### Host scope invalidation

The terminal demo owns `GestureState` beside its other transient pointer policies. It explicitly resets
the armed row on:

- keyboard/text takeover before dispatch;
- terminal resize;
- top-level pointer-surface leave;
- modal menu pointer takeover;
- observation that the ComboBox popup is no longer open.

This prevents a release from completing across a close/reopen or geometry/scope boundary.

### Consequences

Positive:

- mouse click can now commit a Terminal ComboBox row;
- press/release identity is revalidated without storing semantic object pointers;
- hover during an armed press remains independent from click origin;
- stale item/preview snapshots reset the gesture fail-closed;
- Core `ComboBox` remains backend-neutral and unchanged;
- application callback/lifetime semantics remain centralized in `commitPreviewSelection()`;
- the stateless ADR 0125 hover API remains available.

Trade-offs:

- outside click still does not dismiss the popup;
- no PointerRouter capture is introduced, so host/surface continuity is handled by explicit gesture
  invalidation rather than captured delivery;
- a mismatched release leaves the current hover preview in place.

### Deliberately deferred

- outside-click cancellation/dismissal;
- pointer capture policy;
- opening the collapsed ComboBox by pointer;
- scrolling/maximum visible rows;
- Rendered popup pointer interaction.

---

## Deutsch

### Kontext

ADR 0125 hat gemeinsames Popup-Row-Hit-Testing und passive Pointer-Motion-Preview eingeführt.
Press/Release wurden bewusst ohne Completion-Semantik konsumiert, weil ein Klick Identität über zwei
physische Events benötigt. Da finale Row-Geometrie und Stale-Snapshot-Prüfung jetzt stabil sind, kann
der Terminal-Host eine kleine Click-Transaktion ergänzen, ohne Pointer-Zustand in die Core-`ComboBox`
zu verschieben.

Eine naive Regel "Release über irgendeiner Row = Commit" wäre unsicher. Das Release könnte zu einem
Press gehören, das außerhalb, auf einer anderen Row, vor Keyboard-Takeover, vor Resize oder gegen eine
ältere Item-Sammlung begann. Ebenso würden über Events gespeicherte `ComboBox*`, Item-`string_view`
oder Presentation-Frame-Pointer unnötige Lifetime-Kopplung erzeugen.

Outside-Click-Dismissal und Pointer-Capture bleiben eigene Policy-Fragen und werden nicht in den ersten
Completion-Vertrag hineingemischt.

### Entscheidung

`TerminalComboBoxPopupPointerInteraction` erhält einen optionalen host-owned `GestureState`.

Der Zustand speichert ausschließlich:

`std::optional<std::size_t> pressed_row`.

Er besitzt weder Widget/ComboBox noch Item-Text, Frame, Callback, Terminalgerät oder Timestamp.

Der bestehende stateless `handle()`-Overload bleibt unverändert als Hover-/Modal-Consumption-Vertrag.
Ein neuer Overload mit `GestureState&` ergänzt Primary-Click-Completion.

#### Press

Jedes frische Pointer-Press löscht zuerst eine ältere armed Identity.

Bei Primary-Press auf einer gültigen Popup-Row:

1. Hit-Test gegen den gelieferten owned Snapshot;
2. Row wird über `ComboBox::setPreviewIndex()` aktuelle Preview;
3. numerische Row-Identität wird im `GestureState` armed.

Press verändert keine committed Selection und sendet kein `SelectionChanged`.

Press außerhalb der Rows oder mit anderem Button wird konsumiert und hinterlässt keine armed Row. Das
offene Popup wird dabei noch nicht dismissed.

#### Motion

Motion behält die ADR-0125-Semantik: Hover über einer Row ändert Preview, auch während ein Press armed
ist. Motion überschreibt `pressed_row` niemals. So kann die sichtbare Preview dem Pointer folgen,
während Click-Completion weiterhin den physischen Press-Ursprung beweist.

#### Release

Ein Release darf nur committen, wenn gleichzeitig gilt:

- aktueller ComboBox-/Snapshot-Zustand besteht die vorhandene Stale-Snapshot-Prüfung;
- Release ist Primary;
- eine Primary-Row ist armed;
- Release trifft exakt dieselbe Row.

Die Gesture-Identität wird vor semantischer Completion gelöscht.

Hat Motion nach dem Press eine andere Preview erzeugt und Release kehrt später auf die ursprünglich
gedrückte Row zurück, wird Preview direkt vor `commitPreviewSelection()` wieder auf diese armed Row
ausgerichtet. Der Klick committed damit die Row, die Press **und** Release besitzt, nicht irgendeine
zwischenzeitlich gehoverte Preview.

`commitPreviewSelection()` bleibt die einzige Core-Autorität für Selection-Publikation und Schließen
des Drop-downs. Die Methode stellt den Endzustand vor Callbacks her und darf synchron Anwendungscode
ausführen. Der Pointer-Helfer greift nach ihrem Aufruf nicht mehr auf ComboBox-Member zu.

Mismatch-/Outside-Release löscht lediglich die Gesture. Es committed nicht, cancelt die ComboBox-
Transaktion nicht und dismissed das Popup nicht.

#### Host-Scope-Invalidierung

Das Terminal-Demo besitzt den `GestureState` neben seinen anderen transienten Pointer-Policies. Die
armed Row wird explizit gelöscht bei:

- Keyboard-/Text-Takeover vor Dispatch;
- Terminal-Resize;
- Verlassen der Top-Level-Pointer-Surface;
- modalem Menü-Pointer-Takeover;
- Feststellung, dass das ComboBox-Popup nicht mehr offen ist.

Damit kann kein Release über Close/Reopen- oder Geometrie-/Scope-Grenzen hinweg committen.

### Konsequenzen

Positiv:

- Mausklick kann jetzt eine Terminal-ComboBox-Row committen;
- Press-/Release-Identität wird ohne gespeicherte semantische Objekt-Pointer revalidiert;
- Hover während armed Press bleibt unabhängig vom Click-Ursprung;
- stale Item-/Preview-Snapshots löschen die Gesture fail-closed;
- Core-`ComboBox` bleibt backend-neutral und unverändert;
- Application-Callback-/Lifetime-Semantik bleibt in `commitPreviewSelection()` zentralisiert;
- die stateless Hover-API aus ADR 0125 bleibt verfügbar.

Abwägungen:

- Outside-Click dismissed das Popup weiterhin nicht;
- es wird noch kein `PointerRouter`-Capture eingeführt; Host-/Surface-Kontinuität entsteht vorerst
  durch explizite Gesture-Invalidierung statt captured Delivery;
- Mismatch-Release lässt die aktuelle Hover-Preview bestehen.

### Bewusst vertagt

- Outside-Click-Cancellation/Dismissal;
- Pointer-Capture-Policy;
- Pointer-Öffnen der collapsed ComboBox;
- Scrolling/maximal sichtbare Rows;
- Rendered-Popup-Pointer-Interaktion.
