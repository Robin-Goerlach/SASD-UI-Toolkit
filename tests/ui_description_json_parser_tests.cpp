#include "test_framework.hpp"
#include "ui_description_json_golden.hpp"
#include <sasd/ui/ui_description_document.hpp>
#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/tree_view.hpp>
#include <sasd/ui/tree_selection_model.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>
#include <algorithm>
#include <array>
#include <limits>

using namespace sasd::ui;
namespace {
using Error = UiDescriptionJsonParseErrorCode;
std::string replace(std::string text, std::string_view from, std::string_view to) {
    const auto at = text.find(from);
    CHECK(at != std::string::npos);
    text.replace(at, from.size(), to);
    return text;
}
void rejects(std::string_view text, Error code) {
    const auto result = parseBuiltInUiDescriptionJson(text);
    CHECK(!result);
    CHECK(!result.document);
    CHECK(result.error.code == code);
    CHECK(result.error.byte_offset <= text.size());
}
std::string encoded(const Component& component) {
    const auto json = encodeBuiltInUiDescriptionJson(component);
    CHECK(json);
    return *json;
}
// Test-only token slicing permits schema mutations without a second production JSON API.
std::size_t valueEnd(std::string_view text, std::size_t start) {
    bool quoted = false, escaped = false;
    int nesting = 0;
    for (std::size_t i = start; i < text.size(); ++i) {
        const char c = text[i];
        if (quoted) {
            if (escaped) { escaped = false; }
            else if (c == '\\') { escaped = true; }
            else if (c == '"') { quoted = false; if (nesting == 0) { return i + 1; } }
        } else if (c == '"') { quoted = true; }
        else if (c == '[' || c == '{') { ++nesting; }
        else if (c == ']' || c == '}') { if (nesting == 0) { return i; } --nesting; if (nesting == 0) { return i + 1; } }
        else if (nesting == 0 && (c == ',' || c == '\n')) { return i; }
    }
    return text.size();
}
std::string changeValue(std::string text, std::string_view property, std::string_view value) {
    const auto propertyAt = text.find(std::string{"\"propertyId\": \""} + std::string{property} + '"');
    CHECK(propertyAt != std::string::npos);
    auto start = text.find("\"value\": ", propertyAt);
    CHECK(start != std::string::npos);
    start += 9;
    text.replace(start, valueEnd(text, start) - start, value);
    return text;
}
std::string node(std::string_view type, std::uint64_t id, std::string_view owns = "[]") {
    // Component/RadioGroup/model nodes have no properties; structural Widget nodes use the
    // Label golden's inherited properties, stripping both Label-specific properties.
    std::string properties = "[]";
    if (type == "Window" || type == "Container" || type == "Label") {
        const std::string golden{defaultLabelJson};
        const auto start = golden.find('[', golden.find("\"properties\""));
        properties = golden.substr(start, valueEnd(golden, start) - start);
        if (type != "Label") {
            const auto direct = properties.find("        {\n          \"declaringType\": \"sasd.ui.Label\"");
            CHECK(direct != std::string::npos);
            const auto comma = properties.rfind(',', direct);
            properties.erase(comma);
            properties += ']';
        }
    }
    return "{\"id\":" + std::to_string(id) + ",\"type\":\"sasd.ui." + std::string{type} +
        "\",\"owns\":" + std::string{owns} + ",\"properties\":" + properties + '}';
}
std::string graph(std::string_view nodes, std::string_view edges = "[]", std::uint64_t root = 0) {
    return "{\"format\":\"sasd.ui-description\",\"formatVersion\":1,\"root\":" + std::to_string(root) +
        ",\"nodes\":[" + std::string{nodes} + "],\"references\":" + std::string{edges} + '}';
}
template<class Target, class Source>
void copyValues(Target& target, const Source& source) {
    for (const auto& p : source) { target.push_back({p.declaring_type_id, p.property_id, p.value}); }
    std::sort(target.begin(), target.end(), [](const auto& a, const auto& b) {
        if (a.declaring_type_id != b.declaring_type_id) { return a.declaring_type_id < b.declaring_type_id; }
        return a.property_id < b.property_id;
    });
}
BuiltInUiDescriptionDocument expected(const Component& component) {
    const auto capture = captureBuiltInComponentReferenceGraphSnapshot(component);
    CHECK(capture);
    BuiltInUiDescriptionDocument result;
    result.root_node_id = capture->ownership.root_node_id;
    for (const auto& captured : capture->ownership.nodes) {
        BuiltInUiDescriptionNode n;
        n.node_id = captured.node_id;
        n.type_id = captured.properties.type_id;
        n.owned_component_ids = captured.owned_component_ids;
        const auto& p = captured.properties;
        copyValues(n.boolean_properties, p.boolean_properties);
        copyValues(n.string_properties, p.string_properties);
        copyValues(n.signed_integer_properties, p.signed_integer_properties);
        copyValues(n.unsigned_integer_properties, p.unsigned_integer_properties);
        copyValues(n.string_collection_properties, p.string_collection_properties);
        copyValues(n.optional_unsigned_integer_properties, p.optional_unsigned_integer_properties);
        copyValues(n.size_constraints_properties, p.size_constraints_properties);
        copyValues(n.text_style_properties, p.text_style_properties);
        result.nodes.push_back(std::move(n));
    }
    for (const auto& e : capture->reference_edges) {
        result.reference_edges.push_back({e.source_node_id, e.target_node_id, e.kind});
    }
    return result;
}
void roundtrip(const Component& component) {
    const auto result = parseBuiltInUiDescriptionJson(encoded(component));
    CHECK(result);
    CHECK(result.error.code == Error::none);
    CHECK(*result.document == expected(component));
}
// Permute every object's members and every non-ownership array. IDs and owns order remain intact.
std::string permuted(std::string_view input, bool preserveArray = false) {
    std::size_t first = input.find_first_not_of(" \r\n\t");
    if (first == std::string_view::npos) { return {}; }
    input.remove_prefix(first);
    if (input.front() != '{' && input.front() != '[') { return std::string{input}; }
    const bool object = input.front() == '{';
    std::vector<std::string> parts;
    std::size_t at = 1;
    while (at < input.size()) {
        at = input.find_first_not_of(" \r\n\t,", at);
        if (at == std::string_view::npos || input[at] == '}' || input[at] == ']') { break; }
        if (object) {
            const auto keyEnd = valueEnd(input, at);
            const auto key = input.substr(at, keyEnd - at);
            at = input.find_first_not_of(" \r\n\t:", keyEnd);
            const auto end = valueEnd(input, at);
            parts.push_back(std::string{key} + " : " + permuted(input.substr(at, end - at), key == "\"owns\"" || key == "\"value\""));
            at = end;
        } else {
            const auto end = valueEnd(input, at);
            parts.push_back(permuted(input.substr(at, end - at)));
            at = end;
        }
    }
    if (object || !preserveArray) { std::reverse(parts.begin(), parts.end()); }
    std::string output = object ? "{ \t" : "[ \r\n";
    for (std::size_t i = 0; i < parts.size(); ++i) { if (i) { output += " , "; } output += parts[i]; }
    output += object ? " }" : " ]";
    return output;
}
}

