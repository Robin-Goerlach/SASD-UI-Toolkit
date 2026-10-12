# SASD UI Toolkit — LaTeX User Guide

The **German** user guide is original, modular LaTeX. Its didactic order
(orientation, first working example, concepts, practical use and reference)
takes inspiration from the classic Borland manuals, **without** copying
their protected text, drawings, or sample programs.

## Build

Requirements: LuaLaTeX, latexmk, and ordinary TeX Live packages
(KOMA-Script, fontspec, babel with German support, tcolorbox, listings,
tikz, hyperref, booktabs, tabularx, microtype, enumitem, scrlayer-scrpage).

Run from this directory:

~~~sh
make pdf
# alternatively:
latexmk -lualatex -interaction=nonstopmode -halt-on-error -outdir=build main.tex
~~~

Output: **build/main.pdf**. Do not check generated artifacts into Git.

## Structure and translation workflow

~~~text
docs/manual/
├── main.tex                  # language-independent book assembly
├── style/sasdmanual.sty      # typography, layout, code and callouts
├── i18n/de.tex               # German common strings and metadata
├── i18n/en.tex               # English common strings (scaffold)
├── content/de/               # Complete German manuscript, 10 chapters + appendix
└── Makefile
~~~

For another language, create every corresponding chapter in
**content/LOCALE/**, translate *narrative text* while keeping the C++ API
identifiers, code paths, labels and examples accurate, and provide
**i18n/LOCALE.tex**. Then set the chosen locale using the
**SASDLanguage** macro before inputting main.tex. For example, an
English entry file may set the macro to en and input main.tex.

**Important:** The English i18n metadata is present, but the actual English
chapters have **not** been translated. The book does not silently mix
untranslated chapters into an English edition. Keep chapter structure and
opening summaries aligned across editions; translations can be reviewed and
released independently.

## Editorial conventions

- Verify every API detail against **include/sasd/ui/** and tests, not only old README prose.
- Explain the distinction between Core, completed terminal path, rendered
  foundation, experimental SDL3 adapter, and *planned* native peers.
- All chapters open with **ChapterSummary**; follow with detailed guidance,
  code excerpts, constraints, and practical advice.
- Code snippets that omit the host loop are explicitly described as excerpts.
  See **examples/terminal_form_demo.cpp** for the complete working host.
- Do not reproduce proprietary Borland material.
- Treat pre-1.0 API changes as documentation update triggers.
- Use UTF-8 with LuaLaTeX. Do not commit fonts, PDFs or build caches.

See the repository [documentation index](../README.md).
