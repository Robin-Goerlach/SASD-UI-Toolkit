# ADR 0029 – Render device replay boundary below DisplayList

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

ADR 0027 established `rendered::DisplayList` as the deterministic, SDL-independent intermediate
representation for M3. `RenderedPresentationSink` now translates semantic Widgets into that command
stream, and ADR 0028 adds rendered text metrics for TextField viewport/caret placement.

The next architectural step is to execute the already-produced commands on a real desktop drawing
substrate. Letting every future adapter inspect `std::variant<DrawCommand>` directly would duplicate
variant dispatch and make it easy for individual adapters to reinterpret or omit command semantics.
Putting SDL3 calls directly into DisplayList would instead destroy the dependency boundary that M3
was designed to protect.

A device boundary is therefore needed between the passive command list and a concrete renderer.

### Decision

The `SASD::UI::Rendered` layer introduces:

- `rendered::RenderDevice`, a small device-facing interface with one operation for each current
  DisplayList command family;
- `rendered::DisplayListExecutor`, which replays an immutable DisplayList synchronously and in exact
  command order into one RenderDevice.

The initial RenderDevice operations are deliberately the same semantic primitives already proven by
DisplayList:

- `fillRect(FillRectCommand)`;
- `strokeRect(StrokeRectCommand)`;
- `drawText(DrawTextCommand)`.

The interface receives complete command value objects instead of SDL/native arguments. Logical
coordinates, portable `Color`, `TextStyle`, UTF-8 ownership and optional text clipping therefore
survive unchanged until the concrete device boundary.

`DisplayListExecutor` owns the `std::variant` visitation. Concrete devices do not need to duplicate
that dispatch. Adding another `DrawCommand` type consequently requires the executor/device boundary
to be extended deliberately rather than being silently ignored by an adapter.

Execution is synchronous and observational with respect to DisplayList:

- source commands are not removed, reordered or mutated;
- successful callbacks are dispatched in list order;
- an empty list performs no device callbacks;
- if a device callback throws, the exception propagates immediately and later commands are not
  executed;
- no automatic rollback of already-dispatched device work is promised.

The last point is intentional. Immediate-mode APIs and buffered APIs have different transactional
models. A future adapter that needs all-or-nothing presentation should replay into its own back buffer
or staging surface and present only after successful execution.

The first RenderDevice contract intentionally does **not** define:

- window creation/destruction;
- event loops;
- swap/present timing;
- device acquisition/loss;
- DPI policy;
- font loading/shaping resources;
- GPU resource caches.

Those requirements belong to the first concrete desktop adapter and must not be guessed before that
adapter exists.

### Rationale

This gives M3 a clean three-step presentation pipeline:

```text
semantic Widgets
      |
RenderedPresentationSink
      |
   DisplayList
      |
DisplayListExecutor
      |
  RenderDevice
      |
SDL3 / native / test device
```

Each layer has one responsibility. Widget traversal stays in PresentationCoordinator/Sink,
intermediate drawing data stays in DisplayList, generic variant dispatch stays in the executor, and
platform rendering stays in the device.

The boundary is also useful for tests and package validation: a deterministic recording device can
verify the exact order and payload that a real adapter will receive without requiring a desktop
window.

### Alternatives considered

#### Let each adapter iterate DisplayList directly

Rejected. Every adapter would duplicate variant visitation and could accidentally diverge in how
commands are dispatched.

#### Add SDL3 calls directly to DisplayList

Rejected. DisplayList must remain an SDL-independent intermediate representation and usable in
headless tests.

#### Put execution methods on DisplayList itself

Rejected. A passive command snapshot is easier to reason about, serialize/inspect later, and test
without giving it knowledge of concrete device behavior.

#### Add frame/window lifecycle to RenderDevice now

Deferred. The correct lifecycle contract depends on the first real desktop adapter. Freezing it before
SDL3/native experiments would be premature.

#### Make replay transactional automatically

Rejected at this layer. The executor cannot generically roll back an arbitrary immediate-mode device.
Transactional presentation belongs to a concrete adapter's back-buffer/staging policy.

### Consequences

- the first concrete SDL3/native adapter can focus on drawing instead of semantic Widget traversal;
- command order and payload remain deterministic across devices;
- the Rendered public API gains a small, testable device seam without SDL/native types;
- future command additions require explicit compile-time changes at the executor/device boundary;
- device-side transaction/frame lifecycle remains intentionally open until a real adapter validates
  those requirements.

---

## Deutsch

### Kontext

ADR 0027 hat `rendered::DisplayList` als deterministische, SDL-unabhängige Zwischendarstellung für
M3 festgelegt. Der `RenderedPresentationSink` übersetzt inzwischen semantische Widgets in diesen
Befehlsstrom; ADR 0028 ergänzt gerenderte Textmetriken für Viewport und Caret des TextFields.

Der nächste Architekturschritt ist die Ausführung dieser bereits erzeugten Befehle auf einer realen
Desktop-Zeichenoberfläche. Würde jeder spätere Adapter direkt
`std::variant<DrawCommand>` auswerten, entstünde mehrfach derselbe Dispatch-Code und einzelne
Adapter könnten Befehlssemantik versehentlich unterschiedlich interpretieren oder auslassen.
SDL3-Aufrufe direkt in der DisplayList würden dagegen genau die Abhängigkeitsgrenze zerstören, die M3
schützen soll.

