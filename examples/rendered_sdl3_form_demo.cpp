#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/clipboard.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_model.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/menu_model.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/radio_group_navigation.hpp>
#include <sasd/ui/rendered/combo_box_popup_pointer_interaction.hpp>
#include <sasd/ui/rendered/combo_box_popup_presentation.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/menu_frame_presentation.hpp>
#include <sasd/ui/rendered/menu_pointer_interaction.hpp>
#include <sasd/ui/rendered/list_view_presentation.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/rendered/rendered_text_field_pointer_selection.hpp>
#include <sasd/ui/shortcut.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

using namespace std::chrono_literals;

namespace {

using namespace sasd::ui;
using namespace sasd::ui::rendered;
using namespace sasd::ui::rendered::sdl3;

/**
 * Measures/arranges the same small semantic form used by the terminal demo.
 *
 * The only presentation-specific input is the MeasurementContext implemented by the SDL3 backend.
 * Window/VBox/Button/TextField remain the same Core widget classes; there is no parallel SDL widget
 * hierarchy hidden in this example.
 */
void layoutForm(Window& window,
                VBox& form,
                const RenderedMeasurementContext& metrics,
                Size window_size,
                Coordinate menu_bar_height) {
    window.arrange({0, 0, window_size.width, window_size.height});

    const Coordinate content_height =
        window_size.height > menu_bar_height
            ? static_cast<Coordinate>(window_size.height - menu_bar_height)
            : 0;
    const Rect content{0, menu_bar_height, window_size.width, content_height};

    /*
     * The persistent menu bar owns the first logical row of the window. The form is deliberately
     * arranged below that final presentation rectangle rather than teaching Core's Window/VBox
     * layout about menus. This keeps menu geometry a Rendered concern and prevents the ordinary
     * widget tree from painting underneath a persistent overlay.
     */
    (void)form.measure(metrics, {{0, 0}, content.size()});
    form.arrange(content);
}

/**
 * Resolves one Widget's parent-relative arranged bounds into the SDL demo's root logical coordinate
 * system.
 *
 * Rendered ComboBox popup presentation intentionally accepts an already-resolved anchor. It should not
 * know about Widget parenting, VBox layout or top-level-window ownership. Keeping this tree walk in the
 * host preserves that architecture boundary and mirrors the same rule already used by the Terminal
 * demo.
 *
 * Parent offsets are accumulated in 64-bit arithmetic and narrowed only after the complete origin has
 * been proven representable. Pathological nested geometry therefore fails closed instead of wrapping a
 * signed Coordinate into an unrelated on-screen popup location.
 */
[[nodiscard]] std::optional<Rect> absoluteWidgetBounds(const Widget& widget) noexcept {
    std::int64_t x = static_cast<std::int64_t>(widget.bounds().x);
    std::int64_t y = static_cast<std::int64_t>(widget.bounds().y);

    for (const Container* parent = widget.parent();
         parent != nullptr;
         parent = parent->parent()) {
        x += static_cast<std::int64_t>(parent->bounds().x);
        y += static_cast<std::int64_t>(parent->bounds().y);
    }

    const auto minimum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min());
    const auto maximum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());

    if (x < minimum || x > maximum || y < minimum || y > maximum) {
        return std::nullopt;
    }

    return Rect{
        static_cast<Coordinate>(x),
        static_cast<Coordinate>(y),
        widget.bounds().width,
        widget.bounds().height,
    };
}

/**
 * Mirrors logical widget focus into SDL's text-input/IME activation boundary.
 *
 * SDL3 intentionally leaves text input disabled until requested. Keeping this policy in the desktop
 * host/example is preferable to teaching FocusManager about SDL or making Backend assume every
 * focused control accepts text.
 */
void synchronizeTextInput(Sdl3WindowBackend& backend, const FocusManager& focus) {
    const auto* field = dynamic_cast<const TextField*>(focus.focusedWidget());
    const bool should_enable =
        field != nullptr && field->isVisible() && field->isEnabled();

    backend.setTextInputEnabled(should_enable);
}

/**
 * Builds and presents one complete rendered frame, including transient ComboBox popup presentation.
 *
 * RenderedPresentationSink supports incremental synchronization, but an SDL window back buffer is not
 * treated as persistent state after SDL_RenderPresent(). PresentationCoordinator::replay() therefore
 * reconstructs the complete current visual tree without pretending that semantic Widgets became dirty
 * merely because the native presentation surface needs a new frame.
 *
 * The popup is intentionally composed *after* the ordinary Widget replay. It is not inserted into the
 * Widget tree and it never asks SDL to own a second native control. The generic Rendered layer receives
 * the immutable base DisplayList plus the ComboBox's absolute logical anchor and produces a value-owned
 * overlay command stream. SDL remains only the final RenderDevice/metric adapter.
 */
