#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/text/utf8.hpp>

#include "ui_description_document_validation.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <initializer_list>
#include <string_view>
#include <system_error>
#include <utility>

namespace sasd::ui {
namespace {
using Error = UiDescriptionJsonParseErrorCode;
using Kind = ComponentPropertyValueKind;
using Reference = BuiltInComponentReferenceKind;

constexpr std::array property_symbols{"boolean", "string", "signed-integer", "unsigned-integer",
    "string-collection", "optional-unsigned-integer", "size-constraints", "text-style"};
constexpr std::array property_kinds{Kind::boolean, Kind::string, Kind::signed_integer,
    Kind::unsigned_integer, Kind::string_collection, Kind::optional_unsigned_integer,
    Kind::size_constraints, Kind::text_style};
constexpr std::array color_symbols{"default", "black", "red", "green", "yellow", "blue", "magenta",
    "cyan", "white", "bright-black", "bright-red", "bright-green", "bright-yellow", "bright-blue",
    "bright-magenta", "bright-cyan", "bright-white"};
constexpr std::array colors{Color::default_color, Color::black, Color::red, Color::green,
    Color::yellow, Color::blue, Color::magenta, Color::cyan, Color::white, Color::bright_black,
    Color::bright_red, Color::bright_green, Color::bright_yellow, Color::bright_blue,
    Color::bright_magenta, Color::bright_cyan, Color::bright_white};
struct Relation final {
    std::string_view symbol;
    Reference kind;
    std::string_view source;
    std::string_view target;
};
// This explicit table preserves the established semantic enum order without external ordinals.
constexpr std::array relations{
    Relation{"button.command", Reference::button_command, "sasd.ui.Button", "sasd.ui.Command"},
    Relation{"radio-button.group", Reference::radio_button_group, "sasd.ui.RadioButton", "sasd.ui.RadioGroup"},
    Relation{"list-view.model", Reference::list_view_model, "sasd.ui.ListView", "sasd.ui.ListModel"},
    Relation{"table-view.model", Reference::table_view_model, "sasd.ui.TableView", "sasd.ui.TableModel"},
    Relation{"tree-view.model", Reference::tree_view_model, "sasd.ui.TreeView", "sasd.ui.TreeModel"},
    Relation{"list-view.selection-model", Reference::list_view_selection_model, "sasd.ui.ListView", "sasd.ui.ListSelectionModel"},
    Relation{"table-view.selection-model", Reference::table_view_selection_model, "sasd.ui.TableView", "sasd.ui.TableSelectionModel"},
    Relation{"tree-view.selection-model", Reference::tree_view_selection_model, "sasd.ui.TreeView", "sasd.ui.TreeSelectionModel"},
    Relation{"list-selection-model.model", Reference::list_selection_model_model, "sasd.ui.ListSelectionModel", "sasd.ui.ListModel"},
    Relation{"table-selection-model.model", Reference::table_selection_model_model, "sasd.ui.TableSelectionModel", "sasd.ui.TableModel"},
    Relation{"tree-selection-model.model", Reference::tree_selection_model_model, "sasd.ui.TreeSelectionModel", "sasd.ui.TreeModel"}};

class Parser final {
public:
    explicit Parser(std::string_view input) : input_(input) {}

    BuiltInUiDescriptionJsonParseResult parse() {
        // Validate the entire byte stream first, including bytes after an otherwise valid root.
        // Runtime decoding recovery must never silently alter persisted text.
        for (std::size_t offset = 0; offset < input_.size();) {
            const auto scalar = utf8::decodeOne(input_, offset);
            if (!scalar.valid || scalar.consumed == 0) {
                fail(Error::invalid_utf8, offset);
                return {std::nullopt, error_};
            }
            offset += scalar.consumed;
        }
        object({"format", "formatVersion", "root", "nodes", "references"}, [&](std::size_t member) {
            const auto at = cursor_;
            switch (member) {
            case 0:
                if (string() != "sasd.ui-description") { fail(Error::unsupported_format, at); }
                break;
            case 1:
                if (integer<std::int64_t>(Error::unsupported_format_version) != 1) {
                    fail(Error::unsupported_format_version, at);
                }
                break;
            case 2: document_.root_node_id = integer<ComponentDescriptionNodeId>(Error::invalid_ownership); break;
            case 3: array([&] { parseNode(); }); break;
            case 4: array([&] { parseReference(); }); break;
            default: break;
            }
        });
        whitespace();
        if (cursor_ != input_.size()) { fail(Error::syntax_error, cursor_); }
        if (ok()) {
            const auto validation =
                detail::validateAndNormalizeBuiltInUiDescriptionDocument(document_);
            if (!validation) {
                std::size_t offset = 0;
                if (validation.location == detail::UiDescriptionValidationLocation::node &&
                    validation.input_index < node_offsets_.size()) {
                    offset = node_offsets_[validation.input_index];
                } else if (validation.location ==
                               detail::UiDescriptionValidationLocation::reference &&
                           validation.input_index < reference_offsets_.size()) {
                    offset = reference_offsets_[validation.input_index];
                }
                fail(validation.error, offset);
            }
        }
        if (!ok()) { return {std::nullopt, error_}; }
        return {std::move(document_), {}};
    }

private:
    std::string_view input_;
    std::size_t cursor_{0};
    UiDescriptionJsonParseError error_{};
    BuiltInUiDescriptionDocument document_;
    // Positions travel with input nodes until normalization; semantic failures retain input offsets.
    std::vector<std::size_t> node_offsets_;
    std::vector<std::size_t> reference_offsets_;

