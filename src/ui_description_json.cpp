#include <sasd/ui/ui_description_json.hpp>

#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_reference_snapshot.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <charconv>
#include <string_view>
#include <system_error>
#include <utility>

namespace sasd::ui {
namespace {

// Symbols are a versioned external contract, independent of C++ enum ordinals.
std::string_view colorSymbol(Color color) {
    switch (color) {
    case Color::default_color: return "default";
    case Color::black: return "black";
    case Color::red: return "red";
    case Color::green: return "green";
    case Color::yellow: return "yellow";
    case Color::blue: return "blue";
    case Color::magenta: return "magenta";
    case Color::cyan: return "cyan";
    case Color::white: return "white";
    case Color::bright_black: return "bright-black";
    case Color::bright_red: return "bright-red";
    case Color::bright_green: return "bright-green";
    case Color::bright_yellow: return "bright-yellow";
    case Color::bright_blue: return "bright-blue";
    case Color::bright_magenta: return "bright-magenta";
    case Color::bright_cyan: return "bright-cyan";
    case Color::bright_white: return "bright-white";
    }
    // Runtime accepts unknown Color values; persistence cannot invent their meaning.
    return {};
}

std::string_view referenceSymbol(BuiltInComponentReferenceKind kind) {
    switch (kind) {
    case BuiltInComponentReferenceKind::button_command: return "button.command";
    case BuiltInComponentReferenceKind::radio_button_group: return "radio-button.group";
    case BuiltInComponentReferenceKind::list_view_model: return "list-view.model";
    case BuiltInComponentReferenceKind::table_view_model: return "table-view.model";
    case BuiltInComponentReferenceKind::tree_view_model: return "tree-view.model";
    case BuiltInComponentReferenceKind::list_view_selection_model: return "list-view.selection-model";
    case BuiltInComponentReferenceKind::table_view_selection_model: return "table-view.selection-model";
    case BuiltInComponentReferenceKind::tree_view_selection_model: return "tree-view.selection-model";
    case BuiltInComponentReferenceKind::list_selection_model_model: return "list-selection-model.model";
    case BuiltInComponentReferenceKind::table_selection_model_model: return "table-selection-model.model";
    case BuiltInComponentReferenceKind::tree_selection_model_model: return "tree-selection-model.model";
    }
    return {};
}

// This is a specialized writer for a fixed document, not a JSON DOM or public snapshot validator.
class Writer final {
public:
    std::optional<std::string> encode(const BuiltInComponentReferenceGraphSnapshot& graph) {
        output_ = "{\n  \"format\": \"sasd.ui-description\",\n  \"formatVersion\": 1,\n  \"root\": ";
        integer(graph.ownership.root_node_id);
        output_ += ",\n  \"nodes\": ";
        array(graph.ownership.nodes, 2, [this](const auto& node) { writeNode(node); });
        output_ += ",\n  \"references\": ";
        array(graph.reference_edges, 2, [this](const auto& edge) {
            const auto symbol = referenceSymbol(edge.kind);
            if (symbol.empty()) { valid_ = false; }
            output_ += "{\n      \"source\": "; integer(edge.source_node_id);
            output_ += ",\n      \"kind\": "; string(symbol);
            output_ += ",\n      \"target\": "; integer(edge.target_node_id);
            output_ += "\n    }";
        });
        output_ += "\n}\n";
        if (!valid_) { return std::nullopt; }
        return std::move(output_);
    }

private:
    std::string output_;
    bool valid_{true};

    void indent(std::size_t spaces) { output_.append(spaces, ' '); }

    template<class Integer>
    void integer(Integer value) {
        // Enough for any int64/uint64, including sign; no locale or floating conversion.
        char buffer[32];
        const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (result.ec != std::errc{}) { valid_ = false; return; }
        output_.append(buffer, result.ptr);
    }

    void string(std::string_view value) {
        // decodeOne's recovery is useful at runtime, but replacing invalid bytes here would
        // silently change persisted semantics. Validate first and reject the whole document.
        for (std::size_t offset = 0; offset < value.size();) {
            const auto scalar = utf8::decodeOne(value, offset);
            if (!scalar.valid || scalar.consumed == 0) { valid_ = false; return; }
            offset += scalar.consumed;
        }
        constexpr std::string_view hex = "0123456789abcdef";
        output_ += '"';
        for (const char byte : value) {
            switch (byte) {
            case '"': output_ += "\\\""; break;
            case '\\': output_ += "\\\\"; break;
            case '\b': output_ += "\\b"; break;
            case '\f': output_ += "\\f"; break;
            case '\n': output_ += "\\n"; break;
            case '\r': output_ += "\\r"; break;
            case '\t': output_ += "\\t"; break;
            default: {
                const auto code = static_cast<unsigned char>(byte);
                if (code < 0x20) {
                    output_ += "\\u00";
                    output_ += hex[code >> 4];
                    output_ += hex[code & 0x0f];
                } else { output_ += byte; }
                break;
            }
            }
        }
        output_ += '"';
    }

    template<class Values, class Emit>
    void array(const Values& values, std::size_t spaces, Emit emit) {
        output_ += '[';
        bool first = true;
        for (const auto& value : values) {
            if (!first) { output_ += ','; }
            output_ += '\n'; indent(spaces + 2); emit(value); first = false;
        }
        if (!first) { output_ += '\n'; indent(spaces); }
        output_ += ']';
    }