TEST_CASE("JSON reader normative Label and owning input independence") {
    Label label;
    auto text = std::string{defaultLabelJson};
    auto result = parseBuiltInUiDescriptionJson(text);
    CHECK(result);
    CHECK(result.error.code == Error::none);
    CHECK(result.document->root_node_id == 0);
    CHECK(result.document->nodes.size() == 1);
    CHECK(result.document->reference_edges.empty());
    CHECK(*result.document == expected(label));
    text.assign(10000, 'x');
    text.clear(); text.shrink_to_fit();
    CHECK(*result.document == expected(label));
}

TEST_CASE("JSON reader comprehensive golden and eight concrete property collections") {
    const auto parsed = parseBuiltInUiDescriptionJson(comprehensiveJson);
    CHECK(parsed);
    Window root;
    auto& command = root.emplace<Command>("Save");
    auto& box = root.emplace<VBox>(); box.setSpacing(12);
    auto& label = box.emplace<Label>("Name");
    label.setTextStyle({Color::bright_cyan, true, false, true, false});
    label.setSizeConstraints({{-12, -3}, {40, 20}, {100, 50}});
    box.emplace<Button>("Save").bindCommand(command);
    auto& combo = box.emplace<ComboBox>(std::vector<std::string>{"One", "Two"});
    CHECK(combo.setSelectedIndex(1));
    box.emplace<GridLayout>(3);
    CHECK(*parsed.document == expected(root));
    roundtrip(root);
    CHECK(parsed.document->nodes[2].signed_integer_properties.front().value == 12);
    CHECK(parsed.document->nodes[5].optional_unsigned_integer_properties.front().value == 1);
    CHECK(parsed.document->nodes[6].unsigned_integer_properties.front().value == 3);
    // Reordering includes value before kind and properties before node type.
    const auto reordered = parseBuiltInUiDescriptionJson(permuted(comprehensiveJson));
    CHECK(reordered);
    CHECK(reordered.document == parsed.document);
}

