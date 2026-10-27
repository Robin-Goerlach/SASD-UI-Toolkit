#include <sasd/ui/button.hpp>
#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/ui_description_document.hpp>
#include <sasd/ui/ui_description_reconstruction.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/display_list_executor.hpp>
#include <sasd/ui/rendered/render_device.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/window.hpp>

#include <cstdlib>
#include <cstddef>

namespace {

class PackageRenderDevice final : public sasd::ui::rendered::RenderDevice {
public:
    void fillRect(const sasd::ui::rendered::FillRectCommand&) override {
        ++fill_count;
    }

    void strokeRect(const sasd::ui::rendered::StrokeRectCommand&) override {
        ++stroke_count;
    }

    void drawText(const sasd::ui::rendered::DrawTextCommand&) override {
        ++text_count;
    }

    std::size_t fill_count{0};
    std::size_t stroke_count{0};
    std::size_t text_count{0};
};

} // namespace

int main() {
    /*
     * This executable is deliberately tiny: its purpose is not another widget unit test. It proves
     * that an external project can discover the installed package, include public Core/Terminal/
     * Rendered headers, and link all exported targets without relying on the source-tree layout.
     */
    sasd::ui::Button button{"Package smoke"};
    const auto json = sasd::ui::encodeBuiltInUiDescriptionJson(button);
    if (!json || !json->starts_with("{\n  \"format\": \"sasd.ui-description\",")) {
        return EXIT_FAILURE;
    }
    const auto parsed = sasd::ui::parseBuiltInUiDescriptionJson(*json);
    if (!parsed || parsed.error.code != sasd::ui::UiDescriptionJsonParseErrorCode::none ||
        parsed.document->nodes.size() != 1 || parsed.document->nodes[0].type_id != "sasd.ui.Button") {
        return EXIT_FAILURE;
    }
    auto reconstructed = sasd::ui::reconstructBuiltInUiDescription(*parsed.document);
    if (!reconstructed ||
        sasd::ui::encodeBuiltInUiDescriptionJson(*reconstructed.root) != json) {
        return EXIT_FAILURE;
    }
    sasd::ui::terminal::ScreenBuffer buffer{{2, 1}};
    sasd::ui::rendered::DisplayList display_list;
    sasd::ui::rendered::RenderedPresentationSink rendered_sink{display_list};
    sasd::ui::Window rendered_window;

    buffer.set({0, 0}, {U'X'});

    /*
     * Exercise a non-inline Rendered symbol from the installed library, not just DisplayList headers.
     * Arranging the Window requests a subtree refresh, so synchronizing it records one root clear.
     */
    rendered_window.arrange({0, 0, 32, 16});
    const auto rendered_result = rendered_sink.synchronize(rendered_window);

    /*
     * Also execute the installed DisplayList through the public device boundary. This catches a
     * package/export regression where the new executor header is installed but its non-inline symbol
     * is missing from SASD::UI::Rendered.
     */
    PackageRenderDevice package_device;
    const std::size_t executed =
        sasd::ui::rendered::DisplayListExecutor::execute(display_list, package_device);

    // Exercise the new non-inline typed symbols through the installed public headers/library.
    const auto* style_property =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.Button", "textStyle");
    if (style_property == nullptr) {
        return EXIT_FAILURE;
    }
    const sasd::ui::TextStyle style{sasd::ui::Color::bright_cyan, true, false, true, false};
    if (sasd::ui::builtInTextStylePropertyDefault("sasd.ui.Button", *style_property) !=
            sasd::ui::TextStyle{} ||
        !sasd::ui::writeBuiltInTextStyleProperty(button, *style_property, style) ||
        sasd::ui::readBuiltInTextStyleProperty(button, *style_property) != style) {
        return EXIT_FAILURE;
    }
    const auto snapshot = sasd::ui::captureBuiltInComponentPropertySnapshot(button);
    const bool style_linked = snapshot && snapshot->text_style_properties.size() == 1 &&
        snapshot->text_style_properties.front().value == style &&
        !snapshot->text_style_properties.front().is_default;

    const bool core_linked = button.text() == "Package smoke";
    const bool terminal_linked = buffer.at({0, 0}).code_point == U'X';
    const bool rendered_linked =
        rendered_result == sasd::ui::PresentationUpdateResult::synchronized &&
        display_list.size() == 1 &&
        executed == 1 &&
        package_device.fill_count == 1 &&
        package_device.stroke_count == 0 &&
        package_device.text_count == 0;

    return style_linked && core_linked && terminal_linked && rendered_linked ? EXIT_SUCCESS : EXIT_FAILURE;
}
