# Optional third-party dependencies

SASD UI Toolkit's Core, Terminal backend and generic `SASD::UI::Rendered` layer do not require SDL.
The experimental M3 SDL3 adapter is opt-in and currently remains a build-tree-only target.

This file records the dependency/license boundary for that experimental path. It is not a bundled
copy of the third-party license texts; source distributions obtained through the optional fetch path
retain the upstream license files supplied by those projects.

| Dependency | Role in the experimental adapter | License / policy |
|---|---|---|
| SDL3 | software rendering substrate now; later candidate for window/input integration | zlib license |
| SDL_ttf | UTF-8 font rasterization, measurement and shaped text/caret information | zlib license |
| FreeType | font engine used by SDL_ttf | FreeType License (FTL), as documented by SDL_ttf |
| HarfBuzz | shaping used by SDL_ttf | MIT license, as documented by SDL_ttf |
| PlutoSVG / PlutoVG | optional SDL_ttf SVG support | MIT; deliberately disabled by the current SASD adapter fetch configuration |

The dedicated adapter CI currently pins SDL `release-3.4.16` and SDL_ttf `release-3.2.2`.
Developers can instead supply compatible installed CMake packages by enabling the adapter without
`SASD_UI_FETCH_SDL3`.

No font binary is shipped by SASD UI Toolkit for this adapter. Tests and applications provide a font
path explicitly. The license and redistribution conditions of that font are therefore separate from
the SASD UI Toolkit and from SDL/SDL_ttf.

Before the SDL3 adapter becomes an installed/exported release component, its redistribution model,
runtime/shared-vs-static linkage, transitive notices and platform packaging will receive a separate
release review.
