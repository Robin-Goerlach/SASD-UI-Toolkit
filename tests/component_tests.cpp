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
    CHECK(root.childCount() == 1);
    CHECK(&root.childAt(0) == &widget);
    CHECK(widget.owner() == &root);
    CHECK(widget.parent() == &root);
    CHECK(service.owner() == &root);
    CHECK(root.componentAt(0).owner() == &root);
}

TEST_CASE("Container exposes visual children separately from owned components") {
    Container root;

    auto& first_service = root.emplace<NonVisualComponent>();
    auto& first_widget = root.emplace<Widget>();
    auto& second_service = root.emplace<NonVisualComponent>();
    auto& nested_container = root.emplace<Container>();
    auto& second_widget = root.emplace<Widget>();

    CHECK(root.componentCount() == 5);
    CHECK(root.childCount() == 3);
    CHECK(&root.childAt(0) == &first_widget);
    CHECK(&root.childAt(1) == &nested_container);
    CHECK(&root.childAt(2) == &second_widget);
    CHECK(first_service.owner() == &root);
    CHECK(second_service.owner() == &root);
    CHECK(nested_container.owner() == &root);
    CHECK(nested_container.parent() == &root);

    const Container& const_root = root;
    CHECK(&const_root.childAt(1) == &nested_container);

    bool threw = false;
    try {
        (void)root.childAt(3);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
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
