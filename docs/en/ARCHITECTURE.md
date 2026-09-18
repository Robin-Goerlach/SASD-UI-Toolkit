# Architecture

## Overview

SASD UI Toolkit separates the **semantic UI layer** from its concrete presentation. Application code works with `Window`, `Button`, `Label`, layouts, models and events. A backend decides how these concepts are implemented in a target environment.

```text
                     SASD Application
                           │
                     Public UI API
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   Components            Layouts            Models
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                      Events/Commands
                           │
                         Toolkit
                           │
                    Backend/Peer API
                           │
       ┌───────────────────┼────────────────────┐
       │                   │                    │
 Native Desktop       Rendered Desktop        Terminal
 Win32/GTK/AppKit        e.g. SDL3          ANSI/VT/Console
```

## Core architectural idea

The public UI API must not depend on a concrete platform. At the same time, the project should not pretend that every platform is identical.

This leads to three rules:

1. **Unify semantics.** A button remains a button regardless of backend.
2. **Allow different presentation.** A native Windows button and a terminal button do not have to look identical.
3. **Expose capabilities explicitly.** Platform differences are queried rather than hidden.

## Component model

Inspired by VCL and AWT, the architecture distinguishes generic components from visible widgets.

```text
Component
├── Command
├── Action
├── Timer
├── DataSource
└── Widget
    ├── Label
    ├── Button
    ├── CheckBox
    ├── TextField
    └── Container
        ├── Panel
        ├── ScrollView
        └── Window
            └── Dialog
```

`Component` is not necessarily visible. This leaves room for actions, commands, timers and data sources to share lifecycle and ownership concepts with visual components.

### Ownership

Initial core rules:

- a `Container` owns its children;
- RAII and explicit ownership should represent lifetime relationships;
- non-owning references must not silently extend lifetimes;
- callback connections must be safely disconnectable when either side is destroyed.

The exact C++ API will be validated by the first core prototype. Raw owning pointers are not a design goal.

## Toolkit and backend

The `Toolkit` bridges the semantic widget tree and platform implementation. It is responsible for global services such as:

- creation and destruction of top-level windows;
- event pump / event loop;
- backend capability discovery;
- clipboard and cursor services;
- theme and metric access;
- platform integration.

A backend may implement widgets in two ways.

### Native peers

A SASD widget is mapped to an operating-system-native control.

Examples:

- `Button` → Win32 button
- `Button` → GTK button
- `Button` → AppKit button

### Rendered peers

The backend draws the widget itself onto a drawing surface.

This is important for:

- terminal backends;
- SDL-based backends;
- widgets without a native equivalent;
- specialized controls that need consistent custom rendering.

A backend may mix both strategies.

## BackendCapabilities

Platform differences should be explicit. A backend reports capabilities such as:

```cpp
struct BackendCapabilities {
    bool mouse;
    bool clipboard;
    bool dragAndDrop;
    bool nativeMenus;
    bool multipleWindows;
    bool trueColor;
    bool accessibility;
};
```

This is conceptual, not yet a stable API.

## Event model

The event model is backend-neutral. Raw platform input is translated into semantic events.

Planned event families include:

- keyboard;
- text input;
- pointer/mouse;
- focus;
- activation/click;
- window/lifecycle;
- resize;
- command/action.

**Key events and text input are intentionally separate.** A character is not the same thing as a physical key; international keyboards, IMEs and accessibility depend on that distinction.

The initial threading model assumes a single UI thread with an event queue. Background work should explicitly dispatch results back to the UI thread rather than making all widgets arbitrarily thread-safe.

### Routing and focus in the current M1 core

The implemented M1 core deliberately separates event pumping, target selection, routing and focus:

```text
Backend
   │
   ▼
Application
   │
   ├── QuitEvent → application lifecycle
   │
   └── semantic input event
             │
             ▼
      EventTargetResolver
             │
             ▼
        target Widget
             │
             ▼
      EventDispatcher
             │
             └── Target → Parent → ... → Root
```

`EventDispatcher` performs synchronous bubbling along the **visual parent path**. A widget may consume an event with `EventResult::handled`; `ignored` offers the same event to the visual parent. Ownership relationships to non-visual components are intentionally not part of this routing path.

`FocusManager` is a separate target-selection component for logical keyboard focus. It does not own widgets; it holds exactly one non-owning observation of the currently focused widget. Widget and FocusManager detach that observation from either destruction path so no dangling pointer remains.

Widgets are **not focusable by default**. In the M1 core, local focus eligibility is `focusable && visible && enabled`. Inherited state from future `Window`/container focus-scope rules is deliberately deferred until real widgets and backends can validate those semantics.

