# Inspirations and References

SASD UI Toolkit is **not a clone** of any single framework. The following projects are studied as sources of architectural experience. Ideas are evaluated and re-expressed in an independent C++ design; source code and implementation details from other projects must not be copied without understanding their licenses and implications.

## Delphi VCL

**Useful lessons:**

- separation between generic components and visible controls;
- ownership/component model;
- properties and events;
- productive component-oriented application development;
- design-time thinking.

**Do not inherit blindly:**

- Windows-centric architecture;
- Object Pascal-specific conventions;
- historical API baggage.

Reference:

- Embarcadero RAD Studio Libraries: https://docwiki.embarcadero.com/Libraries/

## Java AWT

AWT is an especially important reference for a common widget abstraction over platform-specific implementations.

**Useful lessons:**

- `Component` / `Container`;
- event queue;
- layout managers;
- toolkit/peer concept;
- native/heavyweight components.

**Do not inherit blindly:**

- Java-specific API and legacy details;
- deprecated event models.

References:

- Component: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/java/awt/Component.html
- Container: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/java/awt/Container.html
- LayoutManager: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/java/awt/LayoutManager.html

## Java Swing

Swing adds primarily lightweight widgets and pluggable presentation on top of AWT concepts.

**Useful lessons:**

- lightweight widgets;
- UI delegate / look-and-feel concept;
- rich component models;
- model concepts for lists, tables and trees.

**Do not inherit blindly:**

- the complete complexity of Swing;
- Java serialization as the persistence model for UI definitions.

Reference:

- JComponent: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/javax/swing/JComponent.html

## Qt

Qt is an important example of a mature and successful C++ application framework.

**Useful lessons:**

- event loop;
- layout system;
- Model/View/Delegate;
- platform abstraction;
- experience with large widget sets and desktop integration.

**Do not inherit blindly:**

- Qt as a mandatory dependency;
- a MOC/meta-object mechanism as a requirement for normal SASD application code;
- an unnecessarily large framework surface in early releases.

References:

- Qt Documentation: https://doc.qt.io/qt-6/
- Model/View Programming: https://doc.qt.io/qt-6/model-view-programming.html

## wxWidgets

wxWidgets is particularly relevant because it combines a cross-platform C++ API with native controls.

**Useful lessons:**

- native look-and-feel strategy;
- port/backend structure;
- practical C++ portability across Windows, Linux and macOS.

**Do not inherit blindly:**

- wxWidgets as a mandatory implementation layer;
- historical API conventions maintained only for compatibility.

References:

- Overview: https://wxwidgets.org/about/
- Documentation: https://docs.wxwidgets.org/

## FLTK

FLTK is useful as an example of a relatively small and pragmatic C++ GUI toolkit.

**Useful lessons:**

- modest complexity;
- fast builds;
- limited dependency surface;
- pragmatic widget architecture.

Reference:

- https://www.fltk.org/

## SDL3

SDL3 is not a complete GUI toolkit, which is precisely why it is interesting as an optional foundation for a custom rendered backend.

**Useful lessons:**

- windows and input;
- cross-platform foundation;
- permissive licensing;
- clean separation between the SASD widget API and the rendering substrate.

References:

- https://libsdl.org/
- https://wiki.libsdl.org/SDL3/FrontPage

## FTXUI

FTXUI is a modern C++ project for interactive terminal user interfaces.

**Useful lessons:**

- compositional/declarative TUI ideas;
- separation of components, layout and screen;
- cross-platform terminal rendering.

Reference:

- https://github.com/ArthurSonzogni/FTXUI

## Turbo Vision

Turbo Vision is both historically and currently relevant: object-oriented, component-based text-mode user interfaces.

**Useful lessons:**

- windows, dialogs and menus in terminals;
- focus and event handling;
- proof that sophisticated TUI applications can use component-oriented design.

Reference:

- https://github.com/magiblot/tvision

## Study principle

For important architecture questions, ask:

1. How does AWT solve the abstraction problem?
2. How does wxWidgets integrate native platforms?
3. How does Qt model large data and complex widgets?
4. What made VCL productive and approachable?
5. How do FTXUI and Turbo Vision solve equivalent problems in terminals?
6. What is the cleanest modern C++20 answer without inheriting unrelated legacy constraints?

The result must fit SASD UI Toolkit, not the reference framework.
