#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/widget.hpp>

using namespace sasd::ui;

TEST_CASE("HitTest returns the deepest visible child using parent-relative coordinates") {
    Container root;
    root.arrange({10, 20, 200, 120});

    auto& panel = root.emplace<Container>();
    panel.arrange({15, 10, 100, 80});

    auto& child = panel.emplace<Widget>();
    child.arrange({7, 6, 30, 20});

    // Point is expressed in root-parent coordinates: root + panel + child local offset.
    CHECK(HitTest::deepestAt(root, {33, 37}) == &child);

    // Inside panel but outside child falls back to the deepest containing ancestor.
    CHECK(HitTest::deepestAt(root, {27, 32}) == &panel);
}

TEST_CASE("HitTest uses reverse child order so later painted siblings are topmost") {
    Container root;
    root.arrange({0, 0, 100, 100});

    auto& first = root.emplace<Widget>();
    first.arrange({10, 10, 50, 50});

    auto& second = root.emplace<Widget>();
    second.arrange({20, 20, 50, 50});

    CHECK(HitTest::deepestAt(root, {25, 25}) == &second);
    CHECK(HitTest::deepestAt(root, {15, 15}) == &first);
}

TEST_CASE("HitTest clips descendants by hidden or non-containing ancestors") {
    Container root;
    root.arrange({0, 0, 100, 100});

    auto& panel = root.emplace<Container>();
    panel.arrange({20, 20, 20, 20});

    auto& child = panel.emplace<Widget>();
    child.arrange({15, 15, 20, 20});

    // The child geometrically extends beyond its parent, but hit testing is clipped by ancestors.
    CHECK(HitTest::deepestAt(root, {50, 50}) == &root);

    panel.setVisible(false);
    CHECK(HitTest::deepestAt(root, {25, 25}) == &root);
}

TEST_CASE("HitTest follows half-open bounds and does not treat disabled state as geometry") {
    Container root;
    root.arrange({0, 0, 40, 30});

    auto& child = root.emplace<Widget>();
    child.arrange({5, 6, 10, 8});
    child.setEnabled(false);

    CHECK(HitTest::deepestAt(root, {5, 6}) == &child);
    CHECK(HitTest::deepestAt(root, {14, 13}) == &child);

    // Right/bottom edges are excluded by the same half-open convention as Rect::contains().
    CHECK(HitTest::deepestAt(root, {15, 13}) == &root);
    CHECK(HitTest::deepestAt(root, {14, 14}) == &root);
}