    void value(bool v) { output_ += v ? "true" : "false"; }
    void value(const std::string& v) { string(v); }
    void value(std::int64_t v) { integer(v); }
    void value(std::uint64_t v) { integer(v); }
    void value(const std::optional<std::uint64_t>& v) {
        if (v) { integer(*v); } else { output_ += "null"; }
    }
    void value(const std::vector<std::string>& v) {
        array(v, 10, [this](const auto& text) { string(text); });
    }
    void size(Size v) {
        output_ += "{\n              \"width\": "; integer(v.width);
        output_ += ",\n              \"height\": "; integer(v.height);
        output_ += "\n            }";
    }
    void value(const SizeConstraints& v) {
        output_ += "{\n            \"minimum\": "; size(v.minimum);
        output_ += ",\n            \"preferred\": "; size(v.preferred);
        output_ += ",\n            \"maximum\": "; size(v.maximum);
        output_ += "\n          }";
    }
    void value(const TextStyle& v) {
        const auto symbol = colorSymbol(v.foreground);
        if (symbol.empty()) { valid_ = false; }
        output_ += "{\n            \"foreground\": "; string(symbol);
        output_ += ",\n            \"bold\": "; value(v.bold);
        output_ += ",\n            \"dim\": "; value(v.dim);
        output_ += ",\n            \"underline\": "; value(v.underline);
        output_ += ",\n            \"inverse\": "; value(v.inverse);
        output_ += "\n          }";
    }

    template<class Properties>
    void property(const Properties& properties, const ComponentPropertyMetadata& descriptor,
                  std::string_view kind) {
        const typename Properties::value_type* found = nullptr;
        for (const auto& candidate : properties) {
            if (candidate.declaring_type_id == descriptor.declaring_type_id &&
                candidate.property_id == descriptor.property_id) {
                if (found) { valid_ = false; return; }
                found = &candidate;
            }
        }
        if (!found) { valid_ = false; return; }
        output_ += "{\n          \"declaringType\": "; string(descriptor.declaring_type_id);
        output_ += ",\n          \"propertyId\": "; string(descriptor.property_id);
        output_ += ",\n          \"kind\": "; string(kind);
        output_ += ",\n          \"value\": "; value(found->value);
        output_ += "\n        }";
        // Defaults are explicit durable values. is_default is capture/tooling metadata only;
        // neither it nor a fresh default query may influence this encoding.
    }

    void properties(const BuiltInComponentPropertySnapshot& snapshot, std::string_view type,
                    bool& first, std::size_t& count, std::size_t depth = 0) {
        // Capture already validates the chain. Keep this traversal bounded as well if a future
        // metadata edit accidentally introduces a cycle; no partial document can escape.
        if (depth >= builtInComponentTypes().size()) { valid_ = false; return; }
        const auto* metadata = findBuiltInComponentType(type);
        if (!metadata) { valid_ = false; return; }
        if (!metadata->base_type_id.empty()) {
            properties(snapshot, metadata->base_type_id, first, count, depth + 1);
        }
        for (const auto& descriptor : declaredBuiltInComponentProperties(type)) {
            if (!first) { output_ += ','; }
            output_ += "\n        "; first = false; ++count;
            switch (descriptor.value_kind) {
            case ComponentPropertyValueKind::boolean:
                property(snapshot.boolean_properties, descriptor, "boolean"); break;
            case ComponentPropertyValueKind::string:
                property(snapshot.string_properties, descriptor, "string"); break;
            case ComponentPropertyValueKind::signed_integer:
                property(snapshot.signed_integer_properties, descriptor, "signed-integer"); break;
            case ComponentPropertyValueKind::unsigned_integer:
                property(snapshot.unsigned_integer_properties, descriptor, "unsigned-integer"); break;
            case ComponentPropertyValueKind::string_collection:
                property(snapshot.string_collection_properties, descriptor, "string-collection"); break;
            case ComponentPropertyValueKind::optional_unsigned_integer:
                property(snapshot.optional_unsigned_integer_properties, descriptor,
                         "optional-unsigned-integer"); break;
            case ComponentPropertyValueKind::size_constraints:
                property(snapshot.size_constraints_properties, descriptor, "size-constraints"); break;
            case ComponentPropertyValueKind::text_style:
                property(snapshot.text_style_properties, descriptor, "text-style"); break;
            default: valid_ = false; break;
            }
        }
    }

    void writeNode(const ComponentOwnershipNodeSnapshot& node) {
        output_ += "{\n      \"id\": "; integer(node.node_id);
        output_ += ",\n      \"type\": "; string(node.properties.type_id);
        output_ += ",\n      \"owns\": ";
        array(node.owned_component_ids, 6, [this](auto id) { integer(id); });
        output_ += ",\n      \"properties\": [";
        bool first = true;
        std::size_t count = 0;
        // Metadata supplies base-to-derived declaration order, independent of the eight value
        // vectors. This stable semantic order keeps Git diffs useful (items precedes selection).
        properties(node.properties, node.properties.type_id, first, count);
        const auto& p = node.properties;
        if (count != p.boolean_properties.size() + p.string_properties.size() +
            p.signed_integer_properties.size() + p.unsigned_integer_properties.size() +
            p.string_collection_properties.size() + p.optional_unsigned_integer_properties.size() +
            p.size_constraints_properties.size() + p.text_style_properties.size()) { valid_ = false; }
        if (!first) { output_ += "\n      "; }
        output_ += "]\n    }";
    }
};
} // namespace

std::optional<std::string> encodeBuiltInUiDescriptionJson(const Component& root) {
    // Read runtime state once through the established capture pipeline. Encoding consumes only
    // owned snapshot values; it never re-discovers relations or calls control getters.
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    if (!graph) { return std::nullopt; }
    return Writer{}.encode(*graph);
}

} // namespace sasd::ui
