# ADR 0118 – Terminal collapsed ComboBox presentation

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0117 introduced the backend-neutral, non-editable `ComboBox` selection foundation. Core owns
items, committed selection, keyboard navigation and a semantic `measureComboBox()` hook, but it
intentionally does not define terminal chrome, drop-down geometry or popup state.

The next useful vertical slice is the closed control in the first-class Terminal backend. This needs
to make an empty/unselected selector recognizable, keep measurement stable across selection changes,
and preserve the terminal backend's existing fail-closed Unicode behavior without inventing the
open drop-down interaction early.

### Decision

The Terminal backend renders the collapsed ComboBox with a small ASCII presentation:

```text
[ choice v ]   normal
> choice v <   focused
( choice v )   disabled
```

The letter `v` is an ASCII drop indicator. It is presentation-only and does not mean that an open
popup state already exists.

The collapsed geometry has six fixed cells in addition to the selected text width:

1. left delimiter;
2. leading interior space;
3. one separating space before the indicator;
4. the `v` indicator;
5. one separating space before the right delimiter;
6. right delimiter.

`TerminalMeasurementContext::measureComboBox()` therefore measures UTF-8 text with the existing
`TextMetrics` policy and adds six columns with saturation. Core already measures the empty baseline
and every owned item through this hook, so the resulting intrinsic width is based on the widest item
and remains unchanged when committed selection moves between items.

The renderer uses the complete arranged width rather than concatenating chrome immediately after the
selected text. Selected text is left-aligned, while the indicator and right delimiter are anchored to
the right edge. This gives short and long selections one stable control outline and also behaves
sensibly when a layout expands the ComboBox beyond its intrinsic width.

No selection is rendered as blank content inside the same chrome. No placeholder string is invented
in Core or Terminal presentation.

Focus and disabled state are presentation overlays consistent with existing terminal controls:

- focus uses `>` / `<` delimiters plus `TextStyle::inverse`;
- disabled uses `(` / `)` delimiters plus `TextStyle::dim`;
- the user-supplied `ComboBox::textStyle()` remains the base and is never mutated by the backend.

The complete arranged rectangle is repainted with styled blank cells before selected text and chrome
are written. This is correctness-first: changing from a long item to a short item cannot leave stale
cells behind, and focus styling remains contiguous across unused expanded space.

Before any old cells are modified, selected text is measured and checked for the current terminal
Cell model. Multiline or otherwise non-simple-cell-renderable selected text yields
`PresentationUpdateResult::deferred`. The last complete frame remains intact and the widget stays
presentation-pending. Wide scalars are emitted only when both cells fit in the text viewport.

### Deliberately deferred

This ADR does **not** introduce:

- open/closed ComboBox state;
- popup rows or popup placement;
- pointer opening or item hit testing;
- preview selection versus committed selection;
- Escape rollback;
- type-ahead search;
- Rendered Desktop or native-peer presentation.

Those remain separate contracts so popup geometry and interaction do not leak into Core through a
presentation-only slice.

### Consequences

Positive:

- Terminal now consumes the Core ComboBox contract through the same measurement/presentation split as
  other controls.
- Empty, selected, focused and disabled collapsed states have deterministic cell geometry.
- Selection changes do not cause layout jitter and cannot leave stale text.
- Unsupported terminal text remains fail-closed and transactional.

Trade-offs:

- `v` is intentionally plain ASCII rather than a Unicode arrow so the initial chrome is universally
  one cell under the current terminal width model.
- The control looks expandable before expansion is implemented; the indicator is a visual affordance
  for the intended ComboBox semantics, not a promise that this slice opens a popup.

---

## Deutsch

### Kontext

ADR 0117 hat die backend-neutrale Grundlage der nicht editierbaren `ComboBox` eingeführt. Der Core
besitzt Einträge, committed Selection, Tastaturnavigation und den semantischen
`measureComboBox()`-Hook, definiert aber bewusst weder Terminal-Chrome noch Drop-down-Geometrie oder
Popup-Zustand.

Der nächste sinnvolle vertikale Slice ist das geschlossene Control im vollwertigen Terminal-Backend.
Dabei müssen auch eine leere/nicht ausgewählte ComboBox erkennbar sein, die Messung bei
Selection-Wechseln stabil bleiben und das vorhandene fail-closed Unicode-Verhalten des Terminalpfads
erhalten werden, ohne den offenen Drop-down bereits vorwegzunehmen.

### Entscheidung