Zwischen passiver Befehlsliste und konkretem Renderer wird daher eine Device-Grenze benötigt.

### Entscheidung

Die Schicht `SASD::UI::Rendered` führt ein:

- `rendered::RenderDevice`, ein kleines Device-Interface mit je einer Operation pro aktueller
  DisplayList-Befehlsfamilie;
- `rendered::DisplayListExecutor`, der eine unveränderliche DisplayList synchron und in exakter
  Befehlsreihenfolge in ein RenderDevice abspielt.

Die ersten RenderDevice-Operationen entsprechen bewusst genau den bereits bewährten
DisplayList-Primitiven:

- `fillRect(FillRectCommand)`;
- `strokeRect(StrokeRectCommand)`;
- `drawText(DrawTextCommand)`.

Das Interface erhält vollständige Command-Value-Objekte statt SDL-/Native-Argumenten. Logische
Koordinaten, portables `Color`, `TextStyle`, UTF-8-Besitz und optionales Text-Clipping bleiben
dadurch bis zur konkreten Device-Grenze unverändert erhalten.

Der `DisplayListExecutor` besitzt den `std::variant`-Dispatch. Konkrete Devices müssen ihn nicht
duplizieren. Ein neuer `DrawCommand`-Typ erzwingt damit eine bewusste Erweiterung der
Executor-/Device-Grenze, statt von einem Adapter unbemerkt ignoriert zu werden.

Die Ausführung ist synchron und gegenüber der DisplayList rein beobachtend:

- Quellbefehle werden weder entfernt noch umsortiert noch verändert;
- erfolgreiche Callbacks laufen in Listenreihenfolge;
- eine leere Liste erzeugt keinen Device-Callback;
- wirft ein Device-Callback eine Exception, wird sie unverändert weitergereicht und spätere Befehle
  werden nicht mehr ausgeführt;
- bereits ausgeführte Device-Arbeit wird nicht automatisch zurückgerollt.

Der letzte Punkt ist Absicht. Immediate-Mode- und gepufferte APIs besitzen unterschiedliche
Transaktionsmodelle. Ein späterer Adapter mit All-or-Nothing-Anforderung soll in einen eigenen
Backbuffer bzw. eine Staging-Surface rendern und erst nach erfolgreichem Replay präsentieren.

Der erste RenderDevice-Vertrag definiert bewusst **noch nicht**:

- Erzeugen/Zerstören von Fenstern;
- Event Loop;
- Swap-/Present-Zeitpunkt;
- Device-Loss/-Acquisition;
- DPI-Policy;
- Font-Laden/Shaping-Ressourcen;
- GPU-Ressourcen-Caches.

Diese Anforderungen sollen aus dem ersten konkreten Desktop-Adapter entstehen und nicht vorher
geraten werden.

### Begründung

M3 erhält damit eine klare dreistufige Presentation-Pipeline:

```text
semantische Widgets
       |
RenderedPresentationSink
       |
    DisplayList
       |
DisplayListExecutor
       |
   RenderDevice
       |
SDL3 / native / Test-Device
```

Jede Schicht besitzt genau eine Aufgabe. Widget-Traversierung bleibt bei
PresentationCoordinator/Sink, Zeichendaten bleiben in der DisplayList, generischer Variant-Dispatch
liegt im Executor und Plattform-Rendering im Device.

Die Grenze ist außerdem für Tests und Package-Validierung wertvoll: Ein deterministisches
Recording-Device kann Reihenfolge und vollständigen Payload prüfen, den später auch ein realer Adapter
erhält, ohne ein Desktopfenster zu benötigen.

### Betrachtete Alternativen

#### Jeder Adapter iteriert die DisplayList selbst

Verworfen. Dadurch würde jeder Adapter Variant-Dispatch duplizieren und könnte in der
Befehlsausführung auseinanderlaufen.

#### SDL3-Aufrufe direkt in DisplayList

Verworfen. DisplayList muss eine SDL-unabhängige Zwischendarstellung und headless testbar bleiben.

#### Ausführungsmethoden direkt auf DisplayList

Verworfen. Ein passiver Command-Snapshot ist leichter zu verstehen, später zu inspizieren/
serialisieren und ohne konkretes Device zu testen.

#### Frame-/Window-Lifecycle bereits in RenderDevice aufnehmen

Verschoben. Der richtige Lifecycle-Vertrag hängt vom ersten realen Desktop-Adapter ab. Ihn vor
SDL3-/Native-Experimenten festzuschreiben wäre verfrüht.

#### Replay automatisch transaktional machen

Auf dieser Ebene verworfen. Ein generischer Executor kann ein beliebiges Immediate-Mode-Device nicht
zurückrollen. Transaktionale Presentation gehört in die Backbuffer-/Staging-Policy des konkreten
Adapters.

### Konsequenzen

- der erste konkrete SDL3-/Native-Adapter kann sich auf Zeichnung statt Widget-Traversierung
  konzentrieren;
- Reihenfolge und Payload bleiben über Devices hinweg deterministisch;
- die öffentliche Rendered-API erhält eine kleine testbare Device-Naht ohne SDL-/Native-Typen;
- neue Commands verlangen explizite Compile-Time-Anpassungen an Executor/Device;
- Frame-/Transaktions-/Lifecycle-Details bleiben bewusst offen, bis ein realer Adapter sie validiert.