TEST_CASE("JSON reader all 17 color symbols and invalid aliases") {
    const std::array colors{Color::default_color, Color::black, Color::red, Color::green,
        Color::yellow, Color::blue, Color::magenta, Color::cyan, Color::white,
        Color::bright_black, Color::bright_red, Color::bright_green, Color::bright_yellow,
        Color::bright_blue, Color::bright_magenta, Color::bright_cyan, Color::bright_white};
    const std::array symbols{"default", "black", "red", "green", "yellow", "blue", "magenta",
        "cyan", "white", "bright-black", "bright-red", "bright-green", "bright-yellow",
        "bright-blue", "bright-magenta", "bright-cyan", "bright-white"};
    for (std::size_t i = 0; i < colors.size(); ++i) {
        const auto result = parseBuiltInUiDescriptionJson(replace(std::string{defaultLabelJson}, "\"default\"", '"' + std::string{symbols[i]} + '"'));
        CHECK(result);
        CHECK(result.document->nodes.front().text_style_properties.front().value.foreground == colors[i]);
    }
    for (const auto symbol : {"\"bright_cyan\"", "\"DEFAULT\"", "\"unknown\""}) {
        rejects(replace(std::string{defaultLabelJson}, "\"default\"", symbol), Error::invalid_property_value);
    }
    rejects(replace(std::string{defaultLabelJson}, "\"default\"", "17"), Error::wrong_value_type);
}

TEST_CASE("JSON reader strict members on every object level") {
    const std::string golden{defaultLabelJson};
    for (const auto& mutation : std::array{
        std::pair{"\"format\":", "\"format\":\"sasd.ui-description\",\"format\":"},
        std::pair{"\"id\":", "\"id\":0,\"id\":"},
        std::pair{"\"propertyId\":", "\"propertyId\":\"visible\",\"propertyId\":"},
        std::pair{"\"bold\":", "\"bold\":false,\"bold\":"},
        std::pair{"\"minimum\":", "\"minimum\":{\"width\":0,\"height\":0},\"minimum\":"},
        std::pair{"\"width\":", "\"width\":0,\"width\":"}}) {
        rejects(replace(golden, mutation.first, mutation.second), Error::duplicate_member);
    }
    for (const auto& mutation : std::array{
        std::pair{"\"format\":", "\"generator\":\"x\",\"format\":"},
        std::pair{"\"id\":", "\"name\":\"foo\",\"id\":"},
        std::pair{"\"propertyId\":", "\"isDefault\":true,\"propertyId\":"},
        std::pair{"\"bold\":", "\"background\":\"red\",\"bold\":"},
        std::pair{"\"minimum\":", "\"extra\":0,\"minimum\":"},
        std::pair{"\"width\":", "\"depth\":0,\"width\":"}}) {
        rejects(replace(golden, mutation.first, mutation.second), Error::unknown_member);
    }
    // Unknown members fail before attempting to skip their arbitrarily nested values.
    rejects(replace(golden, "\"format\":", "\"unknown\":[[[[[[[[[[[[[[[[[[[[[[[[["), Error::unknown_member);
    for (const auto key : {"format", "formatVersion", "root", "nodes", "references", "type", "properties",
                            "kind", "value", "maximum", "inverse"}) {
        auto text = golden;
        const auto start = text.find('"' + std::string{key} + '"');
        CHECK(start != std::string::npos);
        const auto value = text.find_first_not_of(" \r\n\t:", start + std::string_view{key}.size() + 2);
        auto end = valueEnd(text, value);
        while (end < text.size() && (text[end] == ' ' || text[end] == '\r' || text[end] == '\n')) { ++end; }
        if (text[end] == ',') { text.erase(start, end - start + 1); }
        else { const auto comma = text.rfind(',', start); text.erase(comma, end - comma); }
        rejects(text, Error::missing_member);
    }
}

