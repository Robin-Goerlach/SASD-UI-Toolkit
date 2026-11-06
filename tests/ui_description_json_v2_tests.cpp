#include "test_framework.hpp"

#include <sasd/ui/label.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/resource_key.hpp>
#include <sasd/ui/resource_reference.hpp>
#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/ui_description_reconstruction.hpp>

#include <algorithm>
#include <string>
#include <string_view>

using namespace sasd::ui;

namespace {

constexpr std::string_view resourceBackedLabelV2Golden = R"json({
  "format": "sasd.ui-description",
  "formatVersion": 2,
  "resources": [
    {
      "kind": "text-style",
      "key": "theme.heading",
      "value": {
        "foreground": "bright-cyan",
        "bold": true,
        "dim": false,
        "underline": false,
        "inverse": false
      }
    }
  ],
  "root": 0,
  "nodes": [
    {
      "id": 0,
      "type": "sasd.ui.Label",
      "owns": [],
      "properties": [
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "visible",
          "kind": "boolean",
          "value": true
        },
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "enabled",
          "kind": "boolean",
          "value": true
        },
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "focusable",
          "kind": "boolean",
          "value": false
        },
        {
          "declaringType": "sasd.ui.Widget",
          "propertyId": "sizeConstraints",
          "kind": "size-constraints",
          "value": {
            "minimum": {
              "width": 0,
              "height": 0
            },
            "preferred": {
              "width": 0,
              "height": 0
            },
            "maximum": {
              "width": 2147483647,
              "height": 2147483647
            }
          }
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "text",
          "kind": "string",
          "value": "Heading"
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "textStyle",
          "kind": "resource-reference",
          "value": {
            "resourceKind": "text-style",
            "key": "theme.heading"
          }
        }
      ]
    }
  ],
  "references": []
}
)json";

BuiltInUiDescriptionDocument inlineLabelDocument() {
    Label label{"Heading"};
    label.setTextStyle({Color::bright_cyan, true, false, false, false});
    const auto v1 = encodeBuiltInUiDescriptionJson(label);
    CHECK(v1);
    auto parsed = parseBuiltInUiDescriptionJson(*v1);
    CHECK(parsed);
    return std::move(*parsed.document);
}

TextStyleResource resource(std::string_view key, Color color = Color::bright_cyan) {
    auto parsed_key = ResourceKey::create(key);
    CHECK(parsed_key);
    return {std::move(*parsed_key), {color, true, false, false, false}};
}

void makeLabelTextStyleResourceBacked(BuiltInUiDescriptionDocument& document,
                                      std::string_view key = "theme.heading") {
    CHECK(document.nodes.size() == 1);
    auto& node = document.nodes.front();
    const auto inline_property = std::find_if(
        node.text_style_properties.begin(), node.text_style_properties.end(), [](const auto& value) {
            return value.declaring_type_id == "sasd.ui.Label" &&
                   value.property_id == "textStyle";
        });
    CHECK(inline_property != node.text_style_properties.end());
    node.text_style_properties.erase(inline_property);

    auto parsed_key = ResourceKey::create(key);
    CHECK(parsed_key);
    auto reference = ResourceReference::create(ResourceKind::text_style, std::move(*parsed_key));
    CHECK(reference);
    node.resource_reference_properties.push_back(
        {"sasd.ui.Label", "textStyle", std::move(*reference)});
}

std::string replaceOnce(std::string text, std::string_view from, std::string_view to) {
    const auto at = text.find(from);
    CHECK(at != std::string::npos);
    text.replace(at, from.size(), to);
    return text;
}

void rejectsV2(std::string_view json, UiDescriptionJsonParseErrorCode error) {
    const auto result = parseBuiltInUiDescriptionJsonV2(json);
    CHECK(!result);
    CHECK(!result.document);
    CHECK(result.error.code == error);
    CHECK(result.error.byte_offset <= json.size());
}

} // namespace