void presentFullFrame(Window& window,
                      ComboBox& surface_mode,
                      DisplayList& display_list,
                      RenderedPresentationSink& presentation,
                      Sdl3WindowBackend& backend,
                      const MenuBarModel& menu_bar,
                      const MenuInteractionController& menu_interaction) {
    display_list.clear();

    const auto pass = PresentationCoordinator::replay(window, presentation);
    if (!pass.complete()) {
        throw std::runtime_error{
            "SDL3 demo contains presentation state the current rendered model cannot represent"};
    }

    const auto menu_frame = buildMenuFramePresentation(
        menu_bar,
        menu_interaction,
        {0, 0},
        window.bounds(),
        backend);
    if (!menu_frame.has_value()) {
        throw std::runtime_error{
            "SDL3 demo menu frame cannot be represented in the current logical viewport"};
    }

    if (menu_interaction.isActive() && surface_mode.isDropDownOpen()) {
        throw std::runtime_error{
            "SDL3 demo has competing menu and ComboBox overlay scopes"};
    }

    if (menu_interaction.isActive() || menu_interaction.popupOpen()) {
        if (!renderMenuFramePresentation(display_list, *menu_frame, Color::black)) {
            throw std::runtime_error{"SDL3 demo menu frame rendering failed"};
        }
        (void)backend.presentFrame(display_list);
        return;
    }

    if (!surface_mode.isDropDownOpen()) {
        if (!renderMenuFramePresentation(display_list, *menu_frame, Color::black)) {
            throw std::runtime_error{"SDL3 demo menu bar rendering failed"};
        }
        (void)backend.presentFrame(display_list);
        return;
    }

    /*
     * ComboBox and menu interaction are mutually exclusive, but the persistent bar remains part of
     * the base for a ComboBox popup. Compose it before the popup so the popup path never has to know
     * how menu titles are painted and the menu bar cannot disappear merely because a ComboBox opened.
     */
    if (!renderMenuFramePresentation(display_list, *menu_frame, Color::black)) {
        throw std::runtime_error{"SDL3 demo menu bar rendering failed"};
    }

    const auto anchor = absoluteWidgetBounds(surface_mode);
    if (!anchor.has_value()) {
        throw std::runtime_error{
            "SDL3 demo ComboBox anchor cannot be represented"};
    }

    /*
     * The Window is arranged at root origin (0,0), so its current bounds are also the complete logical
     * popup viewport. The generic composer handles proportional-font metrics, below/above fallback and
     * immutable base-list composition; this host contributes only tree-to-root geometry and background
     * policy.
     */
    const auto composed = composeComboBoxPopupDisplayList(
        display_list,
        surface_mode,
        *anchor,
        window.bounds(),
        backend,
        Color::black);

    if (!composed.has_value()) {
        throw std::runtime_error{
            "SDL3 demo ComboBox popup cannot be represented in the current logical viewport"};
    }

    (void)backend.presentFrame(*composed);
}

[[nodiscard]] std::string fontPathFromArguments(int argc, char** argv) {
    if (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0') {
        return argv[1];
    }

#if defined(_WIN32)
    /*
     * MSVC deliberately diagnoses std::getenv() as C4996. The demo is built with /W4 and can be
     * built with /WX, so suppressing that diagnostic globally would weaken the warning policy for
     * unrelated code. Use Microsoft's ownership-explicit environment API locally instead.
     *
     * _dupenv_s() allocates the returned buffer with malloc-compatible storage. Copy into std::string
     * before releasing it so no environment pointer/lifetime leaks into the rest of the demo.
     */
    char* environment = nullptr;
    std::size_t environment_size = 0;

    if (_dupenv_s(&environment, &environment_size, "SASD_UI_FONT") == 0) {
        std::string value;

        if (environment != nullptr) {
            value.assign(environment);
            std::free(environment);
        }

        if (!value.empty()) {
            return value;
        }
    } else if (environment != nullptr) {
        /*
         * Defensive cleanup in case a CRT implementation ever reports failure after assigning a
         * buffer. Current MSVC documentation normally leaves it null on failure.
         */
        std::free(environment);
    }
#else
    if (const char* environment = std::getenv("SASD_UI_FONT");
        environment != nullptr && environment[0] != '\0') {
        return environment;
    }
#endif

    return {};
}

} // namespace

