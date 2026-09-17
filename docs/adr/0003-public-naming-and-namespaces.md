# ADR 0003 – Public naming and namespaces

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

Vendor-prefixed class names such as `SasdWindow`, `SasdButton` or `SasdObject` are verbose in everyday code. Classic frameworks demonstrate that the framework or vendor name does not need to be repeated in every type name: VCL uses names such as `TObject`/`TButton`, while AWT and Swing use `Button`, `Component`, `JButton`, etc.

Modern C++ already provides namespaces for collision avoidance and library identity.

### Decision

- Public types do **not** use `SASD`, `Sasd` or another vendor prefix.
- Library identity is expressed through namespaces, primarily `sasd::ui`.
- Public type names use concise PascalCase nouns such as `Window`, `Button`, `Label`, `Component`, `Widget`, `Menu`, `Command` and `TableModel`.
- A universal `Object` base class will **not** be introduced merely to imitate VCL or Java. It may only be introduced later if a concrete technical requirement justifies it.
- Internal implementation types should prefer namespaces and descriptive names over Hungarian-style or vendor prefixes.

### Examples

Preferred:

```cpp
sasd::ui::Window window;
sasd::ui::Button okButton{"OK"};
```

Local shortening is possible without changing the public API:

```cpp
namespace ui = sasd::ui;
ui::Window window;
ui::Button okButton{"OK"};
```

Not preferred:

```cpp
SASDWindow window;
SasdButton button;
SasdObject object;
```

### Consequences

The API stays readable in normal application code while remaining unambiguous when fully qualified. The project avoids baking branding into every source line.

## Deutsch

### Kontext

Klassenbezeichnungen wie `SasdWindow`, `SasdButton` oder `SasdObject` wären im täglichen Code unnötig lang. Die VCL hieß nicht `BorlandObject`/`BorlandButton`, und auch AWT bzw. Swing stellen den Herstellernamen nicht vor jede Klasse. In modernem C++ übernehmen Namespaces die Aufgabe, Bibliothekszugehörigkeit und Namenskollisionen zu lösen.

### Entscheidung

- Öffentliche Typen erhalten **kein** `SASD`-/`Sasd`-Präfix.
- Die Bibliothekszugehörigkeit wird über den Namespace `sasd::ui` ausgedrückt.
- Klassen heißen beispielsweise `Window`, `Button`, `Label`, `Component`, `Widget`, `Menu`, `Command` oder `TableModel`.
- Ein universeller Basistyp `Object` wird nicht nur deshalb eingeführt, weil VCL oder Java einen solchen Ursprung besitzen. Dafür müsste es einen konkreten technischen Nutzen geben.

Damit ist `sasd::ui::Window` die vollständige öffentliche Bezeichnung; im lokalen Anwendungscode kann sie bei Bedarf über einen Namespace-Alias auf `ui::Window` verkürzt werden.