TEST_CASE("JSON reader format version grammar and byte positions") {
    const std::string golden{defaultLabelJson};
    rejects(replace(golden, "sasd.ui-description", "other"), Error::unsupported_format);
    for (const auto version : {"0", "2", "-1", "9223372036854775808"}) {
        const auto text = replace(golden, "\"formatVersion\": 1", std::string{"\"formatVersion\": "} + version);
        const auto result = parseBuiltInUiDescriptionJson(text);
        CHECK(result.error.code == Error::unsupported_format_version);
        CHECK(result.error.byte_offset == text.find(version, text.find("formatVersion")));
    }
    for (const auto version : {"\"1\"", "1.0", "1e0"}) {
        rejects(replace(golden, "\"formatVersion\": 1", std::string{"\"formatVersion\": "} + version), Error::wrong_value_type);
    }
    for (const auto bad : {"", "{", "{}x", "//comment\n{}", "{'format':1}", "NaN", "Infinity"}) {
        CHECK(!parseBuiltInUiDescriptionJson(bad));
    }
    CHECK(!parseBuiltInUiDescriptionJson(golden + "x"));
    CHECK(!parseBuiltInUiDescriptionJson(replace(golden, "\"root\": 0", "\"root\": +1")));
    rejects(replace(golden, "\"root\": 0", "\"root\": 01"), Error::syntax_error);
    CHECK(!parseBuiltInUiDescriptionJson(replace(golden, "\"references\": []", "\"references\": [],")));
    const auto duplicate = replace(golden, "\"formatVersion\":", "\"format\":\"x\",\"formatVersion\":");
    CHECK(parseBuiltInUiDescriptionJson(duplicate).error.byte_offset == duplicate.find("\"format\"", duplicate.find("\"format\"") + 1));
    auto incomplete = golden.substr(0, golden.size() - 3);
    CHECK(parseBuiltInUiDescriptionJson(incomplete).error.byte_offset == incomplete.size());
    const auto trailing = parseBuiltInUiDescriptionJson(golden + "x");
    CHECK(trailing.error.code == Error::syntax_error);
    CHECK(trailing.error.byte_offset == golden.size());
}

