#include "test_framework.hpp"

#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/focus_manager.hpp>

#include <cstddef>
#include <string>
#include <vector>

using namespace sasd::ui;

namespace {

class CountingModel final : public ListModel {
public:
    explicit CountingModel(std::size_t count) : count_{count} {}

    [[nodiscard]] std::size_t rowCount() const noexcept override { return count_; }

    [[nodiscard]] std::string_view textAt(std::size_t row) const override {
        ++queries_;
        text_ = "row-" + std::to_string(row);
        return text_;
    }

    [[nodiscard]] std::size_t queries() const noexcept { return queries_; }

private:
    std::size_t count_;
    mutable std::size_t queries_{0};
    mutable std::string text_;
};

} // namespace

TEST_CASE("ListSelectionModel keeps single selection separate from focus") {
    StringListModel model({"a", "b", "c"});
    ListSelectionModel selection;
    selection.setModel(&model);

    CHECK(!selection.selectedRow().has_value());
    CHECK(selection.select(1));
    CHECK(selection.selectedRow() == std::optional<std::size_t>{1});
    CHECK(selection.selectNext());
    CHECK(selection.selectedRow() == std::optional<std::size_t>{2});
    CHECK(!selection.selectNext());
    CHECK(selection.selectFirst());
    CHECK(selection.selectLast());
    CHECK(selection.clear());
}

TEST_CASE("ListSelectionModel normalizes inserted removed and reset rows") {
    StringListModel model({"a", "b", "c", "d"});
    ListSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(2));

    model.insert(1, "new");
    CHECK(selection.selectedRow() == std::optional<std::size_t>{3});
    model.erase(3);
    CHECK(!selection.selectedRow().has_value());

    CHECK(selection.select(1));
    model.reset({"only"});
    CHECK(!selection.selectedRow().has_value());
}

TEST_CASE("ListView materializes only its visible range from a huge model") {
    CountingModel model(1'000'000);
    ListSelectionModel selection;
    selection.setModel(&model);
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(500'000, 20);

    const auto rows = view.visibleRows();
    CHECK(rows.size() == 20);
    CHECK(rows.front().row == 500'000);
    CHECK(rows.back().row == 500'019);
    CHECK(model.queries() == 20);
}

TEST_CASE("ListView keyboard navigation changes semantic selection and viewport") {
    StringListModel model({"a", "b", "c", "d"});
    ListSelectionModel selection;
    selection.setModel(&model);
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 2);
    view.arrange({0, 0, 20, 2});

    // Focus is deliberately a separate policy and must be granted by the host.
    view.setFocusable(true);
    FocusManager focus;
    CHECK(focus.requestFocus(view));
    CHECK(view.handleEvent(KeyEvent{Key::down, true, KeyModifier::none}) == EventResult::handled);
    CHECK(selection.selectedRow() == std::optional<std::size_t>{0});
    CHECK(view.handleEvent(KeyEvent{Key::down, true, KeyModifier::none}) == EventResult::handled);
    CHECK(selection.selectedRow() == std::optional<std::size_t>{1});
    CHECK(view.handleEvent(KeyEvent{Key::down, true, KeyModifier::none}) == EventResult::handled);
    CHECK(selection.selectedRow() == std::optional<std::size_t>{2});
    CHECK(view.firstVisibleRow() == 1);
}

TEST_CASE("ListView fails closed after model destruction") {
    ListSelectionModel selection;
    ListView view;
    {
        StringListModel model({"a", "b"});
        selection.setModel(&model);
        view.setModel(&model);
        view.setSelectionModel(&selection);
        CHECK(selection.select(1));
        CHECK(view.visibleRows().size() == 0); // viewport defaults to zero rows
    }
    CHECK(selection.model() == nullptr);
    CHECK(view.model() == nullptr);
    CHECK(view.visibleRows().empty());
}
