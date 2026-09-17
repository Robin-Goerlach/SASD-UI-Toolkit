# ADR 0010 – Release and compatibility strategy

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

The project should reach a useful first release quickly, but a UI framework can easily become trapped by premature compatibility promises. Earlier discussions also emphasized small demonstrable slices rather than a broad but shallow component list.

### Decision

- Releases before 1.0 are **architecture-validation releases** and may make source-breaking changes when necessary.
- Every release should deliver a tested vertical slice, not merely add names to a component catalogue.
- M1 establishes the core plus headless/mock backend.
- The first user-visible release (`v0.1.0`) targets the terminal backend and a small coherent widget set.
- A later rendered desktop preview demonstrates the same public API on Windows, Linux and macOS before native desktop peers are expanded.
- Stable source-compatibility guarantees and ABI commitments are deferred until the architecture has survived multiple backends and real applications.
- Example/reference applications should exercise realistic application flows and become part of regression/conformance testing where practical.

### Rationale

A portable UI abstraction is only credible after it survives more than one implementation environment. Compatibility promises made before that point would preserve mistakes rather than protect users.

### Consequences

- `v0.x` release notes must clearly call out breaking changes.
- The project should prefer removing a flawed abstraction early over carrying it indefinitely.
- Release quality is measured by tested behavior across supported backends, not by widget count.

## Deutsch

### Kontext

Das Projekt soll möglichst früh ein brauchbares Release erreichen. Gleichzeitig kann sich ein UI-Framework durch zu frühe Kompatibilitätsversprechen dauerhaft an falsche APIs binden. In den älteren Diskussionen war deshalb bereits ein kleiner, demonstrierbarer Funktionsschnitt wichtiger als eine lange Liste halbfertiger Komponenten.

### Entscheidung

- Releases vor 1.0 dienen der **Architekturvalidierung** und dürfen bei Bedarf Source-Breaking-Changes enthalten.
- Jedes Release soll einen getesteten vertikalen Schnitt liefern.
- M1 enthält Core und Headless-/Mock-Backend.
- `v0.1.0` wird das erste für Anwender sichtbare Release mit Terminal-Backend und kleinem konsistentem Widget-Satz.
- Ein danach folgender gerenderter Desktop-Prototyp soll dieselbe öffentliche API unter Windows, Linux und macOS beweisen, bevor native Peers breit ausgebaut werden.
- Stabile Source- und ABI-Garantien werden erst zugesagt, wenn die Architektur mehrere Backends und reale Anwendungen überstanden hat.

### Konsequenzen

Breaking Changes in `v0.x` müssen klar dokumentiert werden. Ein falsches Konzept soll lieber früh entfernt werden, statt aus Angst vor vorzeitiger Kompatibilität jahrelang mitgeführt zu werden.