TEST_CASE("JSON v2 resource-backed TextStyle roundtrip is canonical and byte stable") {
    auto document = inlineLabelDocument();
    document.text_style_resources.push_back(resource("theme.heading"));
    makeLabelTextStyleResourceBacked(document);

    const auto first = encodeBuiltInUiDescriptionJsonV2(document);
    CHECK(first);
    CHECK(*first == resourceBackedLabelV2Golden);
    CHECK(first->starts_with(
        "{\n  \"format\": \"sasd.ui-description\",\n  \"formatVersion\": 2,\n"
        "  \"resources\": ["));
    CHECK(first->find("\n  \"root\": 0,\n  \"nodes\": [") != std::string::npos);
    CHECK(first->find("\n  \"references\": []\n}\n") != std::string::npos);
    CHECK(first->find("\"kind\": \"resource-reference\"") != std::string::npos);
    CHECK(first->find("\"resourceKind\": \"text-style\"") != std::string::npos);

    auto parsed = parseBuiltInUiDescriptionJsonV2(*first);
    CHECK(parsed);
    CHECK(*parsed.document == document);
    const auto second = encodeBuiltInUiDescriptionJsonV2(*parsed.document);
    CHECK(second == first);

    // The encoder normalizes a private copy, so compare against an independently parsed normalized
    // document rather than expecting the caller's intentionally unordered aggregate to mutate.
    auto normalized = parseBuiltInUiDescriptionJsonV2(*first);
    CHECK(normalized);
    CHECK(*parsed.document == *normalized.document);
}

TEST_CASE("JSON v2 resources use case-sensitive bytewise ResourceKey order") {
    auto document = inlineLabelDocument();
    document.text_style_resources.push_back(resource("theme.warning", Color::bright_red));
    document.text_style_resources.push_back(resource("theme.body", Color::white));
    document.text_style_resources.push_back(resource("theme.heading", Color::bright_cyan));
    const auto json = encodeBuiltInUiDescriptionJsonV2(document);
    CHECK(json);
    const auto body = json->find("\"key\": \"theme.body\"");
    const auto heading = json->find("\"key\": \"theme.heading\"");
    const auto warning = json->find("\"key\": \"theme.warning\"");
    CHECK(body < heading);
    CHECK(heading < warning);

    auto parsed = parseBuiltInUiDescriptionJsonV2(*json);
    CHECK(parsed);
    CHECK(parsed.document->text_style_resources[0].key.value() == "theme.body");
    CHECK(parsed.document->text_style_resources[1].key.value() == "theme.heading");
    CHECK(parsed.document->text_style_resources[2].key.value() == "theme.warning");
}

TEST_CASE("JSON v2 validates duplicate resources and reference targets fail closed") {
    auto document = inlineLabelDocument();
    document.text_style_resources.push_back(resource("theme.heading"));
    document.text_style_resources.push_back(resource("theme.heading", Color::red));
    CHECK(!encodeBuiltInUiDescriptionJsonV2(document));

    document.text_style_resources.pop_back();
    document.text_style_resources.front().value.foreground = static_cast<Color>(255);
    CHECK(!encodeBuiltInUiDescriptionJsonV2(document));
    document.text_style_resources.front().value.foreground = Color::bright_cyan;

    const auto one_resource = encodeBuiltInUiDescriptionJsonV2(document);
    CHECK(one_resource);
    auto duplicate_json = *one_resource;
    const auto object_start = duplicate_json.find("    {\n", duplicate_json.find("\"resources\""));
    const auto object_end_marker = duplicate_json.find("\n    }", object_start);
    CHECK(object_start != std::string::npos);
    CHECK(object_end_marker != std::string::npos);
    const auto object_end = object_end_marker + 6;
    const auto resource_object = duplicate_json.substr(object_start, object_end - object_start);
    duplicate_json.insert(object_end, ",\n" + resource_object);
    rejectsV2(duplicate_json, UiDescriptionJsonParseErrorCode::invalid_resource);

    makeLabelTextStyleResourceBacked(document, "theme.missing");
    CHECK(!encodeBuiltInUiDescriptionJsonV2(document));

    // Parsing performs the same format-neutral validation after decoding. A syntactically valid
    // key with no local definition never becomes an external or deferred dependency.
    auto valid = inlineLabelDocument();
    valid.text_style_resources.push_back(resource("theme.heading"));
    makeLabelTextStyleResourceBacked(valid);
    const auto json = encodeBuiltInUiDescriptionJsonV2(valid);
    CHECK(json);
    rejectsV2(replaceOnce(*json, "\"key\": \"theme.heading\"\n          }",
                          "\"key\": \"theme.missing\"\n          }"),
              UiDescriptionJsonParseErrorCode::invalid_resource_reference);
}

