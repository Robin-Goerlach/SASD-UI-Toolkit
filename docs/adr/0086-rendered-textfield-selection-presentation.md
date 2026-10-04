# ADR 0086 – Rendered TextField selection presentation by full-run replay and clipped contrast overlay

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

`TextField` already owns backend-neutral directed selection state in Unicode-scalar indices. ADR 0085 made that selection visible in the terminal backend by toggling `TextStyle::inverse` for selected terminal scalars. The rendered backend still emitted only one `DrawTextCommand` for the complete field text, so the same semantic selection remained visually invisible on rendered desktop surfaces.

Rendered text cannot safely be treated like terminal cells. A shaping backend may apply kerning, ligatures, combining-mark placement, script shaping or other context-sensitive layout to the complete UTF-8 run. Re-shaping `selectedText()` as an independent substring can therefore produce glyph positions that do not exactly match the corresponding range inside the original full run.

The existing `RenderedMeasurementContext::textAdvanceToScalar()` contract already supplies scalar-boundary advances measured in the complete text context and is also used by TextField viewport, caret and pointer hit-testing. The `DisplayList` additionally supports per-command clipping.

### Decision

Rendered `TextField` selection is presented as a second `DrawTextCommand` that replays the **same complete UTF-8 text run** at the **same origin** as the ordinary field text, but with:

- a clip rectangle derived from the selected Unicode-scalar boundaries;
- `TextStyle::inverse` toggled relative to the already-resolved field presentation style.

The base text command continues to paint the whole visible TextField content. The selection command is then overlaid only inside the visible intersection of the semantic selection range and the TextField content viewport. The caret, when present, is emitted after both text commands so it remains visible over the selection overlay.

Selection geometry is derived from `RenderedMeasurementContext::textAdvanceToScalar(field.text(), boundary)` for `selectionStart()` and `selectionEnd()`. No selected substring is measured independently and no pixel coordinates are stored in Core.

All selection-boundary metrics and widened coordinate arithmetic are preflighted before the DisplayList is mutated. If a measurement provider cannot express a required selection boundary, presentation returns `deferred` and leaves the previous DisplayList untouched.

### Consequences

Rendered selection remains consistent with the shaping context used for the ordinary text run, viewport and caret. Kerning and ligature context are not lost merely because part of the text is selected.

A focused TextField whose base style is already inverse still shows a distinguishable selection because the overlay toggles inverse back off. Other style attributes such as foreground colour, bold, dim and underline are preserved.

Horizontal scrolling does not change selection semantics. The complete run keeps its existing shifted origin; only the selection clip is intersected with the visible content viewport.

The Core selection model remains backend neutral. Pixel geometry, clipping and replay policy stay inside the Rendered backend.

This is intentionally a presentation solution, not a new public theme contract. A later theming slice may introduce richer selection foreground/background roles if terminal, rendered and native backends converge on a useful common abstraction.

### Alternatives considered

**Render `selectedText()` as a separate substring.** Rejected because independently shaping a substring can change glyph positions, kerning and ligatures compared with the same scalars inside the complete run.

**Split the full text into before/selected/after substrings and render three runs.** Rejected for the same shaping-context reason and because it duplicates viewport arithmetic.

**Store pixel selection bounds in `TextField`.** Rejected because Core selection is semantic Unicode-scalar state. Pixel geometry belongs to the Rendered backend and depends on the active measurement provider and viewport.

**Introduce public selection colours now.** Deferred until multiple backends demonstrate a stable common styling requirement.

---

## Deutsch

### Kontext

`TextField` besitzt bereits einen backend-neutralen gerichteten Auswahlzustand auf Basis von Unicode-Scalar-Indizes. ADR 0085 machte diese Auswahl im Terminal-Backend sichtbar, indem für ausgewählte Terminal-Scalars `TextStyle::inverse` umgeschaltet wird. Das Rendered-Backend erzeugte bislang jedoch nur einen einzigen `DrawTextCommand` für den vollständigen Feldtext. Dadurch blieb dieselbe semantische Auswahl auf gerenderten Desktopoberflächen unsichtbar.

Gerenderter Text darf nicht wie Terminalzellen behandelt werden. Ein Shaping-Backend kann Kerning, Ligaturen, Combining-Mark-Positionierung, Script-Shaping oder andere kontextabhängige Layoutregeln auf den vollständigen UTF-8-Lauf anwenden. Wird `selectedText()` als unabhängiger Teilstring erneut geformt, können sich deshalb Glyphenpositionen ergeben, die nicht exakt zum entsprechenden Bereich des ursprünglichen vollständigen Laufs passen.

