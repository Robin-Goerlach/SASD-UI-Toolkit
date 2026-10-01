#pragma once

#include <sasd/ui/container.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <algorithm>
#include <cstddef>

namespace sasd::ui {

/**
 * Overlay layout that gives every visible child the same final client rectangle.
 *
 * StackLayout is intentionally different from VBox/HBox: it does not place children one after
 * another. Visible children occupy the same logical area and therefore form visual layers. Visual
 * adoption order is the layer order already used by Container/PresentationCoordinator: earlier
 * children are below later children, and HitTest consequently treats later children as topmost.
 *
 * The initial M4 contract deliberately has no active-page concept, z-index property, margins,
 * alignment or per-child offsets. A page switcher can be built by changing child visibility, while a
 * future explicit z-order API can be added only after real use demonstrates that adoption order is
 * insufficient. Keeping those policies out of this primitive prevents an overlay container from
 * becoming an accidental window manager.
 */
class StackLayout final : public Container {
protected:
    /**
     * Returns the component-wise maximum desired size of all visible children.
     *
     * Since children overlap rather than consume independent tracks, widths/heights are not summed.
     * Parent minimum constraints are not forwarded to every child; doing so would make each layer
     * claim the complete parent minimum. Each child receives only the parent's maximum as its
     * per-child ceiling, matching the existing Box/Grid measurement convention.
     */
    [[nodiscard]] Size onMeasure(const MeasureConstraints& constraints) override {
        return measureChildren(
            constraints,
            [](Widget& child, const MeasureConstraints& child_constraints) {
                return child.measure(child_constraints);
            });
    }

    /**
     * Context-aware form of the same overlay measurement rule.
     *
     * StackLayout stores no MeasurementContext. It merely forwards the active presentation-specific
     * metric service to visible children, preserving the backend-neutral measure/arrange boundary.
     */
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override {
        return measureChildren(
            constraints,
            [&context](Widget& child, const MeasureConstraints& child_constraints) {
                return child.measure(context, child_constraints);
            });
    }

    /**
     * Stretches every visible child to the complete StackLayout client area.
     *
     * Child coordinates remain parent-relative, so final_bounds.x/y are intentionally not copied into
     * child rectangles. The presentation layer resolves the visual-parent offset chain exactly as it
     * does for VBox, HBox, GridLayout and FormLayout.
     */
    void onArrange(Rect final_bounds) override {
        const Rect client{0, 0, final_bounds.width, final_bounds.height};

        for (std::size_t index = 0; index < childCount(); ++index) {
            Widget& child = childAt(index);
            if (!child.isVisible()) {
                /*
                 * Hidden children collapse from measurement/arrangement just like existing layouts.
                 * Their previous bounds are irrelevant while invisible; making them visible again
                 * invalidates measurement and causes the parent layout to arrange them anew.
                 */
                continue;
            }

            child.arrange(client);
        }
    }

private:
    [[nodiscard]] static MeasureConstraints childConstraints(
        const MeasureConstraints& parent) noexcept {
        return {{0, 0}, parent.maximum};
    }

    template <typename MeasureChild>
    [[nodiscard]] Size measureChildren(const MeasureConstraints& constraints,
                                       MeasureChild&& measure_child) {
        Size desired{};

        for (std::size_t index = 0; index < childCount(); ++index) {
            Widget& child = childAt(index);
            if (!child.isVisible()) {
                continue;
            }

            const Size child_desired =
                measure_child(child, childConstraints(constraints));
            desired.width = std::max(desired.width, child_desired.width);
            desired.height = std::max(desired.height, child_desired.height);
        }

        return desired;
    }
};

} // namespace sasd::ui
