# Public API navigation and ownership

Use this as a map, **not** a frozen declaration of every method. For each class, inspect its actual public header in the selected release, then tests and demo usage.

## Core (`SASD::UI` CMake target)

| Concern | Start with |
| --- | --- |
| Root, owned components | `include/sasd/ui/window.hpp`, `container.hpp`, `component.hpp` |
| Widget state and layout | `widget.hpp`, `geometry.hpp`, `measurement_context.hpp` |
| Form layout | `vbox.hpp`, `hbox.hpp`, `form_layout.hpp`, `grid_layout.hpp` |
| Form controls | `label.hpp`, `button.hpp`, `text_field.hpp`, `check_box.hpp`, `radio_button.hpp`, `combo_box.hpp` |
| Actions and menus | `command.hpp`, `menu_model.hpp`, `shortcut.hpp` |
| Input/focus | `application.hpp`, `events/event.hpp`, `focus_manager.hpp`, `focus_traversal.hpp`, `pointer_router.hpp` |
| Data views | `list_model.hpp`, `list_view.hpp`, `table_model.hpp`, `table_view.hpp`, `tree_model.hpp`, `tree_view.hpp` |

Do not confuse `Container::emplace<T>()` ownership with manually managed raw pointers. A Container owns components; only `Widget` instances participate in the visible child tree. References into an owned tree expire when elements are removed.

### Model/View notes

- Models own data; views retain non-owning references and produce snapshots for visible rows.
- Selection is distinct from keyboard focus and from application record identity.
- Positional row and tree indices must not be used as stable resource IDs after model mutations.
- The current toolkit does **not** offer a generic spreadsheet editor or a finished cross-backend async application host merely because TableView/Application headers exist.

## Where to confirm behavior

- `include/sasd/ui/`: exact public API and namespaces.
- `src/`: implemented behavior, especially callback and invalidation paths.
- `tests/`: executable contracts for state, mutation, event delivery and lifetimes.
- `examples/terminal_form_demo.cpp`: end-to-end terminal host and working C++ call sites.
- `docs/adr/`: architectural decisions, not per-version executable guarantees.
- `README.md` and `docs/en/`: high-level status; check that claims match the selected version.

If the selected consumer only has an installed package, use **its installed headers**, CMake package files and explicit version, not files from an unrelated `main` branch.

## Safe semantic patterns

1. Instantiate models and commands in a scope that outlives their non-owning UI references.
2. Create visual controls with their intended owner (usually `window.emplace<...>()` or nested container).
3. Prefer `Command` for the same semantic operation triggered by multiple controls.
4. Use `setEnabled` for UX feedback, but enforce authorization in the underlying service.
5. Make callback captures valid throughout registration lifetime; a callback may remove its initiating widget.
6. Move results from background work into the application's designated UI event path; widgets are not an arbitrary multi-threaded data model.

Read `examples/core-consumer/main.cpp` within this skill for a small compiled Core demonstration. For a real terminal application, Core code alone is insufficient.