TEST_CASE("JSON v2 enforces inline XOR resource reference and exact capable property") {
    auto document = inlineLabelDocument();
    document.text_style_resources.push_back(resource("theme.heading"));

    auto key = ResourceKey::create("theme.heading");
    CHECK(key);
    auto reference = ResourceReference::create(ResourceKind::text_style, std::move(*key));
    CHECK(reference);
    document.nodes.front().resource_reference_properties.push_back(
        {"sasd.ui.Label", "textStyle", *reference});
    CHECK(!encodeBuiltInUiDescriptionJsonV2(document)); // inline and reference

    document.nodes.front().resource_reference_properties.clear();
    const auto text = std::find_if(document.nodes.front().string_properties.begin(),
                                   document.nodes.front().string_properties.end(), [](const auto& p) {
        return p.declaring_type_id == "sasd.ui.Label" && p.property_id == "text";
    });
    CHECK(text != document.nodes.front().string_properties.end());
    document.nodes.front().string_properties.erase(text);
    document.nodes.front().resource_reference_properties.push_back(
        {"sasd.ui.Label", "text", std::move(*reference)});
    CHECK(!encodeBuiltInUiDescriptionJsonV2(document)); // known but not resource-capable

    document = inlineLabelDocument();
    const auto style = std::find_if(document.nodes.front().text_style_properties.begin(),
                                    document.nodes.front().text_style_properties.end(), [](const auto& p) {
        return p.declaring_type_id == "sasd.ui.Label" && p.property_id == "textStyle";
    });
    CHECK(style != document.nodes.front().text_style_properties.end());
    document.nodes.front().text_style_properties.erase(style);
    CHECK(!encodeBuiltInUiDescriptionJsonV2(document)); // neither inline nor reference
}

TEST_CASE("JSON v2 accepts exactly all six current resource-capable textStyle slots") {
    const auto accepts = [](const Component& component, std::string_view declaring_type) {
        const auto v1 = encodeBuiltInUiDescriptionJson(component);
        CHECK(v1);
        auto parsed = parseBuiltInUiDescriptionJson(*v1);
        CHECK(parsed);
        auto& document = *parsed.document;
        document.text_style_resources.push_back(resource("theme.control"));
        auto& node = document.nodes.front();
        const auto inline_style = std::find_if(
            node.text_style_properties.begin(), node.text_style_properties.end(),
            [&](const auto& property) {
                return property.declaring_type_id == declaring_type &&
                       property.property_id == "textStyle";
            });
        CHECK(inline_style != node.text_style_properties.end());
        node.text_style_properties.erase(inline_style);
        auto key = ResourceKey::create("theme.control");
        CHECK(key);
        auto reference = ResourceReference::create(ResourceKind::text_style, std::move(*key));
        CHECK(reference);
        node.resource_reference_properties.push_back(
            {std::string{declaring_type}, "textStyle", std::move(*reference)});
        const auto v2 = encodeBuiltInUiDescriptionJsonV2(document);
        CHECK(v2);
        CHECK(parseBuiltInUiDescriptionJsonV2(*v2));
    };

    const Label label;
    const Button button;
    const TextField field;
    const CheckBox check_box;
    const RadioButton radio_button;
    const ComboBox combo_box;
    accepts(label, "sasd.ui.Label");
    accepts(button, "sasd.ui.Button");
    accepts(field, "sasd.ui.TextField");
    accepts(check_box, "sasd.ui.CheckBox");
    accepts(radio_button, "sasd.ui.RadioButton");
    accepts(combo_box, "sasd.ui.ComboBox");
}

