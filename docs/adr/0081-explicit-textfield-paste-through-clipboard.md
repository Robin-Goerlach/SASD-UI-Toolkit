# ADR 0081 – Explicit TextField paste through the clipboard service

**Status:** Accepted  
**Date:** 2026-10-03

## English

### Context

ADR 0080 introduced the backend-neutral `Clipboard` service, but deliberately stopped before making an editable control consume it. `TextField` already has one well-tested insertion path for `TextInputEvent`: text is sanitized to valid single-line UTF-8, inserted at a Unicode-scalar cursor boundary, and only actual mutations invalidate measurement and presentation.

The first clipboard consumer should reuse that behavior without prematurely solving selection, copy/cut, printable keyboard shortcuts or backend-specific clipboard acquisition. In particular, the current `TextField` has no selection model, so inventing “copy” as “copy the whole field” would establish surprising semantics that would later have to be undone.

### Decision

Add the explicit public operation:

```cpp
[[nodiscard]] bool TextField::pasteFromClipboard(const Clipboard& clipboard);
```

The operation has the following contract:

- `TextField` borrows the clipboard only for the duration of the call and stores no `Clipboard`, `Backend` or `Application` pointer.
- `Clipboard::readText()` is performed before any `TextField` mutation. If reading throws, the field remains unchanged and the exception propagates according to the clipboard contract.
- `std::nullopt` means there is no textual payload and returns `false` without invalidation.
- A present payload is passed through the same single-line UTF-8 sanitization and insertion path used by `TextInputEvent`.
- The text is inserted at the current Unicode-scalar cursor position, and the cursor advances by the number of inserted sanitized scalars.
- The function returns `true` only when sanitized text was actually inserted. Empty text or a payload that sanitizes entirely away returns `false` and does not dirty measurement/presentation.
- Programmatic paste does not require logical focus, visibility or enabled state. Those conditions govern routed user input; an explicit editing API is analogous to `setText()` and can be invoked by application/command code after it has chosen the editing target.

The private insertion helper now returns whether it changed content, so `TextInputEvent` and clipboard paste continue to share one mutation implementation. Routed `TextInputEvent` remains `handled` even when sanitization produces no mutation because the focused editor still owns that text-input transaction.

Copy and cut are not added in this slice. They depend on a future selection model. No Ctrl+V binding is added either; printable shortcut identity is a separate keyboard-model concern and should not be smuggled into clipboard semantics.

### Consequences

`TextField` becomes the first real consumer of the backend-neutral clipboard abstraction while remaining independent of native platform APIs and backend lifetime. Menu/command/application code can obtain a currently usable clipboard service from the backend and pass it explicitly to the field.

Terminal, SDL3 and future native adapters still decide whether they expose a clipboard service at all. This change does not claim new platform clipboard support.

The API is intentionally correctness-first. Reading returns an owned string and insertion sanitizes/copies it before mutation. Copy reduction, richer clipboard formats and selection-aware replacement can be added later without changing the service boundary established here.

### Alternatives considered

**Store a Clipboard pointer inside TextField.** Rejected because that would couple widget lifetime to backend/application service lifetime and make testing/composition less explicit.

**Make TextField find Application/Backend globally.** Rejected because the project avoids hidden global runtime dependencies and because widgets should not need platform-host knowledge to edit semantic text.

**Implement copy/cut by treating the whole field as selected.** Rejected because that is not conventional text-editing semantics and would conflict with the future explicit selection model.

**Route paste as a synthetic TextInputEvent.** Rejected for the explicit API. Clipboard reading is a service operation and may throw; synthesizing an event would hide the distinction between committed platform text input and an application-requested clipboard action. Both paths instead share the same private insertion primitive.

---

## Deutsch

### Kontext

ADR 0080 hat den backend-neutralen `Clipboard`-Dienst eingeführt, aber bewusst noch keinen editierbaren Consumer daran angebunden. `TextField` besitzt bereits einen gut getesteten Einfügepfad für `TextInputEvent`: Text wird zu gültigem einzeiligem UTF-8 bereinigt, an einer Unicode-Scalar-Cursorgrenze eingefügt, und nur tatsächliche Änderungen invalidieren Measurement und Presentation.

Der erste Clipboard-Consumer soll genau dieses Verhalten wiederverwenden, ohne Selection, Copy/Cut, druckbare Keyboard-Shortcuts oder backend-spezifische Clipboard-Beschaffung vorzeitig mitzulösen. Das aktuelle `TextField` besitzt insbesondere noch kein Selection-Modell. „Copy“ jetzt als „kopiere das gesamte Feld“ zu definieren, würde daher überraschende Semantik etablieren, die später wieder korrigiert werden müsste.

