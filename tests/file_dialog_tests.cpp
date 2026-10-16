#include "test_framework.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/testing/mock_backend.hpp>
#include <sasd/ui/widget.hpp>

#include <stdexcept>

using namespace sasd::ui;
using sasd::ui::testing::MockBackend;

namespace {

class RecordingWidget final : public Widget {
public:
    [[nodiscard]] int deliveryCount() const noexcept { return deliveries; }

protected:
    EventResult onEvent(const Event&) override {
        ++deliveries;
        return EventResult::handled;
    }

    int deliveries{0};
};

} // namespace

TEST_CASE("FileDialogResultEvent enforces accepted, cancelled and failed invariants") {
    const std::filesystem::path selected{"/tmp/selected.txt"};
    const FileDialogResultEvent accepted{1, FileDialogResultStatus::accepted, selected};
    CHECK(accepted.path == selected);
    bool threw = false;
    try { (void)FileDialogResultEvent{1, FileDialogResultStatus::accepted}; } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { (void)FileDialogResultEvent{1, FileDialogResultStatus::cancelled, selected}; } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { (void)FileDialogResultEvent{0, FileDialogResultStatus::failed}; } catch (const std::logic_error&) { threw = true; }
    CHECK(threw);
}

TEST_CASE("FileDialogResultEvent is an application event and never bubbles to widgets") {
    RecordingWidget widget;
    const Event event = FileDialogResultEvent{7, FileDialogResultStatus::cancelled};
    CHECK(std::holds_alternative<FileDialogResultEvent>(event));
    CHECK(!EventDispatcher::dispatch(widget, event).handled());
    CHECK(widget.deliveryCount() == 0);

    MockBackend backend;
    Application application{backend};
    backend.postEvent(event);
    int application_events = 0;
    const auto processed = application.processRoutedEvents(
        [&](const Event&) -> Widget* { return &widget; },
        [&](const Event& received) {
            ++application_events;
            CHECK(std::holds_alternative<FileDialogResultEvent>(received));
        });
    CHECK(processed == 1);
    CHECK(application_events == 1);
    CHECK(widget.deliveryCount() == 0);
}

TEST_CASE("Save dialog acceptance uses the same result event and permits a new target path") {
    const SaveFileDialogRequest request{"Save File"};
    CHECK(request.title == "Save File");
    const Event event = FileDialogResultEvent{
        8, FileDialogResultStatus::accepted, std::filesystem::path{"/tmp/not-created-yet.txt"}};
    CHECK(std::holds_alternative<FileDialogResultEvent>(event));
    const auto& result = std::get<FileDialogResultEvent>(event);
    CHECK(result.status == FileDialogResultStatus::accepted);
    CHECK(result.path == std::filesystem::path{"/tmp/not-created-yet.txt"});
}

TEST_CASE("Backends default to no file-dialog service and no capability") {
    MockBackend backend;
    CHECK(!backend.capabilities().file_dialogs);
    CHECK(backend.fileDialogs() == nullptr);
    backend.initialize();
    CHECK(backend.fileDialogs() == nullptr);
    backend.shutdown();
    CHECK(backend.fileDialogs() == nullptr);
}
