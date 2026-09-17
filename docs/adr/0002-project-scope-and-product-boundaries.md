# ADR 0002 – Product scope and boundaries

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

Earlier project discussions ranged from an open-source VCL-like component library to a long-term Qt-class cross-platform framework. Trying to implement the full breadth of a mature framework immediately would delay the first useful release and make architectural mistakes expensive.

### Decision

SASD UI Toolkit is an **independent modern C++ UI framework**, not a source-compatible clone of VCL, AWT, Swing, Qt or wxWidgets.

The long-term ambition may grow toward the breadth expected from a serious cross-platform desktop toolkit, but early releases deliberately focus on a small coherent foundation.

The toolkit core owns:

- component and widget semantics;
- retained component trees;
- events and commands;
- layout;
- focus and input abstractions;
- backend contracts and capabilities;
- model/view foundations;
- cross-platform UI semantics.

The following are outside the initial core scope:

- a full IDE;
- a RAD Studio replacement;
- a visual form designer;
- a complete 2D/3D graphics engine;
- a browser engine;
- mobile and web targets;
- feature parity with Qt or VCL in the first releases.

A designer may later become a separate sister project that consumes stable toolkit metadata and serialization APIs.

### Rationale

A small, testable core makes it possible to validate the architecture on more than one backend before accumulating a large widget catalogue. The project should earn breadth by extending a stable model rather than by copying the surface area of an existing framework.

## Deutsch

### Kontext

Frühere Diskussionen reichten von einer freien VCL-artigen Komponentenbibliothek bis zu einem langfristig umfangreichen plattformübergreifenden Framework auf dem Niveau etablierter Produkte. Würden wir diesen Gesamtumfang sofort anstreben, käme ein erstes brauchbares Release zu spät und frühe Architekturfehler würden teuer.

### Entscheidung

Das SASD UI Toolkit ist ein **eigenständiges modernes C++-UI-Framework** und kein quellkompatibler Klon von VCL, AWT, Swing, Qt oder wxWidgets.

Langfristig darf das Projekt zu einem umfangreichen plattformübergreifenden Desktop-Toolkit wachsen. Frühe Releases konzentrieren sich jedoch bewusst auf einen kleinen, in sich geschlossenen Kern.

Zum Core gehören Komponenten- und Widget-Semantik, Component Tree, Events und Commands, Layout, Fokus und Input, Backend-Verträge und Capabilities sowie Model/View-Grundlagen.

Nicht zum frühen Core gehören insbesondere eine vollständige IDE, ein RAD-Studio-Ersatz, ein visueller Form-Designer, eine komplette Grafikengine, ein Browser-Stack sowie sofortige Feature-Parität mit Qt oder VCL.

Ein visueller Designer kann später als **eigenständiges Schwesterprojekt** entstehen und stabile Metadaten- und Serialisierungsfunktionen des Toolkits verwenden.