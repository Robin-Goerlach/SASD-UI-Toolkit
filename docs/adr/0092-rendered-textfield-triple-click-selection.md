# ADR 0092 – Rendered TextField triple-click selects complete single-line content

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0091 introduced atomic unmodified double-click word selection for rendered TextField interaction. TextField is intrinsically a single-line control, while desktop text editors commonly use a third click to select the complete logical line.

The toolkit already carries click counts in backend-neutral PointerEvent instances, and TextField selection is expressed as Unicode-scalar indices. No additional native backend contract is therefore required to add a useful triple-click semantic.

A naive implementation could route triple click through rendered caret/scalar geometry and infer a line from the hit point. That would add unnecessary shaping dependencies for a control that can contain only one semantic line. It would also make the meaning of triple click depend on whether a particular scalar boundary is representable even though the requested semantic range is the entire field.

### Decision

An exact unmodified primary press with `click_count == 3` on a rendered TextField selects the complete TextField content.

The selection is established as:

`[0, utf8::scalarCount(field.text()))`

This keeps the result in the same Unicode-scalar index domain as all existing TextField cursor and selection operations.

Triple-click selection does not call `caretIndexAt()` or `scalarIndexAt()`. Once ordinary widget hit testing has established that the deepest semantic target is the TextField, no additional font/shaping geometry is necessary because the control has exactly one logical line.

### Atomic multi-click behavior

Like ADR 0091 double-click word selection, triple-click whole-content selection is atomic in this slice.

TextField still handles the primary press so normal PointerRouter ownership rules apply. If that press creates capture, `RenderedTextFieldPointerSelection` immediately releases it through `PointerRouter::releaseCapture()` after applying the semantic selection.

This matters because a matching release must not run the ordinary character-drag finalization path and collapse or shrink the newly established range. Releasing through PointerRouter also preserves the existing lifetime handshake and invokes TextField's capture-loss cleanup rather than reaching into transient Core state directly.

### Modifier policy

The new behavior requires an exact no-modifier triple click.

Ctrl/Alt/Meta/Shift combinations are deliberately not assigned select-all semantics by this ADR. They continue through the previously defined ordinary/Shift pointer policies. This leaves room for later platform-specific conventions without silently freezing them into the generic contract.

### Why this is not a public TextField `selectAll()` API yet

The semantic result can already be expressed with the stable public `setSelection()` contract. Adding another public Core method is unnecessary for this interaction slice and would broaden the API without proving an independent need from command/menu integration.

If later command work needs a first-class Select All operation, that can be added deliberately with its own enablement and command semantics.

### Consequences

- rendered TextField now supports conventional single-, double- and triple-click selection progression;
- triple click selects the complete single-line value independent of shaping details;
- Unicode-scalar selection invariants remain unchanged;
- matching release cannot destroy the multi-click selection;
- no native backend type or pixel geometry enters Core TextField;
- modified triple-click behavior remains available for later policy.

### Deferred scope

This ADR does not define:

- double-click-and-drag word-granular extension;
- triple-click-and-drag line-granular extension;
- multiline controls;
- a public Select All command/API;
- grapheme-aware or UAX #29 word breaking;
- platform-specific modified multi-click conventions.

---

## Deutsch

### Kontext

ADR 0091 führte für gerenderte TextFields die atomare Wortauswahl per unmodifiziertem Doppelklick ein. TextField ist von seiner Semantik her ein einzeiliges Steuerelement; Desktop-Texteditoren verwenden häufig einen dritten Klick, um die komplette logische Zeile auszuwählen.

Das Toolkit transportiert die Klickanzahl bereits backend-neutral im PointerEvent, während TextField Cursor und Auswahl als Unicode-Scalar-Indizes speichert. Für eine sinnvolle Triple-Click-Semantik ist daher kein zusätzlicher nativer Backend-Vertrag notwendig.

