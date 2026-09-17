#include "test_framework.hpp"

#include <sasd/ui/container.hpp>

#include <memory>
#include <stdexcept>

using namespace sasd::ui;

namespace {

class NonVisualComponent final : public Component {};

class LifetimeProbe final : public Component {
public:
    explicit LifetimeProbe(int& destruction_count) : destruction_count_{destruction_count} {}

    ~LifetimeProbe() override {
        ++destruction_count_;
    }

private:
    int& destruction_count_;
};

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

TEST_CASE("Container release clears ownership and visual parenting") {
    Container root;

    auto& widget = root.emplace<Widget>();
    auto& service = root.emplace<NonVisualComponent>();

    auto released_widget = root.release(widget);
    CHECK(released_widget.get() == &widget);
    CHECK(widget.owner() == nullptr);
    CHECK(widget.parent() == nullptr);
    CHECK(root.componentCount() == 1);
    CHECK(root.childCount() == 0);

    auto released_service = root.release(service);
    CHECK(released_service.get() == &service);
    CHECK(service.owner() == nullptr);
    CHECK(root.componentCount() == 0);
}

TEST_CASE("Released components can be transferred between containers") {
    Container first;
    Container second;

    auto& widget = first.emplace<Widget>();
    auto released = first.release(widget);

    CHECK(released != nullptr);
    CHECK(widget.owner() == nullptr);
    CHECK(widget.parent() == nullptr);

    second.adopt(std::move(released));

    CHECK(widget.owner() == &second);
    CHECK(widget.parent() == &second);
    CHECK(second.componentCount() == 1);
    CHECK(second.childCount() == 1);
    CHECK(&second.childAt(0) == &widget);
}

TEST_CASE("Container release leaves unrelated components untouched") {
    Container first;
    Container second;

    auto& first_widget = first.emplace<Widget>();
    auto& second_widget = second.emplace<Widget>();

    auto released = first.release(second_widget);

    CHECK(released == nullptr);
    CHECK(first_widget.owner() == &first);
    CHECK(first_widget.parent() == &first);
    CHECK(second_widget.owner() == &second);
    CHECK(second_widget.parent() == &second);
    CHECK(first.componentCount() == 1);
    CHECK(second.componentCount() == 1);
}

TEST_CASE("Released component lifetime is controlled by returned unique ownership") {
    int destruction_count = 0;
    std::unique_ptr<Component> released;

    {
        Container root;
        auto& probe = root.emplace<LifetimeProbe>(destruction_count);

        released = root.release(probe);
        CHECK(released != nullptr);
        CHECK(destruction_count == 0);
    }

    // Destruction of the old container must not destroy an object that was explicitly released.
    CHECK(destruction_count == 0);
    released.reset();
    CHECK(destruction_count == 1);
}

TEST_CASE("Container destruction releases all owned component lifetimes exactly once") {
    int destruction_count = 0;

    {
        Container root;
        root.emplace<LifetimeProbe>(destruction_count);
        root.emplace<LifetimeProbe>(destruction_count);
        CHECK(destruction_count == 0);
    }

    CHECK(destruction_count == 2);
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
