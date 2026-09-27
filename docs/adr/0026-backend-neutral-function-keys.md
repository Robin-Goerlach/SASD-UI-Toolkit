# ADR 0026 – Backend-neutral F1–F12 identity and terminal function-key decoding

**Status:** Accepted  
**Date:** 2026-09-27

## English

### Context

The terminal backend already maps navigation/editing escape sequences to backend-neutral `KeyEvent`
values. Function keys were deliberately deferred in ADR 0023 because the initial `Key` model did not
yet expose their identity.

For administration-oriented TUIs, function keys are useful for stable application actions such as
Help, contextual commands and Exit. They are also not inherently terminal-specific: graphical/native
backends receive equivalent physical keys through their own platform APIs.

### Decision

The backend-neutral `Key` enumeration gains `f1` through `f12`.

`AnsiInputDecoder` maps the common VT/xterm encodings to those semantic values:

- SS3 `ESC O P` through `ESC O S` for F1–F4;
- legacy CSI `11~` through `14~` for F1–F4;
- CSI `15~`, `17~` … `24~` for F5–F12;
- xterm modifier parameters where the existing modifier grammar applies, including
  `CSI 1;<modifier>P..S` and `CSI <function>;<modifier>~`.

Incremental decoding remains unchanged: escape sequences may be split across non-blocking reads and
are completed or timed out by the existing decoder/event-pump contract.

No built-in application action is attached to a function key. The demo uses F1 for Help and F10 for
Exit as ordinary application-level examples.

### Rationale

Function-key identity belongs in the shared event vocabulary. Encoding details belong in the backend.

This preserves the existing direction:

```text
terminal bytes / native key codes
             |
          backend
             |
          KeyEvent
             |
        application/widgets
```

Applications can therefore bind `Key::f1` without knowing whether it came from xterm, Windows,
SDL, GTK or another future backend.

### Deliberately deferred

This decision does not define:

- F13 and higher function keys;
- arbitrary letter/digit physical-key identity;
- Kitty Keyboard Protocol;
- complete CSI-u / modifyOtherKeys;
- terminal key-release reporting;
- mouse protocols.

Those can extend the same semantic event model when real use cases require them.

### Consequences

- F1–F12 become portable semantic key identities;
- terminal applications can use conventional function-key commands without ANSI-specific code;
- modifier-aware F-key sequences reuse the existing `KeyModifier` model;
- the M2 manual smoke test can validate F1/F10 through the real demo;
- extended keyboard protocols remain independent follow-up work.

---

## Deutsch

### Kontext

Das Terminal-Backend übersetzt Navigations-/Editing-Sequenzen bereits in backendneutrale
`KeyEvent`-Werte. Funktionstasten wurden in ADR 0023 bewusst verschoben, weil das erste `Key`-Modell
ihre Identität noch nicht enthielt.

Für Administrations-TUIs sind Funktionstasten praktisch für stabile Aktionen wie Hilfe, Kontextbefehle
und Beenden. Gleichzeitig sind sie nicht terminalspezifisch: spätere grafische/native Backends erhalten
dieselben physischen Tasten über ihre jeweilige Plattform-API.

### Entscheidung

Das backendneutrale `Key`-Enum erhält `f1` bis `f12`.

`AnsiInputDecoder` bildet die verbreiteten VT-/xterm-Formen darauf ab:

- SS3 `ESC O P` bis `ESC O S` für F1–F4;
- ältere CSI-`11~` bis `14~` für F1–F4;
- CSI `15~`, `17~` … `24~` für F5–F12;
- xterm-Modifier über die bereits vorhandene Modifier-Grammatik, einschließlich
  `CSI 1;<modifier>P..S` und `CSI <function>;<modifier>~`.

Die inkrementelle Dekodierung bleibt unverändert: Sequenzen dürfen über mehrere nichtblockierende Reads
geteilt sein und werden über den bestehenden Decoder-/EventPump-Vertrag vervollständigt bzw. per
Timeout aufgelöst.

Funktionstasten besitzen keine eingebaute Toolkit-Aktion. Die Demo verwendet F1 für Hilfe und F10 zum
Beenden ausschließlich als Anwendungsbeispiel.

### Begründung

Die Identität einer Funktionstaste gehört in das gemeinsame Event-Vokabular; deren Encoding gehört ins
Backend.

Damit bleibt:

```text
Terminalbytes / native Keycodes
             |
          Backend
             |
          KeyEvent
             |
       Anwendung/Widgets
```

Anwendungscode kann also `Key::f1` verwenden, ohne xterm-, Windows-, SDL- oder GTK-Details zu kennen.

### Bewusst später

Nicht festgelegt werden:

- F13 und höher;
- physische Letter-/Digit-Key-Identität;
- Kitty Keyboard Protocol;
- vollständiges CSI-u / modifyOtherKeys;
- terminalseitige Key-Releases;
- Mausprotokolle.

### Konsequenzen

- F1–F12 sind portable semantische Key-Identitäten;
- Terminalanwendungen können klassische Funktionstastenaktionen ohne ANSI-spezifischen Anwendungscode
  anbieten;
- Modifier verwenden das vorhandene `KeyModifier`-Modell;
- der M2-Smoke-Test kann F1/F10 über die reale Demo validieren;
- erweiterte Keyboard-Protokolle bleiben unabhängige Folgeschritte.
