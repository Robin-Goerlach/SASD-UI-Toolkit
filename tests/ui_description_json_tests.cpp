#include "test_framework.hpp"
#include "ui_description_json_golden.hpp"

#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/tree_selection_model.hpp>
#include <sasd/ui/tree_view.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <array>
#include <limits>

using namespace sasd::ui;

TEST_CASE("JSON v1 default Label exact golden includes every default") {
    Label label;
    CHECK(encodeBuiltInUiDescriptionJson(label) == defaultLabelJson);
}

TEST_CASE("JSON v1 comprehensive exact golden and repeat determinism") {
    Window root;
    auto& command = root.emplace<Command>("Save");
    auto& box = root.emplace<VBox>();
    box.setSpacing(12);
    auto& label = box.emplace<Label>("Name");
    label.setTextStyle({Color::bright_cyan, true, false, true, false});
    label.setSizeConstraints({{-12, -3}, {40, 20}, {100, 50}});
    box.emplace<Button>("Save").bindCommand(command);
    auto& combo = box.emplace<ComboBox>(std::vector<std::string>{"One", "Two"});
    CHECK(combo.setSelectedIndex(1));
    box.emplace<GridLayout>(3);
    const auto json = encodeBuiltInUiDescriptionJson(root);
    CHECK(json.has_value());
    CHECK(*json == comprehensiveJson);
    CHECK(encodeBuiltInUiDescriptionJson(root) == json);
    CHECK(json->find('\r') == std::string::npos);
    CHECK(json->ends_with("}\n"));
    CHECK(!json->ends_with("\n\n"));
    for (const auto excluded : {"isDefault", "is_default", "visualParent", "bounds",
             "desiredSize", "focus\"", "hover", "pressed", "capture", "cursor", "preview",
             "dropDownOpen", "viewport", "onActivated", "observer", "function"}) {
        CHECK(json->find(excluded) == std::string::npos);
    }
    ComboBox empty;
    const auto emptyJson = encodeBuiltInUiDescriptionJson(empty);
    CHECK(emptyJson->find("\"value\": []") != std::string::npos);
    CHECK(emptyJson->find("\"value\": null") != std::string::npos);
}

TEST_CASE("JSON v1 validates UTF-8 and escapes controls without sanitizing") {
    Label label{"ASCII / \"\\\b\f\n\r\t"};
    std::string text{label.text()};
    text += '\0'; text += '\x01'; text += '\x1f';
    // Explicit bytes keep source-encoding policy out of this cross-platform test.
    text += " \xc3\xa4\xc3\xb6\xc3\xbc \xe6\x97\xa5 \xf0\x9f\x98\x80";
    label.setText(text);
    const auto json = encodeBuiltInUiDescriptionJson(label);
    CHECK(json.has_value());
    CHECK(json->find("ASCII / \\\"\\\\\\b\\f\\n\\r\\t\\u0000\\u0001\\u001f "
                     "\xc3\xa4\xc3\xb6\xc3\xbc \xe6\x97\xa5 \xf0\x9f\x98\x80") != std::string::npos);
    for (const auto invalid : {"\x80", "\xc0\xaf", "\xe2\x82", "\xed\xa0\x80",
                               "\xf4\x90\x80\x80", "\xff"}) {
        label.setText(invalid);
        CHECK(!encodeBuiltInUiDescriptionJson(label));
        ComboBox combo{std::vector<std::string>{"valid", invalid}};
        CHECK(!encodeBuiltInUiDescriptionJson(combo));
        Command command{invalid};
        CHECK(!encodeBuiltInUiDescriptionJson(command));
    }
}

TEST_CASE("JSON v1 all seventeen Color symbols and unknown Color rejection") {
    const std::array colors{Color::default_color, Color::black, Color::red, Color::green,
        Color::yellow, Color::blue, Color::magenta, Color::cyan, Color::white,
        Color::bright_black, Color::bright_red, Color::bright_green, Color::bright_yellow,
        Color::bright_blue, Color::bright_magenta, Color::bright_cyan, Color::bright_white};
    const std::array symbols{"default", "black", "red", "green", "yellow", "blue", "magenta",
        "cyan", "white", "bright-black", "bright-red", "bright-green", "bright-yellow",
        "bright-blue", "bright-magenta", "bright-cyan", "bright-white"};
    Label label;
    for (std::size_t i = 0; i < colors.size(); ++i) {
        label.setTextStyle({colors[i], true, true, true, true});
        const auto json = encodeBuiltInUiDescriptionJson(label);
        CHECK(json.has_value());
        CHECK(json->find(std::string{"\"foreground\": \""} + symbols[i] + '"') != std::string::npos);
    }
    label.setTextStyle({static_cast<Color>(255), false, false, false, false});
    CHECK(label.textStyle().foreground == static_cast<Color>(255));
    CHECK(!encodeBuiltInUiDescriptionJson(label));
}