### Entscheidung

Es wird die explizite öffentliche Operation eingeführt:

```cpp
[[nodiscard]] bool TextField::pasteFromClipboard(const Clipboard& clipboard);
```

Der Vertrag lautet:

- `TextField` leiht das Clipboard nur für die Dauer des Aufrufs und speichert keinen `Clipboard`-, `Backend`- oder `Application`-Pointer.
- `Clipboard::readText()` erfolgt vor jeder Änderung am `TextField`. Wirft das Lesen eine Exception, bleibt das Feld unverändert und die Exception wird gemäß Clipboard-Vertrag weitergegeben.
- `std::nullopt` bedeutet, dass kein Textinhalt vorhanden ist; die Methode liefert `false`, ohne zu invalidieren.
- Ein vorhandener Inhalt durchläuft denselben einzeiligen UTF-8-Sanitizing- und Einfügepfad wie `TextInputEvent`.
- Der Text wird an der aktuellen Unicode-Scalar-Cursorposition eingefügt; der Cursor wird um die Zahl der tatsächlich eingefügten bereinigten Scalars weiterbewegt.
- Die Funktion liefert nur dann `true`, wenn bereinigter Text tatsächlich eingefügt wurde. Leerer Text oder vollständig herausgefilterter Inhalt liefert `false` und macht Measurement/Presentation nicht dirty.
- Programmatisches Paste benötigt keinen logischen Fokus sowie keinen Visible-/Enabled-Zustand. Diese Bedingungen steuern gerouteten Benutzereingang; eine explizite Editing-API ist wie `setText()` und kann von Anwendung/Command aufgerufen werden, nachdem das Ziel bereits bestimmt wurde.

Der private Einfüge-Helper liefert nun zurück, ob er den Inhalt geändert hat. Dadurch teilen `TextInputEvent` und Clipboard-Paste weiterhin genau eine Mutation-Implementierung. Ein geroutetes `TextInputEvent` bleibt auch dann `handled`, wenn Sanitizing keine Änderung erzeugt, weil das fokussierte Eingabefeld die Textinput-Transaktion trotzdem besitzt.

Copy und Cut werden in diesem Slice bewusst nicht eingeführt. Dafür wird zunächst ein Selection-Modell benötigt. Ebenso wird kein Ctrl+V-Binding erfunden; die Identität druckbarer Shortcuts ist ein eigenes Keyboard-Modell-Thema und soll nicht versteckt Teil der Clipboard-Semantik werden.

### Folgen

`TextField` wird zum ersten realen Consumer der backend-neutralen Clipboard-Abstraktion und bleibt trotzdem unabhängig von nativen Plattform-APIs und Backend-Lebensdauer. Menü-, Command- oder Anwendungscode kann einen aktuell nutzbaren Clipboard-Dienst beim Backend beziehen und explizit an das Feld übergeben.

Terminal, SDL3 und spätere native Adapter entscheiden weiterhin selbst, ob sie überhaupt einen Clipboard-Dienst bereitstellen. Diese Änderung behauptet keine neue Plattform-Clipboard-Unterstützung.

Die API bleibt bewusst correctness-first. Lesen liefert einen eigenen String, der vor der Mutation bereinigt und eingefügt wird. Weniger Kopien, reichere Clipboard-Formate und selection-aware Replacement können später ergänzt werden, ohne die hier etablierte Service-Grenze zu verändern.

### Betrachtete Alternativen

**Einen Clipboard-Pointer im TextField speichern.** Verworfen, weil dadurch Widget-Lebensdauer an Backend-/Application-Service-Lebensdauer gekoppelt würde und Tests/Komposition weniger explizit wären.

**TextField global nach Application/Backend suchen lassen.** Verworfen, weil das Projekt versteckte globale Runtime-Abhängigkeiten vermeidet und Widgets für semantische Textbearbeitung kein Wissen über den Plattform-Host benötigen sollen.

**Copy/Cut durch implizite Auswahl des gesamten Feldes umsetzen.** Verworfen, weil das nicht der üblichen Texteditor-Semantik entspricht und mit einem späteren expliziten Selection-Modell kollidieren würde.

**Paste als synthetisches TextInputEvent routen.** Für die explizite API verworfen. Clipboard-Lesen ist eine Service-Operation und kann Exceptions werfen; ein synthetisches Event würde den Unterschied zwischen committed Plattform-Textinput und einer von der Anwendung angeforderten Clipboard-Aktion verwischen. Beide Pfade teilen stattdessen dasselbe private Einfüge-Primitiv.