Das Terminal-Backend stellt die geschlossene ComboBox mit kleinem ASCII-Chrome dar:

```text
[ choice v ]   normal
> choice v <   fokussiert
( choice v )   deaktiviert
```

Das `v` ist ein reiner ASCII-Drop-Indikator der Presentation. Es bedeutet noch nicht, dass bereits
ein geöffneter Popup-Zustand existiert.

Zur Breite des ausgewählten Textes kommen sechs feste Zellen hinzu:

1. linker Begrenzer;
2. führendes Leerzeichen innen;
3. ein Trenn-Leerzeichen vor dem Indikator;
4. der `v`-Indikator;
5. ein Trenn-Leerzeichen vor dem rechten Begrenzer;
6. rechter Begrenzer.

`TerminalMeasurementContext::measureComboBox()` misst den UTF-8-Text deshalb mit der bestehenden
`TextMetrics`-Policy und addiert sechs Spalten mit Sättigung. Der Core misst bereits die leere
Chrome-Basis und alle eigenen Items über diesen Hook. Damit richtet sich die intrinsische Breite nach
dem breitesten Item und bleibt bei reinen Selection-Wechseln unverändert.

Der Renderer nutzt die vollständig arrangierte Breite, statt das Chrome direkt hinter den aktuell
ausgewählten Text zu hängen. Der Selection-Text ist linksbündig; Indikator und rechter Begrenzer
bleiben an der rechten Kante. Kurze und lange Auswahlwerte behalten dadurch dieselbe Kontur, und eine
vom Layout verbreiterte ComboBox verhält sich ebenfalls stabil.

Ohne Auswahl bleibt der Inhaltsbereich im gleichen Chrome leer. Es wird weder im Core noch im
Terminal-Backend ein künstlicher Platzhaltertext eingeführt.

Fokus und Disabled-Zustand folgen den vorhandenen Terminal-Konventionen:

- Fokus verwendet `>` / `<` plus `TextStyle::inverse`;
- Disabled verwendet `(` / `)` plus `TextStyle::dim`;
- der vom Anwender gesetzte `ComboBox::textStyle()` bleibt die Basis und wird vom Backend nicht
  verändert.

Vor Text und Chrome wird das komplette arrangierte Rechteck mit gestylten Leerzellen neu gezeichnet.
Das ist bewusst correctness-first: Beim Wechsel von einem langen auf ein kurzes Item bleiben keine
alten Zeichen stehen, und Fokus-Styling bleibt auch über zusätzlichen Layout-Platz zusammenhängend.

Bevor bestehende Zellen verändert werden, wird der ausgewählte Text mit dem aktuellen Terminal-
Cell-Modell geprüft. Mehrzeiliger oder sonst nicht als einfache Zellen darstellbarer Text liefert
`PresentationUpdateResult::deferred`. Der letzte vollständige Frame bleibt erhalten und das Widget
bleibt presentation-pending. Breite Unicode-Scalars werden nur geschrieben, wenn beide Zellen in den
Text-Viewport passen.

### Bewusst vertagt

Dieser ADR führt **nicht** ein:

- Open/Closed-State der ComboBox;
- Popup-Zeilen oder Popup-Platzierung;
- Pointer-Öffnung oder Item-Hit-Testing;
- Preview-Selection gegenüber committed Selection;
- Escape-Rollback;
- Type-ahead-Suche;
- Rendered-Desktop- oder Native-Peer-Presentation.

Diese Themen bleiben getrennte Verträge, damit Popup-Geometrie und Interaktion nicht durch einen
reinen Presentation-Slice in den Core gelangen.

### Konsequenzen

Positiv:

- Terminal nutzt den Core-ComboBox-Vertrag nun über dieselbe Measurement-/Presentation-Trennung wie
  die anderen Controls.
- Leerer, ausgewählter, fokussierter und deaktivierter Zustand besitzen deterministische Zellgeometrie.
- Selection-Wechsel erzeugen weder Layout-Sprünge noch stale Text.
- Nicht unterstützter Terminaltext bleibt fail-closed und transaktional.

Abwägungen:

- `v` bleibt bewusst einfaches ASCII statt eines Unicode-Pfeils, damit das erste Chrome unter dem
  aktuellen Terminal-Breitenmodell garantiert genau eine Zelle belegt.
- Das Control wirkt bereits aufklappbar, bevor Aufklappen implementiert ist. Der Indikator ist die
  visuelle Darstellung der beabsichtigten ComboBox-Semantik, kein Versprechen eines in diesem Slice
  vorhandenen Popups.