TEST_CASE("JSON v1 all eleven reference symbols and no application model payload") {
    Window root;
    auto& command = root.emplace<Command>();
    root.emplace<Button>().bindCommand(command);
    auto& group = root.emplace<RadioGroup>();
    root.emplace<RadioButton>(group, "Choice");
    auto& list = root.emplace<StringListModel>(std::vector<std::string>{"SECRET_LIST"});
    auto& ls = root.emplace<ListSelectionModel>(); ls.setModel(&list);
    auto& lv = root.emplace<ListView>(); lv.setModel(&list); lv.setSelectionModel(&ls);
    CHECK(ls.select(0));
    auto& table = root.emplace<StringTableModel>(std::vector<std::string>{"SECRET_HEADER"},
        std::vector<std::vector<std::string>>{{"SECRET_TABLE"}});
    auto& ts = root.emplace<TableSelectionModel>(); ts.setModel(&table);
    auto& tv = root.emplace<TableView>(); tv.setModel(&table); tv.setSelectionModel(&ts);
    auto& tree = root.emplace<StringTreeModel>(std::vector<std::string>{"SECRET_TREE"});
    auto& trs = root.emplace<TreeSelectionModel>(); trs.setModel(&tree);
    auto& trv = root.emplace<TreeView>(); trv.setModel(&tree); trv.setSelectionModel(&trs);
    const auto json = encodeBuiltInUiDescriptionJson(root);
    CHECK(json.has_value());
    for (const auto symbol : {"button.command", "radio-button.group", "list-view.model",
        "table-view.model", "tree-view.model", "list-view.selection-model",
        "table-view.selection-model", "tree-view.selection-model", "list-selection-model.model",
        "table-selection-model.model", "tree-selection-model.model"}) {
        const auto token = std::string{"\"kind\": \""} + symbol + '"';
        const auto position = json->find(token);
        CHECK(position != std::string::npos);
        CHECK(json->find(token, position + 1) == std::string::npos);
    }
    CHECK(json->find("SECRET_") == std::string::npos);
    CHECK(json->find("selection\"") == std::string::npos);
    CHECK(json->find("\"properties\": []") != std::string::npos);
}

TEST_CASE("JSON v1 exact decimal integers without floating conversion") {
    GridLayout grid;
    grid.setColumnCount(std::numeric_limits<std::size_t>::max());
    grid.setColumnSpacing(std::numeric_limits<Coordinate>::max());
    grid.setSizeConstraints({{std::numeric_limits<Coordinate>::min(), -12},
                             {12, 34}, {std::numeric_limits<Coordinate>::max(), 100}});
    const auto json = encodeBuiltInUiDescriptionJson(grid);
    CHECK(json.has_value());
    CHECK(json->find("\"width\": -2147483648") != std::string::npos);
    CHECK(json->find("\"height\": -12") != std::string::npos);
    CHECK(json->find("\"value\": 2147483647") != std::string::npos);
    if constexpr (sizeof(std::size_t) == 8) {
        CHECK(json->find("\"value\": 18446744073709551615") != std::string::npos);
    } else {
        CHECK(json->find("\"value\": 4294967295") != std::string::npos);
    }
}

TEST_CASE("JSON v1 capture failures are atomic at public root API") {
    class Unknown final : public Component {} unknown;
    CHECK(!encodeBuiltInUiDescriptionJson(unknown));
    Window root;
    auto& child = root.emplace<Label>();
    CHECK(!encodeBuiltInUiDescriptionJson(child));
    Command external;
    root.emplace<Button>().bindCommand(external);
    CHECK(!encodeBuiltInUiDescriptionJson(root));
}

TEST_CASE("JSON v1 excludes live transient state and does not notify callbacks") {
    Window root;
    auto& field = root.emplace<TextField>("Text");
    auto& combo = root.emplace<ComboBox>(std::vector<std::string>{"One", "Two"});
    const auto before = encodeBuiltInUiDescriptionJson(root);
    CHECK(before.has_value());
    field.setSelection(1, 3);
    (void)root.measure();
    root.arrange({2, 3, 100, 40});
    int notifications = 0;
    FocusManager focus;
    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));
    combo.setOnSelectionChanged([&notifications](auto) { ++notifications; });
    combo.setOnDropDownChanged([&notifications](bool) { ++notifications; });
    const auto after = encodeBuiltInUiDescriptionJson(root);
    CHECK(after == before);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 3);
    CHECK(combo.isDropDownOpen());
    CHECK(notifications == 0);
    // Retire the observations before local counters leave scope or focus teardown closes it.
    combo.setOnSelectionChanged({});
    combo.setOnDropDownChanged({});
}