Der vorhandene Vertrag `RenderedMeasurementContext::textAdvanceToScalar()` liefert bereits Scalar-Grenzpositionen im Kontext des vollständigen Texts und wird ebenfalls für TextField-Viewport, Caret und Pointer-Hit-Testing genutzt. Die `DisplayList` unterstützt außerdem Clipping pro Command.

### Entscheidung

Die Rendered-Darstellung einer `TextField`-Auswahl erfolgt als zweiter `DrawTextCommand`, der **denselben vollständigen UTF-8-Textlauf** am **gleichen Ursprung** wie der normale Feldtext erneut zeichnet, jedoch mit:

- einem aus den ausgewählten Unicode-Scalar-Grenzen abgeleiteten Clip-Rechteck;
- relativ zum bereits aufgelösten Darstellungsstil umgeschaltetem `TextStyle::inverse`.

Der Basis-Textcommand zeichnet weiterhin den gesamten sichtbaren Inhalt des TextFields. Anschließend wird der Selection-Command nur innerhalb der sichtbaren Schnittmenge aus semantischem Auswahlbereich und Content-Viewport darübergelegt. Ein vorhandener Caret wird nach beiden Textcommands ausgegeben und bleibt dadurch über dem Selection-Overlay sichtbar.

Die Auswahlgeometrie wird mit `RenderedMeasurementContext::textAdvanceToScalar(field.text(), boundary)` für `selectionStart()` und `selectionEnd()` ermittelt. Es wird kein ausgewählter Teilstring separat vermessen und der Core speichert keine Pixelkoordinaten.

Alle Selection-Grenzmetriken und verbreiterten Koordinatenberechnungen werden vollständig geprüft, bevor die DisplayList verändert wird. Kann ein Measurement-Provider eine benötigte Auswahlgrenze nicht ausdrücken, liefert die Presentation `deferred` und lässt die vorherige DisplayList unverändert.

### Folgen

Die Rendered-Auswahl bleibt konsistent mit demselben Shaping-Kontext, der auch für normalen Textlauf, Viewport und Caret verwendet wird. Kerning- und Ligaturkontext gehen nicht verloren, nur weil ein Textabschnitt ausgewählt ist.

Ein fokussiertes TextField, dessen Basisstil bereits invers ist, zeigt weiterhin eine unterscheidbare Auswahl, weil das Overlay `inverse` wieder ausschaltet. Andere Stilattribute wie Vordergrundfarbe, Bold, Dim und Underline bleiben erhalten.

Horizontales Scrolling verändert die Auswahlssemantik nicht. Der vollständige Textlauf behält seinen bereits verschobenen Ursprung; lediglich der Selection-Clip wird mit dem sichtbaren Content-Viewport geschnitten.

Das Core-Auswahlmodell bleibt backend-neutral. Pixelgeometrie, Clipping und Replay-Policy verbleiben vollständig im Rendered-Backend.

Dies ist bewusst eine Presentation-Lösung und noch kein neuer öffentlicher Theme-Vertrag. Ein späterer Theming-Slice kann reichhaltigere Selection-Foreground/Background-Rollen einführen, wenn Terminal-, Rendered- und native Backends eine tragfähige gemeinsame Abstraktion bestätigt haben.

### Betrachtete Alternativen

**`selectedText()` als separaten Teilstring rendern.** Verworfen, weil unabhängiges Shaping eines Teilstrings Glyphenpositionen, Kerning und Ligaturen gegenüber denselben Scalars im vollständigen Lauf verändern kann.

**Den vollständigen Text in Before/Selected/After aufteilen und drei Läufe rendern.** Aus demselben Shaping-Kontext-Grund verworfen und weil dadurch Viewport-Arithmetik dupliziert würde.

**Pixelbasierte Auswahlgrenzen im `TextField` speichern.** Verworfen, weil die Core-Auswahl semantischer Unicode-Scalar-Zustand ist. Pixelgeometrie gehört in das Rendered-Backend und hängt vom aktiven Measurement-Provider sowie vom Viewport ab.

**Jetzt öffentliche Selection-Farben einführen.** Zurückgestellt, bis mehrere Backends eine stabile gemeinsame Styling-Anforderung belegen.
