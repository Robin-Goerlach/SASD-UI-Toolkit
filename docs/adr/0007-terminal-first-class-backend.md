# ADR 0007 – Terminal as a first-class backend

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

A core project goal is to use the same high-level component model not only on Windows, Linux and macOS desktops but also in terminal environments such as VT/xterm-style terminals and modern Windows consoles. A terminal is not a low-resolution graphical display: it has different input, measurement, color and windowing capabilities.

### Decision

Terminal support is a **first-class backend**, not a later renderer hack.

- Shared components keep semantic meaning across desktop and terminal backends.
- Layout uses abstract measurements and constraints; it must not assume that every backend measures in pixels.
- Focus, commands, keyboard navigation and activation semantics are shared where meaningful.
- Physical key events and text input are separate concepts so terminal escape sequences, IME/text input and desktop key handling do not collapse into one ambiguous event.
- Terminal capabilities are detected explicitly: color depth, mouse reporting, Unicode support, clipboard integration and similar features may vary.
- Terminal-specific presentation differences are allowed when they preserve the component's intent.

### Rationale

Treating the terminal as a peer backend from the beginning tests whether the public API truly expresses UI semantics rather than desktop graphics assumptions. It also creates a distinctive use case for administration, engineering and remote environments.

### Consequences

- Pixel coordinates cannot be the universal public layout abstraction.
- Widgets must not assume a native OS handle exists.
- Accessibility and keyboard navigation semantics should be designed above backend rendering.

## Deutsch

### Kontext

Ein Kernziel ist, dass dasselbe semantische Komponentenmodell nicht nur auf Windows-, Linux- und macOS-Desktops, sondern auch in VT-/xterm-artigen Terminals und modernen Windows-Konsolen funktioniert. Ein Terminal ist jedoch keine grafische Oberfläche mit schlechten Pixeln, sondern besitzt andere Mess-, Eingabe-, Farb- und Windowing-Eigenschaften.

### Entscheidung

Terminal-Unterstützung ist ein **gleichwertiges Backend** und kein später angehängter Renderer.

Layout arbeitet mit abstrakten Maßen und Constraints statt überall Pixel vorauszusetzen. Fokus, Commands und Navigation werden soweit sinnvoll gemeinsam modelliert. Physische Tastenereignisse und Texteingabe werden getrennt behandelt. Fähigkeiten wie Farbe, Maus, Unicode oder Clipboard werden über Backend-Capabilities ausgedrückt.

Terminal-spezifische Darstellung ist erlaubt, solange die semantische Bedeutung einer Komponente erhalten bleibt.

### Konsequenz

Die Terminal-Unterstützung dient gleichzeitig als Architekturtest: Wenn eine öffentliche API nur mit Pixeln, nativen Handles oder Mausinteraktion funktioniert, ist sie wahrscheinlich zu stark an Desktop-GUIs gekoppelt.