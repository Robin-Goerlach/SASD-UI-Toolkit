#pragma once

#include <sasd/ui/presentation/presentation_sink.hpp>
#include <sasd/ui/rendered/display_list.hpp>

namespace sasd::ui::rendered {

/**
 * Translates semantic Widget presentation into backend-neutral rendered drawing commands.
 *
 * This is deliberately not an SDL sink. It knows only SASD widgets, logical geometry and DisplayList
 * commands. A later SDL3 (or other) device adapter consumes the resulting list and owns all native
 * window/renderer/font objects.
 *
 * The first M3 slice supports Window, Label and Button plus structural Container/VBox/HBox widgets.
 * Visible TextField presentation is intentionally deferred until rendered text metrics can place the
 * insertion caret correctly; acknowledging a TextField without its caret would falsely claim that
 * its complete visual state had been synchronized.
 */
class RenderedPresentationSink final : public PresentationSink {
public:
    explicit RenderedPresentationSink(
        DisplayList& display_list,
        Color background_color = Color::default_color) noexcept
        : display_list_{display_list}, background_color_{background_color} {}

    [[nodiscard]] PresentationUpdateResult synchronize(const Widget& widget) override;

    [[nodiscard]] Color backgroundColor() const noexcept { return background_color_; }

private:
    DisplayList& display_list_;
    Color background_color_{Color::default_color};
};

} // namespace sasd::ui::rendered
