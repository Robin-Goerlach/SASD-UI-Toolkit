#pragma once

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/presentation/anchored_popup_layout.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui::rendered {

/**
 * One owned item inside a rendered ComboBox popup presentation snapshot.
 *
 * Text bytes and their measured logical size are captured together. Later rendering therefore does not
 * retain a ComboBox string_view or a RenderedMeasurementContext pointer, and it does not have to repeat
 * font measurement merely to replay the already-decided popup geometry.
 */
struct RenderedComboBoxPopupItemPresentation {
    std::string text;
    Size text_size{};

    friend bool operator==(const RenderedComboBoxPopupItemPresentation&,
                           const RenderedComboBoxPopupItemPresentation&) = default;
};

/**
 * Owned rendered presentation state for one currently open ComboBox popup.
 *
 * bounds is the complete outer popup including optional theme border. content_bounds is the exact
 * fixed-row area inside that border. Every item occupies row_height logical units and receives
 * horizontal/vertical padding derived from the rendered theme at snapshot construction time.
 *
 * The snapshot deliberately retains no Widget, ComboBox, font-service or device pointer. It is a value
 * boundary between semantic state/measurement and DisplayList generation, mirroring the terminal
 * ComboBox popup's owned snapshot while preserving rendered logical units.
 *
 * An open ComboBox with zero items is represented by empty bounds/content_bounds and an empty item list.
 * No artificial "(empty)" row is manufactured.
 */
struct RenderedComboBoxPopupPresentationSnapshot {
    Rect bounds{};
    Rect content_bounds{};
    presentation::PopupVerticalSide side{
        presentation::PopupVerticalSide::below};
    std::vector<RenderedComboBoxPopupItemPresentation> items{};
    std::optional<std::size_t> preview_index{};
    TextStyle text_style{};
    Coordinate row_height{0};
    Coordinate padding{0};
    Coordinate border_thickness{0};
};

/**
 * Builds one immutable rendered ComboBox popup presentation snapshot.
 *
 * absolute_anchor and viewport must already use the same top-level logical coordinate space. The
 * builder intentionally does not walk Widget parents: absolute geometry resolution remains a host/
 * presentation-tree responsibility.
 *
 * Measurement policy:
 * - one fixed row height shared by every item;
 * - row content height is at least the active positive line height and at least every measured item;
 * - one theme-derived padding inset surrounds item text inside every row;
 * - the complete popup uses the normalized control border;
 * - popup width is at least the collapsed ComboBox anchor width;
 * - generic ADR 0122 anchored placement chooses below/above without clipping.
 *
 * The semantic open/focus/visible/enabled invariant is revalidated before copying state. Malformed
 * metrics, stale preview identity, arithmetic overflow or inability to fit the complete popup returns
 * std::nullopt before any DisplayList is touched.
 */
[[nodiscard]] std::optional<RenderedComboBoxPopupPresentationSnapshot>
buildComboBoxPopupPresentation(
    const ComboBox& combo,
    Rect absolute_anchor,
    Rect viewport,
    const RenderedMeasurementContext& metrics,
    presentation::PopupVerticalSide preferred_side =
        presentation::PopupVerticalSide::below);

/**
 * Appends one already-built popup snapshot to a DisplayList.
 *
 * Complete structural/coordinate preflight runs before the first command is appended. A false return
 * therefore leaves display_list unchanged. Preview is drawn with a row outline plus toggled inverse
 * text style. Toggling, rather than forcing inverse=true, keeps preview distinguishable when the
 * application-provided ComboBox style is already inverse.
 *
 * background_color is host/surface policy. The popup fills its complete outer bounds with that color
 * before drawing its border and rows, so it behaves as an opaque transient overlay.
 */
[[nodiscard]] bool renderComboBoxPopupPresentation(
    DisplayList& display_list,
    const RenderedComboBoxPopupPresentationSnapshot& snapshot,
    Color background_color = Color::default_color);

/**
 * Composes an open ComboBox popup over an immutable rendered base command list.
 *
 * Closed ComboBoxes return a value-preserving copy of base. Open state is built and validated first,
 * then base is copied and popup commands are appended to the copy. Any placement/geometry failure
 * returns std::nullopt and leaves base untouched.
 *
 * This correctness-first value composition is intentionally simple. A later retained-frame/command-
 * arena optimization can avoid copies without changing semantic/presentation contracts.
 */
[[nodiscard]] std::optional<DisplayList>
composeComboBoxPopupDisplayList(
    const DisplayList& base,
    const ComboBox& combo,
    Rect absolute_anchor,
    Rect viewport,
    const RenderedMeasurementContext& metrics,
    Color background_color = Color::default_color,
    presentation::PopupVerticalSide preferred_side =
        presentation::PopupVerticalSide::below);

} // namespace sasd::ui::rendered
