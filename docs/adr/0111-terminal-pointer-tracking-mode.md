# ADR 0111 – Terminal pointer tracking policy is explicit and can opt into all-motion reporting

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0096 introduced RAII-owned terminal pointer reporting using xterm button-event tracking (`DECSET 1002`) plus SGR coordinates (`DECSET 1006`). That mode is intentionally conservative: it reports presses, releases, and pointer motion while a button is held, which is sufficient for click-and-drag selection without continuously streaming passive movement.

ADR 0110 now gives `TerminalMenuPointerInteraction` a safe semantic meaning for any backend-neutral `PointerAction::move` event that reaches an active popup: the topmost visible popup row may become selected, but motion never activates a Command or opens a submenu.

To support genuine passive hover later, compatible terminals must be able to emit movement even when no mouse button is held. xterm-style all-motion tracking (`DECSET 1003`) provides that behavior. Enabling it unconditionally would, however, change event volume and observable behavior for every existing pointer-enabled application.

The protocol choice therefore belongs in `TerminalSessionOptions`, not in menu code, the decoder, or individual Widgets.

### Decision

Add the portable enum:

```cpp
TerminalPointerTrackingMode::button_events
TerminalPointerTrackingMode::all_motion
```

and the option:

```cpp
TerminalPointerTrackingMode pointer_tracking{
    TerminalPointerTrackingMode::button_events};
```

`pointer_input` remains the authoritative opt-in gate. Merely selecting `all_motion` while `pointer_input == false` emits no terminal protocol bytes.

When pointer input is enabled:

```text
button_events
    -> DECSET 1002
    -> DECSET 1006

all_motion
    -> DECSET 1003
    -> DECSET 1006
```

The two tracking requests are alternatives; `1002` and `1003` are not stacked together. SGR coordinates remain common to both modes so `AnsiInputDecoder` and `TerminalEventPump` require no new protocol-specific semantic path.

### Default compatibility

`button_events` is the default tracking policy. Existing callers that only set:

```cpp
options.pointer_input = true;
```

therefore continue to emit exactly the established activation bytes:

```text
ESC [?1002h ESC [?1006h
```

and the established teardown bytes:

```text
ESC [?1006l ESC [?1002l
```

Applications receive passive movement only when they explicitly request `all_motion`.

### RAII ownership and exact inverse teardown

`TerminalSession` stores the active portable tracking mode only after the corresponding enable write succeeds. That optional value replaces the former independent boolean lifetime flag and becomes the single source of truth for pointer-protocol ownership.

Shutdown disables the exact mode that was enabled:

```text
button_events -> ESC [?1006l ESC [?1002l
all_motion    -> ESC [?1006l ESC [?1003l
```

The protocol is disabled before native `endSession()`, while VT output is still usable.

### Transactional construction and rollback

Protocol selection is resolved before native terminal mutation. An invalid explicitly-cast enum value therefore fails with `std::invalid_argument` before `beginSession()`.

After native setup succeeds, activation remains transactional. If the pointer-protocol write throws, the constructor attempts the complete inverse sequence for the requested mode, calls native `endSession()`, and rethrows. The tracking mode is not published as active until the enable write succeeds.

As before, teardown is `noexcept`: a failed best-effort disable write never prevents native terminal restoration.

### Portable introspection

`TerminalSession::pointerTrackingMode()` returns:

```cpp
std::optional<TerminalPointerTrackingMode>
```

`std::nullopt` means that this session does not currently own pointer reporting. The value exposes only portable protocol lifetime state; it says nothing about `PointerRouter` capture, hover ownership, or pending input.

### Separation from semantic hover

This decision changes only the session/protocol capability. It does not change `TerminalMenuPointerInteraction`, `PointerRouter`, `TextField`, or any menu semantic policy.

The existing input pipeline already maps SGR motion to backend-neutral `PointerEvent` values. Therefore a host that later opts into `all_motion` can reuse the motion-selection behavior from ADR 0110 without teaching Core about DEC/xterm modes.

### Tests

Regression coverage verifies:

