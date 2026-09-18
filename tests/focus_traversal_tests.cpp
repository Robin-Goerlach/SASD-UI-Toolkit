#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>

using namespace sasd::ui;

TEST_CASE("FocusTraversal follows visual preorder and wraps forward") {
    VBox root;
    root.emplace<Label>("non-focusable");

    auto& first = root.emplace<TextField>("first");
    auto& row = root.emplace<HBox>();
    auto& second = row.emplace<Button>("second");
    auto& third = row.emplace<TextField>("third");

    FocusManager focus;

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &first);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &second);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &third);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &first);
}

TEST_CASE("FocusTraversal wraps backward and starts from last when focus is empty") {
    VBox root;
    auto& first = root.emplace<TextField>("first");
    auto& second = root.emplace<Button>("second");

    FocusManager focus;

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::backward));
    CHECK(focus.focusedWidget() == &second);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::backward));
    CHECK(focus.focusedWidget() == &first);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::backward));
    CHECK(focus.focusedWidget() == &second);
}

TEST_CASE("FocusTraversal skips hidden disabled and non-focusable candidates") {
    VBox root;
    auto& first = root.emplace<TextField>("first");

    auto& hidden_group = root.emplace<VBox>();
    auto& hidden_child = hidden_group.emplace<Button>("hidden");
    hidden_group.setVisible(false);

    auto& disabled_group = root.emplace<VBox>();
    auto& disabled_child = disabled_group.emplace<Button>("disabled subtree");
    disabled_group.setEnabled(false);

    auto& locally_disabled = root.emplace<Button>("disabled");
    locally_disabled.setEnabled(false);

    root.emplace<Label>("label");
    auto& last = root.emplace<Button>("last");

    FocusManager focus;

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &first);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &last);

    CHECK(focus.focusedWidget() != &hidden_child);
    CHECK(focus.focusedWidget() != &disabled_child);
    CHECK(focus.focusedWidget() != &locally_disabled);
}

TEST_CASE("FocusTraversal repairs focus that is outside the eligible scope sequence") {
    VBox root;
    auto& first = root.emplace<TextField>("first");
    auto& hidden_group = root.emplace<VBox>();
    auto& hidden_child = hidden_group.emplace<Button>("hidden");

    FocusManager focus;
    CHECK(focus.requestFocus(hidden_child));

    // Hiding the ancestor does not alter the older local-only explicit-focus contract immediately.
    hidden_group.setVisible(false);
    CHECK(focus.focusedWidget() == &hidden_child);

    // Keyboard traversal treats ancestor visibility effectively and moves to a reachable candidate.
    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &first);
}

TEST_CASE("FocusTraversal ignores a hidden or disabled scope") {
    VBox root;
    root.emplace<TextField>("field");
    FocusManager focus;

    root.setVisible(false);
    CHECK(!FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == nullptr);

    root.setVisible(true);
    root.setEnabled(false);
    CHECK(!FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == nullptr);
}

TEST_CASE("Single-candidate traversal is handled and keeps stable focus") {
    VBox root;
    auto& field = root.emplace<TextField>("only");
    FocusManager focus;

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &field);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::forward));
    CHECK(focus.focusedWidget() == &field);

    CHECK(FocusTraversal::move(focus, root, FocusTraversalDirection::backward));
    CHECK(focus.focusedWidget() == &field);
}

TEST_CASE("FocusTraversal handles Tab press and Shift Tab in opposite directions") {
    VBox root;
    auto& first = root.emplace<TextField>("first");
    auto& second = root.emplace<Button>("second");
    FocusManager focus;

    CHECK(focus.requestFocus(first));

    const auto forward =
        FocusTraversal::handleEvent(focus, root, KeyEvent{Key::tab, true, KeyModifier::none});
    CHECK(forward == EventResult::handled);
    CHECK(focus.focusedWidget() == &second);

    const auto backward =
        FocusTraversal::handleEvent(focus, root, KeyEvent{Key::tab, true, KeyModifier::shift});
    CHECK(backward == EventResult::handled);
    CHECK(focus.focusedWidget() == &first);
}

TEST_CASE("Tab key release is consumed without repeating traversal") {
    VBox root;
    auto& first = root.emplace<TextField>("first");
    auto& second = root.emplace<Button>("second");
    FocusManager focus;

    CHECK(focus.requestFocus(first));
    CHECK(FocusTraversal::handleEvent(
              focus, root, KeyEvent{Key::tab, true, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &second);

    CHECK(FocusTraversal::handleEvent(
              focus, root, KeyEvent{Key::tab, false, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &second);
}

TEST_CASE("Modified Tab combinations remain unhandled") {
    VBox root;
    auto& field = root.emplace<TextField>("field");
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    CHECK(FocusTraversal::handleEvent(
              focus, root, KeyEvent{Key::tab, true, KeyModifier::control}) ==
          EventResult::ignored);
    CHECK(FocusTraversal::handleEvent(
              focus,
              root,
              KeyEvent{Key::tab,
                       true,
                       KeyModifier::shift | KeyModifier::control}) ==
          EventResult::ignored);

    CHECK(focus.focusedWidget() == &field);
}

TEST_CASE("Tab is ignored when a scope has no eligible focus candidate") {
    VBox root;
    root.emplace<Label>("label");
    FocusManager focus;

    CHECK(FocusTraversal::handleEvent(
              focus, root, KeyEvent{Key::tab, true, KeyModifier::none}) ==
          EventResult::ignored);
    CHECK(focus.focusedWidget() == nullptr);
}