TEST_CASE("JSON v2 parser rejects invalid keys and unknown resource kinds") {
    auto document = inlineLabelDocument();
    document.text_style_resources.push_back(resource("theme.heading"));
    document.text_style_resources.push_back(resource("dialog/error", Color::bright_red));
    document.text_style_resources.push_back(resource("button-primary", Color::green));
    makeLabelTextStyleResourceBacked(document);
    const auto json = encodeBuiltInUiDescriptionJsonV2(document);
    CHECK(json);
    const auto valid_keys = parseBuiltInUiDescriptionJsonV2(*json);
    CHECK(valid_keys);
    CHECK(valid_keys.document->text_style_resources.size() == 3);

    for (const auto invalid : {"", "theme heading", "theme\\\\heading", "theme@heading"}) {
        const auto changed = replaceOnce(*json, "\"key\": \"theme.heading\"",
                                         std::string{"\"key\": \""} + invalid + "\"");
        rejectsV2(changed, UiDescriptionJsonParseErrorCode::invalid_resource);
    }
    rejectsV2(replaceOnce(*json, "\"kind\": \"text-style\"",
                          "\"kind\": \"font\""),
              UiDescriptionJsonParseErrorCode::invalid_resource);
    rejectsV2(replaceOnce(*json, "\"resourceKind\": \"text-style\"",
                          "\"resourceKind\": \"font\""),
              UiDescriptionJsonParseErrorCode::invalid_resource_reference);
}

TEST_CASE("JSON v2 allows all-inline documents but remains a distinct version") {
    const auto document = inlineLabelDocument();
    const auto v2 = encodeBuiltInUiDescriptionJsonV2(document);
    CHECK(v2);
    CHECK(v2->find("\"formatVersion\": 2") != std::string::npos);
    CHECK(v2->find("\"resources\": []") != std::string::npos);
    CHECK(parseBuiltInUiDescriptionJsonV2(*v2));

    const auto v1 = encodeBuiltInUiDescriptionJson(Label{});
    CHECK(v1);
    CHECK(!parseBuiltInUiDescriptionJsonV2(*v1));
    CHECK(!parseBuiltInUiDescriptionJson(*v2));
}

TEST_CASE("JSON v2 retains strict JSON and exact member vocabulary") {
    const auto v2 = encodeBuiltInUiDescriptionJsonV2(inlineLabelDocument());
    CHECK(v2);
    rejectsV2(replaceOnce(*v2, "\"resources\": []",
                          "\"resources\": [],\n  \"resources\": []"),
              UiDescriptionJsonParseErrorCode::duplicate_member);
    rejectsV2(replaceOnce(*v2, "\n}\n", ",\n  \"future\": true\n}\n"),
              UiDescriptionJsonParseErrorCode::unknown_member);
    CHECK(!parseBuiltInUiDescriptionJsonV2(replaceOnce(*v2, "{\n", "{/* comment */\n")));
    CHECK(!parseBuiltInUiDescriptionJsonV2(replaceOnce(*v2, "\n}\n", ",\n}\n")));

    auto invalid_utf8 = *v2;
    invalid_utf8.insert(invalid_utf8.find("Heading"), 1, static_cast<char>(0xff));
    rejectsV2(invalid_utf8, UiDescriptionJsonParseErrorCode::invalid_utf8);
}

TEST_CASE("M8.27 reconstruction rejects resource-bearing v2 documents atomically") {
    auto document = inlineLabelDocument();
    document.text_style_resources.push_back(resource("theme.heading"));
    makeLabelTextStyleResourceBacked(document);
    const auto reconstructed = reconstructBuiltInUiDescription(document);
    CHECK(!reconstructed);
    CHECK(reconstructed.error == UiDescriptionReconstructionErrorCode::invalid_document);
}
