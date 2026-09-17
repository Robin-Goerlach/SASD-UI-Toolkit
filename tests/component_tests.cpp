#include "test_framework.hpp"

#include <sasd/ui/container.hpp>

#include <memory>
#include <stdexcept>

using namespace sasd::ui;

namespace {

class NonVisualComponent final : public Component {};

} // namespace

TEST_CASE("Container owns components and assigns visual parents only to widgets") {
    Container root;

    auto& widget = root.emplace<Widget>();
    auto& service = root.emplace<NonVisualComponent>();

    CHECK(root.componentCount() == 2);
    CHECK(widget.owner() == &root);
    CHECK(widget.parent() == &root);
    CHECK(service.owner() == &root);
    CHECK(root.componentAt(0).owner() == &root);
}

TEST_CASE("Widget state is backend neutral") {
    Widget widget;

    CHECK(widget.isVisible());
    CHECK(widget.isEnabled());

    widget.setVisible(false);
    widget.setEnabled(false);
    widget.setBounds({1, 2, 30, 40});

    CHECK(!widget.isVisible());
    CHECK(!widget.isEnabled());
    CHECK(widget.bounds() == Rect{1, 2, 30, 40});
}

TEST_CASE("Container rejects null adoption") {
    Container root;
    bool threw = false;

    try {
        root.adopt(nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}