- pointer reporting remains silent by default;
- choosing `all_motion` without enabling `pointer_input` remains inert;
- existing pointer-enabled callers still use `1002 + 1006`;
- explicit all-motion sessions use `1003 + 1006`;
- each tracking mode tears down with its exact inverse sequence;
- all-motion activation failure uses the same constructor rollback boundary;
- shutdown write failure still restores native state and clears published pointer ownership.

### Consequences

- passive terminal motion becomes an explicit portable capability rather than an implicit menu side effect;
- existing pointer-enabled applications retain their lower-volume button-event behavior;
- session lifetime owns the selected terminal protocol end-to-end;
- decoder, event pump, Core routing, and menu semantics stay backend-neutral;
- a later demo/application slice can opt into `all_motion` without another architectural change to menu selection.

### Deferred scope

This decision does not add:

- automatic all-motion enablement in the terminal demo;
- runtime switching between tracking modes inside one `TerminalSession`;
- hover-delay timers for submenu opening;
- top-level menu-title switching on passive movement;
- terminal capability negotiation for unsupported xterm modes;
- rendered/native hover policy changes.

---

## Deutsch

### Kontext

ADR 0096 hat RAII-verwaltetes Terminal-Pointer-Reporting mit xterm Button-Event-Tracking (`DECSET 1002`) plus SGR-Koordinaten (`DECSET 1006`) eingeführt. Dieser Modus ist bewusst zurückhaltend: Er liefert Press, Release und Pointer-Bewegung bei gedrückter Maustaste und reicht damit für Click-and-Drag-Selektion aus, ohne permanent passive Bewegung zu senden.

ADR 0110 definiert nun eine sichere semantische Bedeutung für jedes backend-neutrale `PointerAction::move`, das ein aktives Popup erreicht: Die oberste sichtbare Popup-Zeile darf ausgewählt werden, Motion aktiviert aber niemals einen Command und öffnet kein Submenu.

Für echtes passives Hover müssen kompatible Terminals Bewegung auch ohne gedrückte Maustaste melden können. xterm All-Motion-Tracking (`DECSET 1003`) liefert genau dieses Verhalten. Eine globale Aktivierung würde jedoch Event-Menge und beobachtbares Verhalten aller bestehenden Pointer-Anwendungen verändern.

Die Protokollwahl gehört deshalb in `TerminalSessionOptions` und nicht in Menücode, Decoder oder einzelne Widgets.

### Entscheidung

Wir ergänzen den portablen Enum:

```cpp
TerminalPointerTrackingMode::button_events
TerminalPointerTrackingMode::all_motion
```

und die Option:

```cpp
TerminalPointerTrackingMode pointer_tracking{
    TerminalPointerTrackingMode::button_events};
```

`pointer_input` bleibt der maßgebliche Opt-in-Schalter. Wird lediglich `all_motion` gesetzt, während `pointer_input == false` ist, werden keinerlei Terminal-Protokollbytes ausgegeben.

Bei aktiviertem Pointer-Input gilt:

```text
button_events
    -> DECSET 1002
    -> DECSET 1006

all_motion
    -> DECSET 1003
    -> DECSET 1006
```

Beide Tracking-Anforderungen sind Alternativen; `1002` und `1003` werden nicht gleichzeitig gestapelt. SGR-Koordinaten bleiben für beide Modi identisch, sodass `AnsiInputDecoder` und `TerminalEventPump` keinen neuen protokollspezifischen Semantikpfad benötigen.

### Kompatibler Default

`button_events` bleibt die Standard-Policy. Bestehende Aufrufer, die lediglich:

```cpp
options.pointer_input = true;
```

setzen, senden deshalb weiterhin exakt die bisherigen Aktivierungsbytes:

```text
ESC [?1002h ESC [?1006h
```

und die bisherigen Teardown-Bytes:

```text
ESC [?1006l ESC [?1002l
```

Passive Bewegung erhalten Anwendungen nur nach expliziter Wahl von `all_motion`.

### RAII-Eigentum und exakter inverser Teardown

`TerminalSession` speichert den aktiven portablen Tracking-Modus erst nach erfolgreichem Enable-Write. Dieser optionale Wert ersetzt das frühere separate Boolean-Lifetime-Flag und wird zur einzigen Quelle für Pointer-Protokoll-Eigentum.