TEST_CASE("JSON reader exact Unicode decoding and strict UTF-8") {
    const std::string golden{defaultLabelJson};
    const auto text = changeValue(golden, "text", R"("ASCII \" \\ \/ \b\f\n\r\t\u0000 \u00e4 \uD83D\uDE00")");
    const auto parsed = parseBuiltInUiDescriptionJson(text);
    CHECK(parsed);
    CHECK(parsed.document->nodes[0].string_properties[0].value == std::string{"ASCII \" \\ / \b\f\n\r\t\0 \xc3\xa4 \xf0\x9f\x98\x80", 26});
    Label label{"\xc3\xa4\xc3\xb6\xc3\xbc \xf0\x9f\x98\x80"}; roundtrip(label);
    for (const auto invalid : {"\x80", "\xc0\xaf", "\xe2\x82", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xff"}) {
        rejects(changeValue(golden, "text", '"' + std::string{invalid} + '"'), Error::invalid_utf8);
        rejects(golden + invalid, Error::invalid_utf8);
    }
    for (const auto invalid : {R"("\uD800")", R"("\uDC00")", R"("\uD800\u0041")", R"("\u00xz")", R"("\u000")", R"("\x00")"}) {
        rejects(changeValue(golden, "text", invalid), Error::syntax_error);
    }
    rejects(changeValue(golden, "text", "\"a\nb\""), Error::syntax_error);
    CHECK(!parseBuiltInUiDescriptionJson(std::string{"\xef\xbb\xbf"} + golden));
    CHECK(!parseBuiltInUiDescriptionJson(changeValue(golden, "text", std::string(20000, '['))));
    // No normalization: decomposed and composed strings retain different byte sequences.
    CHECK(parseBuiltInUiDescriptionJson(changeValue(golden, "text", "\"e\\u0301\"")).document->nodes[0].string_properties[0].value == "e\xcc\x81");
}

TEST_CASE("JSON reader every property kind rejects wrong types and schema mismatches") {
    Label label; VBox box; GridLayout grid; ComboBox combo;
    const std::array cases{
        std::pair{encoded(label), std::string{"visible"}}, std::pair{encoded(label), std::string{"text"}},
        std::pair{encoded(box), std::string{"spacing"}}, std::pair{encoded(grid), std::string{"columnCount"}},
        std::pair{encoded(combo), std::string{"items"}}, std::pair{encoded(combo), std::string{"selectedIndex"}},
        std::pair{encoded(label), std::string{"sizeConstraints"}}, std::pair{encoded(label), std::string{"textStyle"}}};
    const std::array wrong{"0", "false", "false", "false", "false", "false", "false", "false"};
    for (std::size_t i = 0; i < cases.size(); ++i) {
        CHECK(parseBuiltInUiDescriptionJson(cases[i].first));
        rejects(changeValue(cases[i].first, cases[i].second, wrong[i]), Error::wrong_value_type);
        auto changed = cases[i].first;
        auto at = changed.find("\"kind\":", changed.find("\"propertyId\": \"" + cases[i].second + '"'));
        auto start = changed.find('"', at + 7);
        auto end = changed.find('"', start + 1);
        changed.replace(start, end - start + 1, "\"unknown\"");
        rejects(changed, Error::property_kind_mismatch);
        changed.replace(start, 9, "\"boolean\"");
        // A schema-compatible JSON value is necessary to isolate descriptor-kind mismatch.
        changed = changeValue(changed, cases[i].second, "true");
        if (i != 0) { rejects(changed, Error::property_kind_mismatch); }
    }
    rejects(changeValue(encoded(combo), "items", "[\"a\",1]"), Error::wrong_value_type);
    CHECK(parseBuiltInUiDescriptionJson(changeValue(encoded(combo), "items", "[\"\",\"A\",\"A\"]")));
    rejects(changeValue(encoded(combo), "selectedIndex", "-1"), Error::invalid_property_value);
    rejects(changeValue(encoded(combo), "selectedIndex", "18446744073709551616"), Error::invalid_property_value);
}

TEST_CASE("JSON reader enforces static integer metadata at semantic boundaries") {
    HBox hbox;
    VBox vbox;
    GridLayout grid;
    FormLayout form;
    const std::array signed_cases{
        std::pair{encoded(hbox), std::string_view{"spacing"}},
        std::pair{encoded(vbox), std::string_view{"spacing"}},
        std::pair{encoded(grid), std::string_view{"columnSpacing"}},
        std::pair{encoded(grid), std::string_view{"rowSpacing"}},
        std::pair{encoded(form), std::string_view{"columnSpacing"}},
        std::pair{encoded(form), std::string_view{"rowSpacing"}},
    };
    const auto coordinate_maximum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());
    const auto coordinate_minimum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min());
    const auto maximum_text = std::to_string(coordinate_maximum);
    const auto above_maximum_text = std::to_string(coordinate_maximum + 1);
    const auto minimum_text = std::to_string(coordinate_minimum);
    for (const auto& [json, property_id] : signed_cases) {
        CHECK(parseBuiltInUiDescriptionJson(changeValue(json, property_id, "0")));
        CHECK(parseBuiltInUiDescriptionJson(changeValue(json, property_id, maximum_text)));
        rejects(changeValue(json, property_id, "-1"), Error::invalid_property_value);
        rejects(changeValue(json, property_id, minimum_text), Error::invalid_property_value);
        rejects(changeValue(json, property_id, above_maximum_text), Error::invalid_property_value);
    }

    // JSON integer carriers remain fixed-width even though each property's semantic domain is
    // narrower. Values outside int64_t/uint64_t still fail during number parsing.
    for (const auto number : {"-9223372036854775809", "9223372036854775808"}) {
        rejects(changeValue(encoded(vbox), "spacing", number), Error::invalid_property_value);
    }
    rejects(changeValue(encoded(grid), "columnCount", "0"), Error::invalid_property_value);
    CHECK(parseBuiltInUiDescriptionJson(changeValue(encoded(grid), "columnCount", "1")));
    const auto maximum = parseBuiltInUiDescriptionJson(
        changeValue(encoded(grid), "columnCount", "18446744073709551615"));
    CHECK(maximum);
    CHECK(maximum.document->nodes[0].unsigned_integer_properties[0].value == std::numeric_limits<std::uint64_t>::max());
    for (const auto number : {"-1", "18446744073709551616"}) {
        rejects(changeValue(encoded(grid), "columnCount", number), Error::invalid_property_value);
    }
    for (const auto number : {"01", "+1", "1.0", "1e0", "NaN", "Infinity"}) {
        CHECK(!parseBuiltInUiDescriptionJson(changeValue(encoded(vbox), "spacing", number)));
    }
}