A `FocusEvent` generated by `FocusManager` is a direct state notification to the affected widget. The focus state has already changed when the handler runs. The notification is not vetoable and is not treated as ordinary input bubbling. Normal keyboard and text input continues to use `EventDispatcher`.

## Layout

Layouts should not assume pixels. Terminals use cells, while desktop systems use logical units and font metrics.

### Measure/arrange contract in the M1 core

The implemented core uses a two-phase, backend-neutral layout lifecycle:

```text
Parent
  │
  ├── measure(MeasureConstraints)
  │        │
  │        ▼
  │   onMeasure(...)
  │        │
  │        ▼
  │   desiredSize
  │
  └── arrange(final Rect)
           │
           ▼
      bounds are stored
           │
           ▼
      onArrange(...)
```

`SizeConstraints` belong to the widget and describe its minimum, preferred and maximum size hints. `MeasureConstraints` come from the parent and describe only the minimum/maximum interval actually offered. The intrinsic result from `onMeasure()` is constrained first by the widget's own hints and then by the parent's constraints, so a child cannot manufacture space that the parent does not have.

`measure()` caches `desiredSize()` for identical parent constraints. Size-relevant state changes call `invalidateMeasure()`, which propagates along the visual parent path. A future text change in a `Label`, for example, can therefore mark its `VBox` and `Window` ancestors stale as well. Adding or removing visual children also invalidates the container.

`arrange()` assigns the final logical rectangle. Bounds are stored before `onArrange()` runs so a container can use its own final geometry while arranging children. Zero-sized rectangles are valid; negative extents are rejected. Direct `setBounds()` intentionally follows the same arrangement path.

Concrete `VBox`/`HBox` allocation, margin/padding/spacing APIs, the layout effect of invisible widgets and backend-specific text measurement remain deliberately open until the first real M2 controls exercise those cases.

The layout process should therefore work with concepts such as:

- minimum size;
- preferred size;
- maximum size;
- available constraints;
- padding, margins and spacing;
- backend-specific text and widget measurement.

Initial planned layouts:


- `HBox`
- `VBox`
- `GridLayout`
- `FormLayout`
- `StackLayout`
- optional `AbsoluteLayout` for exceptional cases

Absolute positioning should be possible but not the default model.

## Model/View for data-heavy widgets

Lists, tables and trees should eventually avoid copying all data into UI objects. A model/view separation inspired by Qt and other mature UI frameworks is planned.

```text
Application Data
      │
    Model
      │
     View
      │
 Delegate/Renderer/Editor
```

A `TableView`, for example, should be able to virtualize large datasets instead of creating millions of per-cell UI objects.

## Commands and actions

A command represents an invokable operation independently from the widget that triggers it. The same command can later be connected to a menu item, toolbar button, keyboard shortcut or command palette.

Planned properties include:

- label/text;
- enabled/disabled state;
- checked state where applicable;
- shortcut;
- execution callback.

This avoids duplicating application logic between menus, buttons and keyboard handling.

## Rendering

The core does not mandate a specific renderer. Rendered backends either receive an abstract drawing/surface layer or fully encapsulate their rendering technology behind the backend.

An optional SDL3 backend is attractive as an early graphical proof of concept because it can provide a cross-platform window/input foundation without exposing SDL in the public SASD API.

## Text and Unicode

Unicode must be considered from the beginning. UTF-8 is the preferred starting point at public API boundaries. Platform conversions, such as UTF-16 on Windows, belong in the backend.

Terminal rendering must eventually handle display widths, combining characters and grapheme clusters. `v0.1.0` may support a narrower subset, but the architecture must not permanently assume that one byte equals one character equals one terminal cell.

## Accessibility

Accessibility is an architectural concern, not a late add-on. Native backends can use platform services; rendered backends will eventually require semantic accessibility bridges. The core should therefore preserve semantic roles, names, states and actions independently from visual rendering details.

## Public API versus backend API

The separation is strict:

```text
include/sasd/ui/...              public API
src/core/...                     platform-neutral core
src/backends/terminal/...        terminal backend
src/backends/sdl/...             rendered desktop backend
src/backends/win32/...           native Windows backend
src/backends/gtk/...             native Linux backend
src/backends/appkit/...          native macOS backend
```

The exact source tree will be created with the first implementation milestone once the minimal interfaces have been validated.

## Stability rule

During `0.x`, API changes are allowed. They should still be explained, documented and migration-friendly where practical. A formal API/ABI policy becomes necessary before a future stable release.