    bool ok() const { return error_.code == Error::none; }
    void fail(Error code, std::size_t at) {
        if (ok()) { error_ = {code, at}; }
    }
    char peek() const { return cursor_ < input_.size() ? input_[cursor_] : '\0'; }
    void whitespace() {
        while (cursor_ < input_.size()) {
            const char c = input_[cursor_];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') { break; }
            ++cursor_;
        }
    }
    bool consume(char c) {
        whitespace();
        if (cursor_ < input_.size() && peek() == c) { ++cursor_; return true; }
        return false;
    }
    void expect(char c, Error code = Error::syntax_error) {
        if (!consume(c)) { fail(code, cursor_); }
    }
    template<class Function>
    void array(Function element) {
        expect('[', Error::wrong_value_type);
        if (!ok() || consume(']')) { return; }
        while (ok()) {
            whitespace();
            element();
            if (!ok() || consume(']')) { return; }
            expect(',');
        }
    }
    template<class Function>
    void object(std::initializer_list<std::string_view> names, Function member) {
        expect('{', Error::wrong_value_type);
        std::vector<bool> seen(names.size(), false);
        if (!ok()) { return; }
        if (!consume('}')) {
            while (ok()) {
                whitespace();
                const auto at = cursor_;
                const auto key = string();
                if (!ok()) { return; }
                const auto found = std::find(names.begin(), names.end(), key);
                // Reject before reading a value: unknown nested values never need generic skipping.
                if (found == names.end()) { fail(Error::unknown_member, at); return; }
                const auto index = static_cast<std::size_t>(found - names.begin());
                if (seen[index]) { fail(Error::duplicate_member, at); return; }
                seen[index] = true;
                expect(':');
                whitespace();
                if (!ok()) { return; }
                member(index);
                if (!ok() || consume('}')) { break; }
                expect(',');
            }
        }
        if (ok() && std::find(seen.begin(), seen.end(), false) != seen.end()) {
            fail(Error::missing_member, cursor_);
        }
    }
    char32_t hexQuad() {
        char32_t value = 0;
        for (int i = 0; i < 4 && ok(); ++i) {
            if (cursor_ == input_.size()) { fail(Error::syntax_error, cursor_); break; }
            const char c = input_[cursor_++];
            unsigned digit = 0;
            if (c >= '0' && c <= '9') { digit = static_cast<unsigned>(c - '0'); }
            else if (c >= 'a' && c <= 'f') { digit = static_cast<unsigned>(c - 'a') + 10U; }
            else if (c >= 'A' && c <= 'F') { digit = static_cast<unsigned>(c - 'A') + 10U; }
            else { fail(Error::syntax_error, cursor_ - 1); }
            value = value * 16U + digit;
        }
        return value;
    }
    std::string string() {
        std::string value;
        expect('"', Error::wrong_value_type);
        while (ok() && cursor_ < input_.size()) {
            const auto at = cursor_;
            const auto c = static_cast<unsigned char>(input_[cursor_++]);
            if (c == '"') { return value; }
            if (c < 0x20U) { fail(Error::syntax_error, at); break; }
            if (c != '\\') { value += static_cast<char>(c); continue; }
            if (cursor_ == input_.size()) { break; }
            switch (input_[cursor_++]) {
            case '"': value += '"'; break;
            case '\\': value += '\\'; break;
            case '/': value += '/'; break;
            case 'b': value += '\b'; break;
            case 'f': value += '\f'; break;
            case 'n': value += '\n'; break;
            case 'r': value += '\r'; break;
            case 't': value += '\t'; break;
            case 'u': {
                auto scalar = hexQuad();
                if (scalar >= 0xd800U && scalar <= 0xdbffU) {
                    // JSON UTF-16 escapes require an immediately adjacent low-surrogate escape.
                    if (input_.size() - cursor_ < 2 || input_[cursor_] != '\\' || input_[cursor_ + 1] != 'u') {
                        fail(Error::syntax_error, cursor_); break;
                    }
                    cursor_ += 2;
                    const auto low = hexQuad();
                    if (low < 0xdc00U || low > 0xdfffU) { fail(Error::syntax_error, cursor_); break; }
                    scalar = 0x10000U + (scalar - 0xd800U) * 0x400U + (low - 0xdc00U);
                } else if (scalar >= 0xdc00U && scalar <= 0xdfffU) {
                    fail(Error::syntax_error, cursor_); break;
                }
                if (ok()) { utf8::appendScalar(value, scalar); }
                break;
            }
            default: fail(Error::syntax_error, cursor_ - 1); break;
            }
        }
        fail(Error::syntax_error, cursor_);
        return value;
    }
    void literal(std::string_view token) {
        whitespace();
        if (input_.substr(cursor_, token.size()) != token) { fail(Error::syntax_error, cursor_); return; }
        cursor_ += token.size();
    }
    bool boolean() {
        whitespace();
        if (peek() == 't') { literal("true"); return true; }
        if (peek() == 'f') { literal("false"); return false; }
        fail(Error::wrong_value_type, cursor_);
        return false;
    }
    std::string_view integerToken() {
        whitespace();
        const auto start = cursor_;
        if (peek() == '-') { ++cursor_; }
        if (peek() < '0' || peek() > '9') { fail(Error::wrong_value_type, cursor_); return {}; }
        if (peek() == '0') {
            ++cursor_;
            if (peek() >= '0' && peek() <= '9') { fail(Error::syntax_error, cursor_); }
        } else {
            while (cursor_ < input_.size() && peek() >= '0' && peek() <= '9') { ++cursor_; }
        }
        if (peek() == '.' || peek() == 'e' || peek() == 'E') { fail(Error::wrong_value_type, cursor_); }
        return input_.substr(start, cursor_ - start);
    }
    template<class Integer>
    Integer integer(Error range = Error::invalid_property_value) {
        whitespace();
        const auto at = cursor_;
        const auto token = integerToken();
        Integer value{};
        if (!ok()) { return value; }
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) { fail(range, at); }
        return value;
    }
    // Only a known property's value may precede its kind. Locate that value's end without a DOM;
    // re-read it with the selected concrete schema below. v1 values have at most two nested
    // objects (constraints -> size), or one array of strings. This explicit bound is schema,
    // not a file-size limit; adversarial nesting never grows the call stack without bound.
    void locatePropertyValue(unsigned depth = 0) {
        whitespace();
        if (!ok()) { return; }
        if (peek() == '"') { static_cast<void>(string()); }
        else if (peek() == 't') { literal("true"); }
        else if (peek() == 'f') { literal("false"); }
        else if (peek() == 'n') { literal("null"); }
        else if (peek() == '{' || peek() == '[') {
            if (depth >= 2) { fail(Error::wrong_value_type, cursor_); return; }
            if (consume('[')) { arrayContents(depth); }
            else {
                expect('{');
                if (consume('}')) { return; }
                while (ok()) {
                    static_cast<void>(string()); expect(':'); locatePropertyValue(depth + 1);
                    if (!ok() || consume('}')) { return; }
                    expect(',');
                }
            }
        } else { static_cast<void>(integerToken()); }
    }
    void arrayContents(unsigned depth) {
        if (consume(']')) { return; }
        while (ok()) {
            locatePropertyValue(depth + 1);
            if (!ok() || consume(']')) { return; }
            expect(',');
        }
    }
    Size size() {
        Size result;
        object({"width", "height"}, [&](std::size_t member) {
            const auto value = integer<Coordinate>();
            if (member == 0) { result.width = value; } else { result.height = value; }
        });
        return result;
    }
    SizeConstraints constraints() {
        const auto at = cursor_;
        SizeConstraints result;
        object({"minimum", "preferred", "maximum"}, [&](std::size_t member) {
            const auto value = size();
            if (member == 0) { result.minimum = value; }
            else if (member == 1) { result.preferred = value; }
            else { result.maximum = value; }
        });
        if (!result.hasValidRange()) { fail(Error::invalid_property_value, at); }
        return result;
    }
    TextStyle style() {
        TextStyle result;
        object({"foreground", "bold", "dim", "underline", "inverse"}, [&](std::size_t member) {
            if (member == 0) {
                const auto at = cursor_;
                const auto symbol = string();
                const auto found = std::find(color_symbols.begin(), color_symbols.end(), symbol);
                if (found == color_symbols.end()) { fail(Error::invalid_property_value, at); }
                else { result.foreground = colors[static_cast<std::size_t>(found - color_symbols.begin())]; }
            } else {
                const bool value = boolean();
                if (member == 1) { result.bold = value; }
                else if (member == 2) { result.dim = value; }
                else if (member == 3) { result.underline = value; }
                else { result.inverse = value; }
            }
        });
        return result;
    }
    void property(BuiltInUiDescriptionNode& node) {
        const auto at = cursor_;
        std::string declaring, id, symbol;
        std::size_t value_at = cursor_;
        object({"declaringType", "propertyId", "kind", "value"}, [&](std::size_t member) {
            if (member == 0) { declaring = string(); }
            else if (member == 1) { id = string(); }
            else if (member == 2) { symbol = string(); }
            else { value_at = cursor_; locatePropertyValue(); }
        });
        if (!ok()) { return; }
        const auto found = std::find(property_symbols.begin(), property_symbols.end(), symbol);
        if (found == property_symbols.end()) { fail(Error::property_kind_mismatch, at); return; }
        const auto kind = property_kinds[static_cast<std::size_t>(found - property_symbols.begin())];
        const auto resume = cursor_;
        cursor_ = value_at;
        switch (kind) {
        case Kind::boolean: node.boolean_properties.push_back({declaring, id, boolean()}); break;
        case Kind::string: node.string_properties.push_back({declaring, id, string()}); break;
        case Kind::signed_integer: node.signed_integer_properties.push_back({declaring, id, integer<std::int64_t>()}); break;
        case Kind::unsigned_integer: node.unsigned_integer_properties.push_back({declaring, id, integer<std::uint64_t>()}); break;
        case Kind::string_collection: {
            std::vector<std::string> values;
            array([&] { values.push_back(string()); });
            node.string_collection_properties.push_back({declaring, id, std::move(values)});
            break;
        }
        case Kind::optional_unsigned_integer: {
            std::optional<std::uint64_t> value;
            if (peek() == 'n') { literal("null"); } else { value = integer<std::uint64_t>(); }
            node.optional_unsigned_integer_properties.push_back({declaring, id, value});
            break;
        }
        case Kind::size_constraints: node.size_constraints_properties.push_back({declaring, id, constraints()}); break;
        case Kind::text_style: node.text_style_properties.push_back({declaring, id, style()}); break;
        }
        cursor_ = resume;
    }
    void parseNode() {
        node_offsets_.push_back(cursor_);
        BuiltInUiDescriptionNode node;
        object({"id", "type", "owns", "properties"}, [&](std::size_t member) {
            if (member == 0) { node.node_id = integer<ComponentDescriptionNodeId>(Error::invalid_ownership); }
            else if (member == 1) { node.type_id = string(); }
            else if (member == 2) {
                array([&] { node.owned_component_ids.push_back(integer<ComponentDescriptionNodeId>(Error::invalid_ownership)); });
            } else { array([&] { property(node); }); }
        });
        document_.nodes.push_back(std::move(node));
    }
    void parseReference() {
        reference_offsets_.push_back(cursor_);
        UiDescriptionReferenceEdge edge;
        object({"source", "kind", "target"}, [&](std::size_t member) {
            if (member == 0) { edge.source_node_id = integer<ComponentDescriptionNodeId>(Error::invalid_reference); }
            else if (member == 2) { edge.target_node_id = integer<ComponentDescriptionNodeId>(Error::invalid_reference); }
            else {
                const auto at = cursor_;
                const auto symbol = string();
                const auto found = std::find_if(relations.begin(), relations.end(), [&](const auto& r) { return r.symbol == symbol; });
                if (found == relations.end()) { fail(Error::invalid_reference, at); }
                else { edge.kind = found->kind; }
            }
        });
        document_.reference_edges.push_back(edge);
    }

};
} // namespace

BuiltInUiDescriptionJsonParseResult parseBuiltInUiDescriptionJson(std::string_view json) {
    return Parser{json}.parse();
}
} // namespace sasd::ui