TEST_CASE("JSON reader ownership dense preorder and all invalid graph shapes") {
    CHECK(parseBuiltInUiDescriptionJson(graph(node("Window", 0, "[1,2]") + ',' + node("Label", 1) + ',' + node("Component", 2))));
    const std::array bad{
        graph(""), graph(node("Component", 0), "[]", 1),
        graph(node("Window", 0, "[1]") + ',' + node("Component", 0)), // duplicate ID
        graph(node("Window", 0, "[2]") + ',' + node("Component", 2)), // gap
        graph(node("Window", 0, "[0]")),
        graph(node("Window", 0) + ',' + node("Component", 1)), // orphan
        graph(node("Window", 0, "[1,2]") + ',' + node("Container", 1, "[2]") + ',' + node("Component", 2)),
        graph(node("Window", 0, "[1,1]") + ',' + node("Component", 1)),
        graph(node("Window", 0, "[1]") + ',' + node("Container", 1, "[1]")),
        graph(node("Window", 0) + ',' + node("Container", 1, "[2]") + ',' + node("Container", 2, "[1]")),
        graph(node("Label", 0, "[1]") + ',' + node("Component", 1)),
        graph(node("Window", 0, "[99]")),
        graph(node("Window", 0, "[2,1]") + ',' + node("Component", 1) + ',' + node("Component", 2))};
    for (const auto& text : bad) { rejects(text, Error::invalid_ownership); }
    rejects(replace(std::string{defaultLabelJson}, "sasd.ui.Label", "sasd.ui.unknown"), Error::unknown_component_type);
}

TEST_CASE("JSON reader property identity completeness and unsupported state") {
    const std::string golden{defaultLabelJson};
    for (const auto& mutation : std::array{
        std::pair{"\"visible\"", "\"bogus\""}, std::pair{"\"enabled\"", "\"visible\""},
        std::pair{"\"sasd.ui.Widget\"", "\"sasd.ui.Label\""},
        std::pair{"\"sasd.ui.Label\",\n          \"propertyId\": \"text\"", "\"sasd.ui.Button\",\n          \"propertyId\": \"text\""}}) {
        rejects(replace(golden, mutation.first, mutation.second), Error::unknown_property);
    }
    for (const auto property : {"visible", "text"}) {
        auto text = golden;
        const auto id = text.find(std::string{"\"propertyId\": \""} + property + '"');
        const auto start = text.rfind('{', id);
        const auto end = valueEnd(text, start);
        auto comma = text.find(',', end);
        CHECK(comma != std::string::npos);
        text.erase(start, comma - start + 1);
        rejects(text, Error::missing_member);
    }
    const auto id = golden.find("\"propertyId\": \"text\"");
    const auto start = golden.rfind('{', id);
    const auto end = valueEnd(golden, start);
    auto extra = golden;
    extra.insert(start, golden.substr(start, end - start) + ',');
    rejects(extra, Error::unknown_property);
}

