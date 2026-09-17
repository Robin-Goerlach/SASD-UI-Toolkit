# ADR 0006 – Headless/mock backend before platform backends

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

UI frameworks are difficult to test when every behavior depends on a real window system, GPU, terminal or operating-system event loop. Earlier SASD Surface discussions therefore proposed a mock renderer/backend before committing to a concrete GUI backend.

### Decision

A deterministic **headless/mock backend** is part of the M1 core foundation and precedes the terminal and graphical production backends.

The mock backend should make it possible to test at least:

- component-tree creation and destruction;
- ownership and parent/child relationships;
- event dispatch, routing and ordering;
- focus transitions;
- layout measurement and arrangement;
- backend capability negotiation;
- semantic render/update requests;
- backend contract behavior without a display server.

The mock backend is test infrastructure, not a user-facing UI target.

### Rationale

If the core can only be exercised through a real terminal or desktop backend, backend bugs and core bugs become difficult to separate. A headless backend forces the contracts to be explicit and gives CI fast, deterministic tests on all platforms.

### Consequences

- M1 is not complete merely because headers compile; core behavior must be executable against the mock backend.
- Terminal becomes the first **user-visible** backend, but not the first backend implementation.
- Backend conformance tests should be reusable by later terminal, SDL/rendered and native backends.

## Deutsch

### Kontext

UI-Frameworks sind schwer zuverlässig zu testen, wenn jedes Verhalten ein echtes Fenstersystem, eine GPU, ein Terminal oder einen betriebssystemspezifischen Event Loop voraussetzt. In den älteren SASD-Surface-Überlegungen war deshalb bereits ein Mock-Renderer bzw. Mock-Backend vorgesehen.

### Entscheidung

Ein deterministisches **Headless-/Mock-Backend** gehört verbindlich zu M1 und wird vor den produktiven Terminal- und GUI-Backends implementiert.

Damit sollen Component Tree und Ownership, Event-Reihenfolge, Fokus, Layout, Capabilities sowie die Backend-Verträge ohne reale Anzeige testbar sein.

Das Mock-Backend ist keine Benutzeroberfläche für Endanwender, sondern Testinfrastruktur.

### Konsequenzen

Das Terminal bleibt das erste **sichtbare und nutzbare** Backend, ist aber nicht die erste Backend-Implementierung. Spätere Backends sollen möglichst dieselben Contract-Tests durchlaufen.