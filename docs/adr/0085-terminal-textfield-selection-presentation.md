# ADR 0085 – Terminal TextField selection presentation by inverse-style toggling

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

`TextField` now has a backend-neutral directed Unicode-scalar selection model and supports Shift+navigation plus Copy/Cut/Paste semantics. Until now, however, the terminal backend rendered selected and unselected text identically. That made a keyboard-created selection semantically real but visually invisible.

The terminal backend already uses `TextStyle::inverse` as a focus overlay for the complete `TextField`. Simply forcing `inverse = true` for selection would therefore fail for focused fields because the entire field is already inverse. Adding background colors or a new public selection-style API at this point would broaden the Core styling contract before rendered/native backends have established a common abstraction.

### Decision

The terminal backend presents the current `TextField` selection as a local presentation overlay by toggling the already resolved field style's `inverse` bit for selected Unicode scalars.

This means:

- in an unfocused normal field, selected scalars become inverse;
- in a focused field whose base presentation is already inverse, selected scalars become non-inverse and therefore remain visibly distinct;
- all other style attributes such as foreground color, bold, dim and underline are preserved;
- disabled state remains dim and can still display a contrasting selection;
- Core receives no terminal-specific color, cell or selection-presentation API.

Selection boundaries remain Unicode-scalar indices from `TextField`. The terminal presentation code applies the overlay per preflighted scalar after deciding the horizontal viewport. It does not convert the semantic selection into screen-column state.

Wide terminal glyphs are styled atomically: `writeScalar()` receives one selected style and applies it to both the wide lead and continuation cells. A selected wide scalar is therefore never rendered with mismatched cell styling.

### Consequences

Shift-created or programmatically established selections are now visible in the terminal backend without changing `TextField` semantics or measurement.

Horizontal scrolling does not alter selection meaning. Only the visible subset of selected scalars is painted, while the semantic range remains unchanged in Core.

Selection remains a presentation-only state change and therefore continues to require visual invalidation but not measurement invalidation.

The inverse-toggle rule is intentionally a terminal presentation policy, not a universal theme contract. Rendered desktop selection painting can later use rectangles/background fills appropriate to that backend without being forced into terminal cell semantics.

Pointer-drag selection and terminal mouse hit-testing remain separate slices.

### Alternatives considered

**Force `inverse = true` for selected text.** Rejected because focused `TextField` presentation is already inverse, making selection invisible.

**Introduce a public selection foreground/background style in Core now.** Deferred because only the terminal backend currently needs this presentation rule. A broader styling API should be validated with rendered/native backends before becoming public contract.

**Store selected screen-column ranges in `TextField`.** Rejected because selection is semantic Unicode-scalar state. Cell geometry and horizontal clipping belong to the terminal presentation backend.

---

## Deutsch

### Kontext

`TextField` besitzt inzwischen ein backend-neutrales gerichtetes Auswahlmodell auf Basis von Unicode-Scalar-Indizes und unterstützt Shift-Navigation sowie Copy/Cut/Paste-Semantik. Bislang stellte das Terminal-Backend ausgewählten und nicht ausgewählten Text jedoch identisch dar. Eine per Tastatur erzeugte Auswahl war damit semantisch vorhanden, aber optisch unsichtbar.

Das Terminal-Backend verwendet `TextStyle::inverse` bereits als Fokus-Overlay für das komplette `TextField`. Würde eine Auswahl lediglich `inverse = true` erzwingen, wäre sie in fokussierten Feldern nicht sichtbar, weil dort bereits das gesamte Feld invers dargestellt wird. Hintergrundfarben oder eine neue öffentliche Selection-Style-API würden dagegen den Core-Styling-Vertrag zu früh erweitern, bevor Rendered- und spätere native Backends eine gemeinsame Abstraktion bestätigt haben.

### Entscheidung

Das Terminal-Backend stellt die aktuelle `TextField`-Auswahl als lokales Presentation-Overlay dar, indem es für ausgewählte Unicode-Scalars das `inverse`-Bit des bereits aufgelösten Feld-Stils umschaltet.

Das bedeutet:

- in einem nicht fokussierten normalen Feld werden ausgewählte Scalars invers;
- in einem fokussierten Feld, dessen Basisdarstellung bereits invers ist, werden ausgewählte Scalars nicht-invers und bleiben dadurch klar unterscheidbar;
- alle übrigen Stilattribute wie Vordergrundfarbe, Bold, Dim und Underline bleiben erhalten;
- ein Disabled-Zustand bleibt gedimmt und kann trotzdem eine kontrastierende Auswahl anzeigen;
- der Core erhält keine terminal-spezifische Farb-, Cell- oder Selection-Presentation-API.

Die Auswahlgrenzen bleiben Unicode-Scalar-Indizes aus `TextField`. Die Terminaldarstellung wendet das Overlay pro vorvalidiertem Scalar an, nachdem der horizontale Viewport bestimmt wurde. Die semantische Auswahl wird nicht in Screen-Column-Zustand umgewandelt.

Breite Terminal-Glyphen werden atomar gestylt: `writeScalar()` erhält genau einen Selection-Stil und wendet ihn sowohl auf die Lead- als auch auf die Continuation-Cell an. Ein ausgewählter breiter Scalar kann dadurch nicht mit uneinheitlicher Cell-Darstellung erscheinen.

### Folgen

Per Shift oder programmatisch erzeugte Auswahlen sind nun im Terminal-Backend sichtbar, ohne die `TextField`-Semantik oder Measurement zu verändern.

Horizontales Scrolling verändert die Bedeutung der Auswahl nicht. Lediglich der sichtbare Teil der ausgewählten Scalars wird gezeichnet; der semantische Bereich bleibt im Core unverändert.

Auswahl bleibt reiner Presentation-State und benötigt daher weiterhin Visual-Invalidierung, aber keine Measurement-Invalidierung.

Die Regel zum Umschalten von `inverse` ist bewusst eine Terminal-Presentation-Policy und kein universeller Theme-Vertrag. Die Rendered-Desktop-Darstellung kann später passende Rechtecke bzw. Hintergrundflächen verwenden, ohne an Terminal-Cell-Semantik gebunden zu sein.

Pointer-Drag-Auswahl und Terminal-Maus-Hit-Testing bleiben getrennte Slices.

### Betrachtete Alternativen

**Für ausgewählten Text immer `inverse = true` setzen.** Verworfen, weil ein fokussiertes `TextField` bereits vollständig invers dargestellt wird und die Auswahl dadurch unsichtbar wäre.

**Jetzt einen öffentlichen Selection-Foreground/Background-Stil im Core einführen.** Zurückgestellt, weil diese konkrete Darstellungsregel derzeit nur das Terminal-Backend benötigt. Eine breitere Styling-API soll erst durch Rendered- und native Backends validiert werden.

**Ausgewählte Screen-Column-Bereiche im `TextField` speichern.** Verworfen, weil Auswahl semantischer Unicode-Scalar-Zustand ist. Cell-Geometrie und horizontales Clipping gehören in das Terminal-Presentation-Backend.