TEST_CASE("JSON reader ComboBox GridLayout SizeConstraints and TextField invariants") {
    ComboBox combo{std::vector<std::string>{"A"}};
    const auto base = encoded(combo);
    CHECK(parseBuiltInUiDescriptionJson(changeValue(base, "selectedIndex", "null")));
    CHECK(parseBuiltInUiDescriptionJson(changeValue(base, "selectedIndex", "0")));
    rejects(changeValue(base, "selectedIndex", "1"), Error::invalid_property_value);
    GridLayout grid;
    rejects(changeValue(encoded(grid), "columnCount", "0"), Error::invalid_property_value);
    CHECK(parseBuiltInUiDescriptionJson(changeValue(encoded(grid), "columnCount", "1")));
    Label label;
    rejects(replace(encoded(label), "\"width\": 0", "\"width\": 2147483648"), Error::invalid_property_value);
    rejects(replace(encoded(label), "\"height\": 2147483647", "\"height\": -1"), Error::invalid_property_value);
    rejects(replace(encoded(label), "\"width\": 2147483647", "\"width\": -1"), Error::invalid_property_value);
    label.setSizeConstraints({{-20,-10}, {500,-500}, {0,0}}); roundtrip(label);
    CHECK(parseBuiltInUiDescriptionJson(changeValue(
        encoded(label), "sizeConstraints",
        R"({"minimum":{"width":0,"height":0},"preferred":{"width":10,"height":10},"maximum":{"width":100,"height":100}})")));
    CHECK(parseBuiltInUiDescriptionJson(changeValue(
        encoded(label), "sizeConstraints",
        R"({"minimum":{"width":-20,"height":-10},"preferred":{"width":500,"height":-500},"maximum":{"width":0,"height":0}})")));
    rejects(changeValue(
        encoded(label), "sizeConstraints",
        R"({"minimum":{"width":1,"height":0},"preferred":{"width":0,"height":0},"maximum":{"width":0,"height":0}})"),
        Error::invalid_property_value);
    rejects(changeValue(
        encoded(label), "sizeConstraints",
        R"({"minimum":{"width":0,"height":1},"preferred":{"width":0,"height":0},"maximum":{"width":0,"height":0}})"),
        Error::invalid_property_value);
    TextField field;
    for (const auto text : {R"("a\nb")", R"("a\tb")", R"("a\u0001b")", R"("a\u0085b")",
                            R"("a\u2028b")", R"("a\u2029b")"}) {
        rejects(changeValue(encoded(field), "text", text), Error::invalid_property_value);
    }
    CHECK(parseBuiltInUiDescriptionJson(changeValue(
        encoded(field), "text", "\"Gr\\u00fc\\u00dfe \\uD83D\\uDE00\"")));
    CHECK(parseBuiltInUiDescriptionJson(changeValue(encoded(field), "text", "\"e\\u0301\"")));

    // Normalization belongs to the exact TextField.text descriptor. The other built-in text
    // setters retain valid UTF-8 controls and line breaks, so persistence must retain them too.
    Command command;
    Button button;
    CheckBox check_box;
    RadioButton radio;
    for (const auto& json : {encoded(command), encoded(label), encoded(button),
                             encoded(check_box), encoded(radio)}) {
        CHECK(parseBuiltInUiDescriptionJson(changeValue(json, "text", R"("line\n\u0001\u2028")")));
    }
    const auto invalid = changeValue(encoded(grid), "columnCount", "0");
    const auto position = parseBuiltInUiDescriptionJson(invalid).error.byte_offset;
    CHECK(position >= invalid.find("\"nodes\""));
    CHECK(position < invalid.find("\"references\""));
}

