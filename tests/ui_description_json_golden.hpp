// Hand-specified semantic fixtures, frozen as exact canonical byte strings.
#pragma once
#include <string_view>
inline constexpr std::string_view defaultLabelJson = R"golden({
  "format": "sasd.ui-description",
  "formatVersion": 1,
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
          "value": ""
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "textStyle",
          "kind": "text-style",
          "value": {
            "foreground": "default",
            "bold": false,
            "dim": false,
            "underline": false,
            "inverse": false
          }
        }
      ]
    }
  ],
  "references": []
}
)golden";
inline constexpr std::string_view comprehensiveJson = R"golden({
  "format": "sasd.ui-description",
  "formatVersion": 1,
  "root": 0,
  "nodes": [
    {
      "id": 0,
      "type": "sasd.ui.Window",
      "owns": [
        1,
        2
      ],
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
        }
      ]
    },
    {
      "id": 1,
      "type": "sasd.ui.Command",
      "owns": [],
      "properties": [
        {
          "declaringType": "sasd.ui.Command",
          "propertyId": "text",
          "kind": "string",
          "value": "Save"
        },
        {
          "declaringType": "sasd.ui.Command",
          "propertyId": "enabled",
          "kind": "boolean",
          "value": true
        }
      ]
    },
    {
      "id": 2,
      "type": "sasd.ui.VBox",
      "owns": [
        3,
        4,
        5,
        6
      ],
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
          "declaringType": "sasd.ui.VBox",
          "propertyId": "spacing",
          "kind": "signed-integer",
          "value": 12
        }
      ]
    },
    {
      "id": 3,
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
              "width": -12,
              "height": -3
            },
            "preferred": {
              "width": 40,
              "height": 20
            },
            "maximum": {
              "width": 100,
              "height": 50
            }
          }
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "text",
          "kind": "string",
          "value": "Name"
        },
        {
          "declaringType": "sasd.ui.Label",
          "propertyId": "textStyle",
          "kind": "text-style",
          "value": {
            "foreground": "bright-cyan",
            "bold": true,
            "dim": false,
            "underline": true,
            "inverse": false
          }
        }
      ]
    },
    {
      "id": 4,
      "type": "sasd.ui.Button",
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
          "value": true
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
          "declaringType": "sasd.ui.Button",
          "propertyId": "text",
          "kind": "string",
          "value": "Save"
        },
        {
          "declaringType": "sasd.ui.Button",
          "propertyId": "textStyle",
          "kind": "text-style",
          "value": {
            "foreground": "default",
            "bold": false,
            "dim": false,
            "underline": false,
            "inverse": false
          }
        }
      ]
    },
    {
      "id": 5,
      "type": "sasd.ui.ComboBox",
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
          "value": true
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
          "declaringType": "sasd.ui.ComboBox",
          "propertyId": "items",
          "kind": "string-collection",
          "value": [
            "One",
            "Two"
          ]
        },
        {
          "declaringType": "sasd.ui.ComboBox",
          "propertyId": "selectedIndex",
          "kind": "optional-unsigned-integer",
          "value": 1
        },
        {
          "declaringType": "sasd.ui.ComboBox",
          "propertyId": "textStyle",
          "kind": "text-style",
          "value": {
            "foreground": "default",
            "bold": false,
            "dim": false,
            "underline": false,
            "inverse": false
          }
        }
      ]
    },
    {
      "id": 6,
      "type": "sasd.ui.GridLayout",
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
          "declaringType": "sasd.ui.GridLayout",
          "propertyId": "columnCount",
          "kind": "unsigned-integer",
          "value": 3
        },
        {
          "declaringType": "sasd.ui.GridLayout",
          "propertyId": "columnSpacing",
          "kind": "signed-integer",
          "value": 0
        },
        {
          "declaringType": "sasd.ui.GridLayout",
          "propertyId": "rowSpacing",
          "kind": "signed-integer",
          "value": 0
        }
      ]
    }
  ],
  "references": [
    {
      "source": 4,
      "kind": "button.command",
      "target": 1
    }
  ]
}
)golden";
