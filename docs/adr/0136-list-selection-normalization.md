# ADR 0136 – Initial single-selection normalization for ListView

**Status:** Accepted  
**Date:** 2026-10-08

## English

`ListSelectionModel` is a small, single-row semantic object separate from `FocusManager`. It observes
its non-owning `ListModel::Reference`, so model mutations are normalized at the semantic boundary rather
than duplicated by every presentation backend. Insertion before a surviving selection shifts its
position; removal containing the selection clears it; removal before it shifts it backward; reset clears
it. Invalid or unrepresentable positions fail closed. A model may be unbound with no selection.

`ListView` observes both model and selection state, but owns neither. It handles only unmodified,
pressed Up/Down/Home/End events while focused and enabled. Keyboard focus does not imply selection, and
selection does not grant focus. The initial viewport is a logical row range, not pixel geometry, and
`visibleRows()` copies only that range into owned values. Page navigation, scrolling infrastructure,
multi-selection and stable item identity remain deferred.

## Deutsch

`ListSelectionModel` ist ein kleines semantisches Objekt für genau eine Zeilenselektion und bleibt vom
`FocusManager` getrennt. Es beobachtet eine nicht-besitzende `ListModel::Reference`, sodass
Modelländerungen an einer semantischen Stelle normalisiert werden. Eine Einfügung vor einer erhaltenen
Selektion verschiebt deren Position; eine Entfernung der selektierten Zeile löscht sie; eine Entfernung
davor verschiebt sie zurück; ein Reset löscht sie. Ungültige oder nicht darstellbare Positionen werden
sicher verworfen. Ein Selection Model darf ungebunden sein.

`ListView` beobachtet Modell und Selection, besitzt aber keines von beiden. Es verarbeitet zunächst nur
unmodifizierte gedrückte Up/Down/Home/End-Ereignisse bei Fokus und Aktivierung. Fokus bedeutet nicht
automatisch Selektion und Selektion erteilt keinen Fokus. Der erste Viewport ist ein logischer
Zeilenbereich statt Pixelgeometrie; `visibleRows()` kopiert ausschließlich diesen Bereich in besitzende
Werte. Page-Navigation, allgemeine Scroll-Infrastruktur, Multi-Selection und stabile Item-Identität
bleiben bewusst zurückgestellt.