TEST_CASE("JSON reader all eleven relation symbols types and normalization") {
    Window root;
    auto& command = root.emplace<Command>(); root.emplace<Button>().bindCommand(command);
    auto& group = root.emplace<RadioGroup>(); root.emplace<RadioButton>(group, "Choice");
    auto& list = root.emplace<StringListModel>();
    auto& other = root.emplace<StringListModel>();
    auto& ls = root.emplace<ListSelectionModel>(); ls.setModel(&other);
    auto& lv = root.emplace<ListView>(); lv.setModel(&list); lv.setSelectionModel(&ls);
    auto& table = root.emplace<StringTableModel>();
    auto& ts = root.emplace<TableSelectionModel>(); ts.setModel(&table);
    auto& tv = root.emplace<TableView>(); tv.setModel(&table); tv.setSelectionModel(&ts);
    auto& tree = root.emplace<StringTreeModel>();
    auto& trs = root.emplace<TreeSelectionModel>(); trs.setModel(&tree);
    auto& trv = root.emplace<TreeView>(); trv.setModel(&tree); trv.setSelectionModel(&trs);
    roundtrip(root); // Deliberately independent view.model and selection.model targets.
    const auto json = encoded(root);
    const auto parsed = parseBuiltInUiDescriptionJson(json);
    CHECK(parsed);
    CHECK(parsed.document->reference_edges.size() == 11);
    CHECK(parsed.document == parseBuiltInUiDescriptionJson(permuted(json)).document);
    const std::array kinds{BuiltInComponentReferenceKind::button_command, BuiltInComponentReferenceKind::radio_button_group,
        BuiltInComponentReferenceKind::list_view_model, BuiltInComponentReferenceKind::table_view_model,
        BuiltInComponentReferenceKind::tree_view_model, BuiltInComponentReferenceKind::list_view_selection_model,
        BuiltInComponentReferenceKind::table_view_selection_model, BuiltInComponentReferenceKind::tree_view_selection_model,
        BuiltInComponentReferenceKind::list_selection_model_model, BuiltInComponentReferenceKind::table_selection_model_model,
        BuiltInComponentReferenceKind::tree_selection_model_model};
    for (const auto kind : kinds) {
        const auto& edges = parsed.document->reference_edges;
        CHECK(std::count_if(edges.begin(), edges.end(), [&](const auto& edge) { return edge.kind == kind; }) == 1);
    }
    // Mutate every relation independently, checking source/target compatibility for each kind.
    const auto refs = json.find("\"references\"");
    auto at = json.find('{', refs);
    while (at != std::string::npos) {
        const auto end = valueEnd(json, at);
        const auto edge = json.substr(at, end - at);
        for (const auto key : {"source", "target"}) {
            const auto value = edge.find_first_not_of(" \r\n\t:", edge.find(key) + std::string_view{key}.size() + 1);
            const auto stop = edge.find_first_not_of("0123456789", value);
            auto bad = json;
            bad.replace(at + value, stop - value, "0");
            rejects(bad, Error::invalid_reference);
            bad = json; bad.replace(at + value, stop - value, "999");
            rejects(bad, Error::invalid_reference);
        }
        auto duplicate = json; duplicate.insert(at, edge + ','); rejects(duplicate, Error::invalid_reference);
        at = json.find('{', end);
    }
    rejects(replace(json, "button.command", "button_command"), Error::invalid_reference);
    auto missing = json;
    auto targetAt = missing.find("\"target\"", refs);
    auto before = missing.rfind(',', targetAt);
    auto end = missing.find('}', targetAt);
    missing.erase(before, end - before);
    rejects(missing, Error::missing_member);
    auto duplicate = json;
    duplicate.insert(duplicate.find("\"target\"", refs), "\"target\":1,");
    rejects(duplicate, Error::duplicate_member);
    auto unknown = json;
    unknown.insert(unknown.find("\"target\"", refs), "\"pointer\":1,");
    rejects(unknown, Error::unknown_member);
    // A self-edge is rejected even before endpoint type compatibility is considered.
    rejects(graph(node("Window", 0), R"([{"source":0,"kind":"button.command","target":0}])"), Error::invalid_reference);
}

TEST_CASE("JSON reader RadioGroup exclusivity and standalone selected values") {
    Window root;
    auto& group = root.emplace<RadioGroup>();
    auto& first = root.emplace<RadioButton>(group, "A");
    root.emplace<RadioButton>(group, "B");
    roundtrip(root); // both false
    first.setSelected(true); roundtrip(root);
    auto json = encoded(root);
    // Replace the last selected false, belonging to the second group member.
    const auto propertyAt = json.rfind("\"propertyId\": \"selected\"");
    const auto value = json.find("false", propertyAt);
    CHECK(value != std::string::npos);
    json.replace(value, 5, "true");
    rejects(json, Error::invalid_reference);
    Window standalone;
    standalone.emplace<RadioButton>("A").setSelected(true);
    standalone.emplace<RadioButton>("B").setSelected(true);
    roundtrip(standalone);
}

TEST_CASE("JSON reader boolean kind mismatch escaped keys and iterative deep ownership") {
    rejects(changeValue(replace(std::string{defaultLabelJson}, "\"kind\": \"boolean\"", "\"kind\": \"string\""),
                        "visible", "\"true\""), Error::property_kind_mismatch);
    rejects(replace(std::string{defaultLabelJson}, "\"formatVersion\":", R"("\u0066ormat":"x","formatVersion":)"), Error::duplicate_member);
    std::string nodes;
    constexpr std::uint64_t depth = 1024;
    for (std::uint64_t i = 0; i < depth; ++i) {
        if (i) { nodes += ','; }
        std::string children{"[]"};
        if (i + 1 != depth) {
            children = '[';
            children += std::to_string(i + 1);
            children += ']';
        }
        nodes += node("Container", i, children);
    }
    const auto result = parseBuiltInUiDescriptionJson(graph(nodes));
    CHECK(result);
    CHECK(result.document->nodes.size() == depth);
}
