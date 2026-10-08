#pragma once

#include <sasd/ui/presentation/presentation_sink.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/list_view_presentation.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

namespace sasd::ui::rendered {

/**
 * Translates semantic Widget presentation into backend-neutral rendered drawing commands.
 *
 * This is deliberately not an SDL sink. It knows only SASD widgets, logical geometry and DisplayList
 * commands. A later SDL3 (or other) device adapter consumes the resulting list and owns all native
 * window/renderer/font objects.
 *
 * Window, Label and Button can be synchronized without a font service because their arranged
 * geometry is already known. ComboBox, CheckBox, RadioButton and TextField presentation additionally
 * depend on rendered font/theme metrics for stable control geometry; TextField also needs scalar-
 * boundary advances for its horizontal viewport and insertion caret. Callers that render these
 * controls therefore construct the sink with a RenderedMeasurementContext. The legacy constructor
 * without metrics intentionally keeps such visible controls deferred rather than drawing an
 * incomplete approximation.
 */
class RenderedPresentationSink final : public PresentationSink {
public:
    explicit RenderedPresentationSink(
        DisplayList& display_list,
        Color background_color = Color::default_color) noexcept
        : display_list_{display_list}, background_color_{background_color} {}

    /**
     * Creates a sink with the rendered metric service needed by editable text presentation.
     *
     * measurement_context is observed, not owned, and must outlive this sink. The same context can be
     * passed to Widget::measure(), keeping layout and caret/viewport geometry on one metric policy.
     */
    RenderedPresentationSink(
        DisplayList& display_list,
        const RenderedMeasurementContext& measurement_context,
        Color background_color = Color::default_color) noexcept
        : display_list_{display_list},
          measurement_context_{&measurement_context},
          background_color_{background_color} {}

    [[nodiscard]] PresentationUpdateResult synchronize(const Widget& widget) override;

    [[nodiscard]] Color backgroundColor() const noexcept { return background_color_; }

private:
    DisplayList& display_list_;

    /*
     * Optional for compatibility with the already useful Window/Label/Button command path. Keeping
     * this non-owning dependency in the presentation sink, rather than a semantic Widget, preserves
     * the Core lifetime rule from ADR 0015: Widgets never retain presentation-specific services.
     */
    const RenderedMeasurementContext* measurement_context_{nullptr};
    Color background_color_{Color::default_color};
};

} // namespace sasd::ui::rendered