Eine naive Implementierung könnte den Triple Click erneut durch Rendered-Caret-/Scalar-Geometrie leiten und daraus eine Zeile ableiten. Für ein Steuerelement mit genau einer semantischen Zeile wäre das unnötig. Außerdem würde die Bedeutung des Triple Click davon abhängen, ob eine konkrete Shaping-Grenze darstellbar ist, obwohl die gewünschte Auswahl ohnehin den gesamten Inhalt umfasst.

### Entscheidung

Ein exakter unmodifizierter Primary Press mit `click_count == 3` auf einem gerenderten TextField wählt den vollständigen TextField-Inhalt aus.

Die Auswahl wird als

`[0, utf8::scalarCount(field.text()))`

angelegt. Damit bleibt sie exakt im bereits vorhandenen Unicode-Scalar-Indexraum aller TextField-Cursor- und Auswahloperationen.

Für Triple-Click-Auswahl werden weder `caretIndexAt()` noch `scalarIndexAt()` benötigt. Sobald das normale Widget-HitTesting das TextField als tiefstes semantisches Ziel bestimmt hat, ist keine weitere Font-/Shaping-Geometrie nötig, weil das Steuerelement genau eine logische Zeile besitzt.

### Atomare Multi-Click-Semantik

Wie die Wortauswahl aus ADR 0091 ist auch die vollständige Triple-Click-Auswahl in diesem Slice atomar.

TextField behandelt den Primary Press weiterhin, sodass die normalen PointerRouter-Ownership-Regeln gelten. Entsteht dadurch Capture, gibt `RenderedTextFieldPointerSelection` dieses nach dem Anwenden der semantischen Auswahl unmittelbar über `PointerRouter::releaseCapture()` wieder frei.

Das ist wichtig, weil der passende Release nicht in die normale Character-Drag-Finalisierung geraten und die gerade erzeugte Auswahl wieder verkleinern oder zusammenklappen darf. Die Freigabe über PointerRouter erhält außerdem den bestehenden Lifetime-Handshake und ruft die Capture-Loss-Bereinigung des TextField auf, statt transienten Core-Zustand direkt zu manipulieren.

### Modifier-Policy

Das neue Verhalten gilt nur für einen exakten Triple Click ohne Modifier.

Ctrl/Alt/Meta/Shift-Kombinationen erhalten durch diese ADR bewusst keine Select-All-Bedeutung. Sie folgen weiterhin den bereits definierten normalen bzw. Shift-Pointer-Regeln. Dadurch bleibt Raum für spätere plattformspezifische Konventionen.

### Warum noch keine öffentliche `TextField::selectAll()`-API

Das semantische Ergebnis lässt sich bereits vollständig über den stabilen öffentlichen `setSelection()`-Vertrag ausdrücken. Eine zusätzliche öffentliche Core-Methode würde die API in diesem Interaktions-Slice verbreitern, ohne dass dafür schon ein unabhängiger Bedarf nachgewiesen ist.

Wenn spätere Command-/Menüarbeit einen erstklassigen Select-All-Befehl benötigt, kann dieser bewusst mit eigener Enablement- und Command-Semantik ergänzt werden.

### Folgen

- Rendered TextField unterstützt nun die übliche Progression aus Einzel-, Doppel- und Triple-Click;
- Triple Click wählt den vollständigen einzeiligen Wert unabhängig von Shaping-Details;
- Unicode-Scalar-Auswahlinvarianten bleiben unverändert;
- der passende Release kann die Multi-Click-Auswahl nicht zerstören;
- keine nativen Backend-Typen oder Pixelgeometrie gelangen in Core TextField;
- modifizierte Triple-Click-Semantik bleibt für spätere Policy offen.

### Bewusst später

Diese ADR definiert noch nicht:

- Double-Click-and-Drag mit Wortgranularität;
- Triple-Click-and-Drag mit Zeilengranularität;
- mehrzeilige Controls;
- eine öffentliche Select-All-Command/API;
- Grapheme-aware bzw. UAX-29-Wortgrenzen;
- plattformspezifische modifizierte Multi-Click-Konventionen.