Beim Shutdown wird exakt der Modus deaktiviert, der zuvor aktiviert wurde:

```text
button_events -> ESC [?1006l ESC [?1002l
all_motion    -> ESC [?1006l ESC [?1003l
```

Die Protokolle werden weiterhin vor dem nativen `endSession()` abgeschaltet, solange VT-Ausgabe noch verfügbar ist.

### Transaktionale Konstruktion und Rollback

Die Protokollwahl wird vor jeder nativen Terminal-Mutation aufgelöst. Ein durch expliziten Cast erzeugter ungültiger Enum-Wert führt deshalb vor `beginSession()` zu `std::invalid_argument`.

Nach erfolgreichem nativen Setup bleibt die Aktivierung transaktional. Wirft der Pointer-Protokoll-Write eine Exception, versucht der Konstruktor die vollständige inverse Sequenz des angeforderten Modus, ruft das native `endSession()` auf und wirft weiter. Der Tracking-Modus wird erst nach erfolgreichem Enable-Write als aktiv veröffentlicht.

Wie bisher bleibt der Teardown `noexcept`: Ein fehlgeschlagener Best-Effort-Disable-Write darf die Wiederherstellung des nativen Terminals niemals verhindern.

### Portable Zustandsabfrage

`TerminalSession::pointerTrackingMode()` liefert:

```cpp
std::optional<TerminalPointerTrackingMode>
```

`std::nullopt` bedeutet, dass diese Session aktuell kein Pointer-Reporting besitzt. Der Wert beschreibt ausschließlich portablen Protokoll-Lifetime-State und sagt nichts über `PointerRouter`-Capture, Hover-Eigentum oder wartende Eingaben aus.

### Trennung von semantischem Hover

Diese Entscheidung verändert ausschließlich Session-/Protokollfähigkeit. `TerminalMenuPointerInteraction`, `PointerRouter`, `TextField` und die Menüsemantik bleiben unverändert.

Die vorhandene Input-Pipeline übersetzt SGR-Motion bereits in backend-neutrale `PointerEvent`-Werte. Ein Host kann deshalb später `all_motion` aktivieren und die Motion-Selection aus ADR 0110 wiederverwenden, ohne Core DEC-/xterm-Modi beizubringen.

### Tests

Regressionstests sichern ab:

- Pointer-Reporting bleibt standardmäßig stumm;
- `all_motion` ohne `pointer_input` bleibt wirkungslos;
- bestehende pointer-aktivierte Aufrufer verwenden weiterhin `1002 + 1006`;
- explizite All-Motion-Sessions verwenden `1003 + 1006`;
- jeder Tracking-Modus wird mit seiner exakten inversen Sequenz beendet;
- ein Fehler beim Aktivieren von All-Motion nutzt dieselbe Konstruktor-Rollback-Grenze;
- ein Fehler beim Shutdown-Write verhindert die native Wiederherstellung nicht und löscht veröffentlichtes Pointer-Eigentum.

### Konsequenzen

- passive Terminal-Bewegung wird eine explizite portable Fähigkeit statt eines versteckten Menü-Seiteneffekts;
- bestehende Pointer-Anwendungen behalten das sparsamere Button-Event-Verhalten;
- die Session besitzt das ausgewählte Terminal-Protokoll vollständig über dessen Lebensdauer;
- Decoder, Event-Pump, Core-Routing und Menüsemantik bleiben backend-neutral;
- ein späterer Demo-/Anwendungs-Slice kann `all_motion` aktivieren, ohne die Menüarchitektur erneut zu verändern.

### Aufgeschobener Umfang

Diese Entscheidung ergänzt noch nicht:

- automatische All-Motion-Aktivierung im Terminal-Demo;
- Laufzeitwechsel zwischen Tracking-Modi innerhalb einer `TerminalSession`;
- Hover-Verzögerungstimer zum Öffnen von Submenus;
- Wechsel von Top-Level-Menütiteln durch passive Bewegung;
- Terminal-Capability-Negotiation für nicht unterstützte xterm-Modi;
- Änderungen an Rendered-/Native-Hover-Policies.