int main(int argc, char** argv) {
    try {
        using namespace sasd::ui;
        using namespace sasd::ui::rendered;
        using namespace sasd::ui::rendered::sdl3;

        const std::string font_path = fontPathFromArguments(argc, argv);
        if (font_path.empty()) {
            std::cerr
                << "Usage: sasd_ui_sdl3_demo <path-to-font.ttf>\n"
                << "   or: set SASD_UI_FONT to a TrueType/OpenType font file.\n";
            return EXIT_FAILURE;
        }

        Sdl3WindowBackendConfig backend_config;
        backend_config.title = "SASD UI Toolkit - Rendered SDL3 Demo";
        backend_config.initial_size = {800, 500};
        backend_config.font_path = font_path;
        backend_config.font_point_size = 18.0F;
        backend_config.hidden = false;
        backend_config.resizable = true;
        backend_config.high_pixel_density = true;
        backend_config.background_color = Color::black;

        Sdl3WindowBackend backend{std::move(backend_config)};
        Application application{backend};

        /* Commands are semantic stack objects; the Window below owns only visual Widgets. */
        Command greet_command{"Greet"};
        Command help_command{"Help"};
        Command exit_command{"Exit"};
        Command select_all_command{"Select All"};
        Command copy_command{"Copy"};
        Command cut_command{"Cut"};
        Command paste_command{"Paste"};

        // The list model is application data, not a Widget child. Its lifetime is deliberately
        // independent from the visual ListView and its rows are never represented by child Widgets.
        StringListModel recent_items({"Portable", "Terminal", "Rendered", "Headless test data"});
        ListSelectionModel recent_selection;
        recent_selection.setModel(&recent_items);

        /*
         * Menu models borrow Command identity but do not own Commands. Commands therefore outlive the
         * menu tree and the Widget bindings below. The menu bar is semantic Core state; only its later
         * Rendered snapshot will contain copied labels and final rectangles.
         */
        MenuBarModel menu_bar;
        MenuModel& actions_menu = menu_bar.appendMenu("Actions");
        actions_menu.appendCommand(greet_command);
        actions_menu.appendCommand(exit_command);

        MenuModel& edit_menu = menu_bar.appendMenu("Edit");
        edit_menu.appendCommand(
            select_all_command,
            Shortcut{Key::a, KeyModifier::control});
        edit_menu.appendCommand(copy_command, Shortcut{Key::c, KeyModifier::control});
        edit_menu.appendCommand(cut_command, Shortcut{Key::x, KeyModifier::control});
        edit_menu.appendCommand(paste_command, Shortcut{Key::v, KeyModifier::control});

        MenuModel& help_menu = menu_bar.appendMenu("Help");
        help_menu.appendCommand(help_command, Shortcut{Key::f1, KeyModifier::none});

        MenuInteractionController menu_interaction;
        RenderedMenuPointerInteraction::GestureState menu_pointer_gesture;
        ShortcutMap shortcuts;

        DisplayList display_list;
        RenderedPresentationSink presentation{
            display_list,
            backend,
            Color::black};

        /*
         * RadioGroup is a non-visual semantic relationship object. Declare it before Window so normal
         * reverse local destruction destroys the Window (and therefore its RadioButtons) first. The
         * relationship code is lifetime-safe in either order, but this ordering also makes the demo's
         * intended ownership graph obvious: Window owns Widgets; RadioGroup owns none of them.
         */
        RadioGroup greeting_group;

        Window window;
        auto& form = window.emplace<VBox>();
        form.setSpacing(8);

        auto& title = form.emplace<Label>("SASD UI Toolkit - Rendered SDL3 Demo");
        TextStyle title_style;
        title_style.foreground = Color::bright_cyan;
        title_style.bold = true;
        title.setTextStyle(title_style);

        auto& name_label = form.emplace<Label>("Name:");
        TextStyle name_label_style;
        name_label_style.foreground = Color::bright_blue;
        name_label.setTextStyle(name_label_style);

        auto& name = form.emplace<TextField>();
        TextStyle field_style;
        field_style.foreground = Color::bright_white;
        name.setTextStyle(field_style);

        /*
         * This is the same backend-neutral ComboBox used by the Terminal demo. The collapsed control is
         * a normal VBox child; only its open item surface becomes a transient Rendered DisplayList
         * overlay. SDL therefore supplies font metrics/pixels without becoming the owner of ComboBox
         * selection, preview or open/closed state.
         */
        auto& surface_label = form.emplace<Label>("ComboBox demo:");
        surface_label.setTextStyle(name_label_style);

        auto& surface_mode = form.emplace<ComboBox>(
            std::vector<std::string>{"Portable", "Terminal", "Rendered"});
        TextStyle combo_style;
        combo_style.foreground = Color::bright_cyan;
        surface_mode.setTextStyle(combo_style);
        (void)surface_mode.setSelectedIndex(0U);

        /*
         * Keep the first M4 form control in the same semantic tree as the M2/M3 widgets. The example
         * does not construct an SDL-specific checkbox: Core owns checked state and interaction while
         * RenderedPresentationSink decides how that state becomes pixels.
         *
         * Starting checked preserves the demo's historical "Hello, name!" behavior until the user
         * deliberately changes the option.
         */
        auto& enthusiastic =
            form.emplace<CheckBox>("Enthusiastic greeting", true);
        TextStyle checkbox_style;
        checkbox_style.foreground = Color::bright_magenta;
        enthusiastic.setTextStyle(checkbox_style);

        /*
         * Two RadioButtons share one explicit semantic group. Their mutual exclusion therefore
         * survives future layout/reparenting changes and is not an accidental property of VBox.
         */
        auto& hello_style =
            form.emplace<RadioButton>(greeting_group, "Greeting word: Hello");
        auto& hi_style =
            form.emplace<RadioButton>(greeting_group, "Greeting word: Hi");

        TextStyle radio_style;
        radio_style.foreground = Color::bright_blue;
        hello_style.setTextStyle(radio_style);
        hi_style.setTextStyle(radio_style);

        /*
         * Establish the initial application choice programmatically. The first selection callback is
         * installed below, so startup does not manufacture a user-visible change notification.
         */
        (void)hello_style.setSelected(true);

        auto& greet = form.emplace<Button>("Greet");
        TextStyle greet_style;
        greet_style.foreground = Color::bright_green;
        greet_style.bold = true;
        greet.setTextStyle(greet_style);

        auto& status = form.emplace<Label>(
            "ComboBox: click/F4 opens; pointer/arrows preview; click/Enter commits; outside click/Escape cancels. F1 help.");
        TextStyle status_style;
        status_style.foreground = Color::yellow;
        status.setTextStyle(status_style);

        auto& list_label = form.emplace<Label>("Virtualized ListView:");
        list_label.setTextStyle(name_label_style);
        auto& recent_view = form.emplace<ListView>();
        recent_view.setModel(&recent_items);
        recent_view.setSelectionModel(&recent_selection);
        recent_view.setViewport(0, 4);
        recent_view.setSizeConstraints({{0, 4}, {40, 4}, {std::numeric_limits<Coordinate>::max(), 4}});

        auto& exit = form.emplace<Button>("Exit");
        TextStyle exit_style;
        exit_style.foreground = Color::bright_red;
        exit.setTextStyle(exit_style);

        greet.bindCommand(greet_command);
        exit.bindCommand(exit_command);
        shortcuts.bind({Key::f1, KeyModifier::none}, help_command);
        shortcuts.bind({Key::a, KeyModifier::control}, select_all_command);
        shortcuts.bind({Key::c, KeyModifier::control}, copy_command);
        shortcuts.bind({Key::x, KeyModifier::control}, cut_command);
        shortcuts.bind({Key::v, KeyModifier::control}, paste_command);

        FocusManager focus;
        PointerRouter pointer_router;

        /*
         * Keep rendered multi-event selection policy next to the PointerRouter that owns capture.
         * GestureState stores only Unicode-scalar word-origin information, never Widget ownership or
         * native SDL state. The host therefore preserves just enough transient policy to support
         * double-click-and-drag while PointerRouter remains the sole authority for gesture lifetime.
         */
        RenderedTextFieldPointerSelection::GestureState text_selection_gesture;

        /*
         * Rendered popup click completion is a host-owned two-event transaction. It stores only the
         * painted row index armed by Primary press; every event still validates that identity against
         * a freshly built owned popup snapshot before it can affect Core selection.
         */
        RenderedComboBoxPopupPointerInteraction::GestureState
            combo_popup_gesture;

        /* Explicit host synchronization is sufficient here; no general binding framework is needed. */
        const auto synchronizeEditCommands = [&] {
            const bool target_active = focus.focusedWidget() == &name &&
                                       name.isVisible() && name.isEnabled();
            select_all_command.setEnabled(target_active);
            const bool clipboard_available = backend.clipboard() != nullptr;
            copy_command.setEnabled(target_active && clipboard_available && name.hasSelection());
            cut_command.setEnabled(target_active && clipboard_available && name.hasSelection());
            paste_command.setEnabled(target_active && clipboard_available);
        };

        greet_command.setOnExecuted([&] {
            std::string value{name.text()};
            if (value.empty()) {
                value = "world";
            }

            /*
             * Application logic consumes the semantic checked value only. Whether the option was
             * toggled by keyboard, SDL pointer input or a future native peer is irrelevant here.
             */
            const char punctuation = enthusiastic.isChecked() ? '!' : '.';
            const std::string greeting_word =
                hi_style.isSelected() ? "Hi" : "Hello";
            status.setText(
                greeting_word + ", " + value + std::string(1, punctuation));
        });

        enthusiastic.setOnCheckedChanged([&](bool checked) {
            /*
             * The callback demonstrates that checked-state notification is backend-neutral. It also
             * gives the visible smoke test immediate feedback without coupling CheckBox to Label.
             */
            status.setText(
                checked
                    ? "Greeting style: enthusiastic (!)"
                    : "Greeting style: calm (.)");
        });

        surface_mode.setOnSelectionChanged([&](std::optional<std::size_t>) {
            /*
             * Preview navigation remains transient while the popup is open. The visible status changes
             * only after Core publishes a committed selection, making preview-versus-commit behavior
             * observable in the real SDL window without introducing adapter-specific application state.
             */
            const auto selected = surface_mode.selectedText();
            status.setText(
                selected.has_value()
                    ? "ComboBox selection committed: " + std::string{*selected}
                    : "ComboBox selection cleared");
        });

        hello_style.setOnSelected([&] {
            /*
             * RadioGroup has already retired the previous selection before this callback runs. The
             * application can therefore trust group state immediately and need not repair exclusivity.
             */
            status.setText("Greeting word selected: Hello");
        });

        hi_style.setOnSelected([&] {
            status.setText("Greeting word selected: Hi");
        });

        help_command.setOnExecuted([&] {
            status.setText(
                "Help: Ctrl+A selects Name; Ctrl+C/X/V copy, cut or paste; F1 help; F10 exits.");
        });

        exit_command.setOnExecuted([&] {
            application.requestExit();
        });

        select_all_command.setOnExecuted([&] { name.selectAll(); });
        copy_command.setOnExecuted([&] {
            if (Clipboard* clipboard = backend.clipboard(); clipboard != nullptr) {
                (void)name.copySelectionToClipboard(*clipboard);
            }
        });
        cut_command.setOnExecuted([&] {
            if (Clipboard* clipboard = backend.clipboard(); clipboard != nullptr) {
                (void)name.cutSelectionToClipboard(*clipboard);
            }
        });
        paste_command.setOnExecuted([&] {
            if (Clipboard* clipboard = backend.clipboard(); clipboard != nullptr) {
                (void)name.pasteFromClipboard(*clipboard);
            }
        });

        auto recent_selection_subscription = recent_selection.observe([&](const ListSelectionChange& change) {
            if (!change.current.has_value()) {
                status.setText("ListView selection cleared");
                return;
            }
            status.setText("ListView selected row " + std::to_string(*change.current));
        });
        (void)recent_selection_subscription;

        const auto initial_menu_frame = buildMenuFramePresentation(
            menu_bar,
            menu_interaction,
            {0, 0},
            {0, 0, backend.windowSize().width, backend.windowSize().height},
            backend);
        if (!initial_menu_frame.has_value()) {
            throw std::runtime_error{"SDL3 demo initial menu frame cannot be represented"};
        }
        layoutForm(
            window,
            form,
            backend,
            backend.windowSize(),
            initial_menu_frame->menu_bar.row_height);
        (void)focus.requestFocus(name);
        synchronizeTextInput(backend, focus);
        synchronizeEditCommands();
        presentFullFrame(
            window,
            surface_mode,
            display_list,
            presentation,
            backend,
            menu_bar,
            menu_interaction);

        Command::Reference pending_menu_command;

        while (!application.exitRequested()) {
            bool menu_presentation_changed = false;
            (void)application.processRoutedEvents(
                [&](const Event& event) -> Widget* {
                    if (std::holds_alternative<KeyEvent>(event) ||
                        std::holds_alternative<TextInputEvent>(event)) {
                        return focus.focusedWidget();
                    }

                    /*
                     * PointerRouter owns hit testing/capture and dispatches pointer events itself in
                     * the fallback handler below. Returning nullptr here prevents Application from
                     * performing a second ordinary dispatch of the same pointer input.
                     */
                    return nullptr;
                },
                [&](const Event& event) {
                    if (const auto* surface =
                            std::get_if<PointerSurfaceEvent>(&event)) {
                        /*
                         * Surface leave is host lifecycle, not a Widget event. Core cannot assume
                         * native mouse capture continues outside this SDL window, so retire hover and
                         * any active semantic capture conservatively. Retire the paired rendered word
                         * gesture state at the same boundary rather than waiting for another pointer
                         * event to observe that capture disappeared. Enter needs no action: the next
                         * real PointerEvent rebuilds hover from trustworthy logical coordinates.
                         */
                        if (surface->action == PointerSurfaceAction::left) {
                            pointer_router.leaveRoot();
                            text_selection_gesture.reset();
                            combo_popup_gesture.reset();
                            RenderedMenuPointerInteraction::handleSurfaceEvent(
                                *surface,
                                menu_pointer_gesture);
                        }
                        return;
                    }

                    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
                        /*
                         * The menu overlay has first refusal for every pointer sample. It is rebuilt
                         * from the current semantic state immediately before hit testing, so this
                         * adapter never guesses old popup placement or retains a frame pointer across
                         * events. An engaged result is modal even when the semantic action is a no-op.
                         */
                        const auto menu_frame = buildMenuFramePresentation(
                            menu_bar,
                            menu_interaction,
                            {0, 0},
                            window.bounds(),
                            backend);
                        if (menu_frame.has_value()) {
                            const auto menu_result =
                                RenderedMenuPointerInteraction::handle(
                                    menu_bar,
                                    menu_interaction,
                                    *menu_frame,
                                    *pointer,
                                    menu_pointer_gesture);
                            if (menu_result.has_value()) {
                                if (menu_interaction.isActive() &&
                                    surface_mode.isDropDownOpen()) {
                                    /*
                                     * Opening a menu claims the only transient overlay scope. Cancel
                                     * ComboBox preview before the menu frame is presented; otherwise
                                     * two independent surfaces could both believe they own pointer
                                     * dismissal and release gestures.
                                     */
                                    (void)surface_mode.setDropDownOpen(false);
                                }
                                pointer_router.leaveRoot();
                                text_selection_gesture.reset();
                                combo_popup_gesture.reset();
                                if (menu_result->action != MenuInteractionAction::none) {
                                    menu_presentation_changed = true;
                                }
                                if (menu_result->action ==
                                    MenuInteractionAction::activate_command) {
                                    pending_menu_command = menu_result->command;
                                }
                                return;
                            }
                        } else if (menu_interaction.isActive()) {
                            /* A stale/unrepresentable frame fails closed at the modal boundary. */
                            menu_interaction.reset();
                            menu_pointer_gesture.reset();
                            menu_presentation_changed = true;
                            pointer_router.leaveRoot();
                            text_selection_gesture.reset();
                            combo_popup_gesture.reset();
                            return;
                        }

                        if (surface_mode.isDropDownOpen()) {
                            /*
                             * The popup is an overlay rather than a Widget-tree child, so its exact final
                             * presentation snapshot must receive input before ordinary hit testing. This
                             * prevents a painted row or outside-dismiss press from clicking through to a
                             * covered Button/TextField. Any old root gesture is retired at the modal seam.
                             */
                            pointer_router.leaveRoot();
                            text_selection_gesture.reset();

                            const auto anchor = absoluteWidgetBounds(surface_mode);
                            if (!anchor.has_value()) {
                                combo_popup_gesture.reset();
                                throw std::runtime_error{
                                    "SDL3 demo ComboBox pointer anchor cannot be represented"};
                            }

                            const auto snapshot = buildComboBoxPopupPresentation(
                                surface_mode,
                                *anchor,
                                window.bounds(),
                                backend);
                            if (!snapshot.has_value()) {
                                combo_popup_gesture.reset();
                                throw std::runtime_error{
                                    "SDL3 demo ComboBox pointer snapshot cannot be represented"};
                            }

                            const auto popup_result =
                                RenderedComboBoxPopupPointerInteraction::handle(
                                    surface_mode,
                                    *snapshot,
                                    *pointer,
                                    combo_popup_gesture);
                            if (!popup_result.has_value()) {
                                throw std::runtime_error{
                                    "SDL3 demo ComboBox pointer snapshot became semantically stale"};
                            }
                            return;
                        }

                        /* Closed state cannot inherit a row press from an older popup scope. */
                        combo_popup_gesture.reset();

                        /*
                         * Desktop focus-on-primary-press remains host policy. The rendered TextField
                         * selection helper deliberately does not own FocusManager because focus scope,
                         * modal policy and top-level window activation are application/host concerns.
                         */
                        if (pointer->action == PointerAction::press &&
                            pointer->button == PointerButton::primary) {
                            Widget* hit = HitTest::deepestAt(window, pointer->position);

                            if (hit != nullptr && hit->canReceiveFocus()) {
                                (void)focus.requestFocus(*hit);
                            }

                            if (hit == &recent_view) {
                                const auto bounds = absoluteWidgetBounds(recent_view);
                                if (!bounds.has_value()) {
                                    throw std::runtime_error{
                                        "SDL3 demo ListView bounds cannot be represented"};
                                }
                                const auto snapshot =
                                    RenderedListViewPresentation::snapshot(recent_view, *bounds);
                                if (!snapshot.has_value()) {
                                    throw std::runtime_error{
                                        "SDL3 demo ListView pointer snapshot is stale or unrepresentable"};
                                }
                                // Selection uses the exact row identity from the owned frame-side
                                // snapshot. Returning here prevents PointerRouter from inventing a
                                // second ListView interaction state machine.
                                (void)RenderedListViewPresentation::selectAt(
                                    *snapshot, pointer->position, recent_selection);
                                pointer_router.leaveRoot();
                                return;
                            }
                        }

                        /*
                         * Route all pointer events through the stateful rendered TextField selection
                         * seam. For non-TextField controls it remains behaviorally identical to
                         * PointerRouter::route(). Ordinary/Shift TextField drags stay character based,
                         * while an unmodified double click now preserves its semantic word origin across
                         * captured move/release events and extends only by complete word/punctuation
                         * runs. GestureState carries no Widget pointer; PointerRouter capture continues
                         * to own the actual interaction lifetime.
                         */
                        (void)RenderedTextFieldPointerSelection::route(
                            window,
                            pointer_router,
                            *pointer,
                            backend,
                            text_selection_gesture);
                        return;
                    }

                    if (const auto* resize = std::get_if<ResizeEvent>(&event)) {
                        /*
                         * Rendered popup bounds belong to the old logical viewport. Cancel the transient
                         * open/preview transaction before re-layout rather than carrying a placement that
                         * may no longer fit either side after a live window resize. Committed selection is
                         * preserved by Core and reopening seeds preview from it again.
                         */
                        if (surface_mode.isDropDownOpen()) {
                            combo_popup_gesture.reset();
                            (void)surface_mode.setDropDownOpen(false);
                        }

                        auto resized_menu_frame = buildMenuFramePresentation(
                            menu_bar,
                            menu_interaction,
                            {0, 0},
                            {0, 0, resize->size.width, resize->size.height},
                            backend);
                        if (!resized_menu_frame.has_value()) {
                            /*
                             * A new viewport must never reuse final rectangles from the old one. Closing
                             * the transient menu is the conservative resize policy; the next F10/title
                             * interaction builds a fresh frame against the new logical viewport.
                             */
                            menu_interaction.reset();
                            menu_pointer_gesture.reset();
                            resized_menu_frame = buildMenuFramePresentation(
                                menu_bar,
                                menu_interaction,
                                {0, 0},
                                {0, 0, resize->size.width, resize->size.height},
                                backend);
                        }
                        if (!resized_menu_frame.has_value()) {
                            throw std::runtime_error{
                                "SDL3 demo resized menu frame cannot be represented"};
                        }
                        layoutForm(
                            window,
                            form,
                            backend,
                            resize->size,
                            resized_menu_frame->menu_bar.row_height);

                        /*
                         * Present resize results immediately instead of waiting until
                         * processRoutedEvents() has drained the entire native event queue.
                         *
                         * During an interactive desktop resize SDL/Windows can enqueue many resize
                         * notifications while the old back buffer is still visible. Deferring the
                         * repaint until the end of that batch lets the window manager temporarily
                         * stretch or expose stale pixels, which is especially noticeable on borders
                         * and text. Rendering here keeps layout and the visible back buffer closely
                         * synchronized without teaching the backend-neutral Application class about
                         * frame scheduling.
                         *
                         * A minimized/native-zero-sized window has no useful drawable surface. Keep
                         * its semantic layout current, but wait for the next positive ResizeEvent
                         * before presenting again.
                         */
                        if (!resize->size.isEmpty()) {
                            presentFullFrame(
                                window,
                                surface_mode,
                                display_list,
                                presentation,
                                backend,
                                menu_bar,
                                menu_interaction);
                        }
                        return;
                    }

                    /* Keyboard/focus handling is a competing completion path for popup pointer state. */
                    combo_popup_gesture.reset();

                    if (const auto* key = std::get_if<KeyEvent>(&event); key != nullptr) {
                        /*
                         * F10 is host policy for entering/leaving menu mode. It is handled before
                         * ShortcutMap so it cannot accidentally execute an unrelated application
                         * command. Opening the menu cancels an uncommitted ComboBox preview and any
                         * captured widget gesture at this transient-overlay boundary.
                         */
                        if (key->pressed &&
                            key->modifiers == KeyModifier::none &&
                            key->key == Key::f10) {
                            menu_pointer_gesture.reset();
                            pointer_router.leaveRoot();
                            text_selection_gesture.reset();
                            combo_popup_gesture.reset();

                            if (menu_interaction.isActive()) {
                                menu_interaction.reset();
                            } else {
                                if (surface_mode.isDropDownOpen()) {
                                    (void)surface_mode.setDropDownOpen(false);
                                }
                                (void)menu_interaction.begin(menu_bar);
                            }
                            menu_presentation_changed = true;
                            return;
                        }

                        if (menu_interaction.isActive()) {
                            const MenuInteractionResult menu_result =
                                menu_interaction.handleKey(menu_bar, *key);
                            if (menu_result.action != MenuInteractionAction::none) {
                                menu_presentation_changed = true;
                            }
                            if (menu_result.action ==
                                MenuInteractionAction::activate_command) {
                                pending_menu_command = menu_result.command;
                            }

                            /* Menu mode owns even unhandled keys to prevent click/key-through. */
                            return;
                        }

                        if (key->pressed && shortcuts.dispatch(*key)) {
                            synchronizeEditCommands();
                            return;
                        }
                    }

                    /*
                     * Radio-group arrow navigation is deliberately separate from generic Tab
                     * traversal. This host opts the current focus scope into the policy explicitly,
                     * preventing a semantic RadioGroup that spans windows from silently moving focus
                     * across top-level surfaces.
                     */
                    if (auto* radio =
                            dynamic_cast<RadioButton*>(focus.focusedWidget());
                        radio != nullptr &&
                        RadioGroupNavigation::handleEvent(
                            focus, *radio, event) == EventResult::handled) {
                        return;
                    }

                    if (FocusTraversal::handleEvent(focus, form, event) ==
                        EventResult::handled) {
                        return;
                    }

                    if (const auto* key = std::get_if<KeyEvent>(&event);
                        key != nullptr &&
                        key->pressed &&
                        key->modifiers == KeyModifier::none &&
                        key->key == Key::escape) {
                        application.requestExit();
                    }

                    synchronizeEditCommands();
                });

            if (application.exitRequested()) {
                break;
            }

            /*
             * Focus traversal may have moved from TextField to Button (or back). Apply text-input
             * activation after the whole event batch so SDL IME state mirrors the final logical focus.
             */
            synchronizeTextInput(backend, focus);
            synchronizeEditCommands();

            if (!form.isMeasureValid()) {
                const auto current_menu_frame = buildMenuFramePresentation(
                    menu_bar,
                    menu_interaction,
                    {0, 0},
                    {0, 0, backend.windowSize().width, backend.windowSize().height},
                    backend);
                if (!current_menu_frame.has_value()) {
                    throw std::runtime_error{
                        "SDL3 demo current menu frame cannot be represented"};
                }
                layoutForm(
                    window,
                    form,
                    backend,
                    backend.windowSize(),
                    current_menu_frame->menu_bar.row_height);
            }

            /*
             * A semantic visual change makes Window pending. Expose/scale events can require repaint
             * even when every Widget is semantically clean, which is why the backend also exposes its
             * presentationRequested() bit.
             */
            if (!backend.windowSize().isEmpty() &&
                (window.isVisualUpdatePending() ||
                 backend.presentationRequested() ||
                 menu_presentation_changed)) {
                /*
                 * Non-resize invalidation and expose/scale requests still use the normal end-of-batch
                 * presentation path. Resize itself is handled eagerly above to reduce live-resize
                 * stale-frame artifacts.
                 */
                presentFullFrame(
                    window,
                    surface_mode,
                    display_list,
                    presentation,
                    backend,
                    menu_bar,
                    menu_interaction);
            }

            /*
             * MenuInteractionController returns a non-owning Command::Reference and closes its
             * transient state before returning it. Present the closed/stabilized frame first, then
             * execute application code. The callback may mutate commands, menus or the host's widget
             * state; no menu member is accessed through the activation boundary itself.
             */
            if (Command* command = pending_menu_command.get(); command != nullptr) {
                pending_menu_command = {};
                (void)command->execute();
            }

            /*
             * The generic Backend polling contract is deliberately non-blocking. Keep the first
             * desktop demo aligned with the terminal demo's simple loop; a later Application::run()
             * wait/wakeup abstraction can replace this sleep for both without changing event semantics.
             */
            std::this_thread::sleep_for(8ms);
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "SASD UI SDL3 demo failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
