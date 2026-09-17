#pragma once

namespace sasd::ui {

class Container;

/**
 * Base class for toolkit components that participate in ownership and lifecycle semantics.
 *
 * Component is intentionally not a universal root object for every value type in the library.
 * A component may be visual (through Widget) or non-visual (for example a future Command,
 * Timer or DataSource).
 */
class Component {
public:
    Component() = default;
    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

    /** Returns the component that owns this component, or nullptr when it is not owned. */
    [[nodiscard]] Component* owner() noexcept { return owner_; }
    [[nodiscard]] const Component* owner() const noexcept { return owner_; }

private:
    friend class Container;

    void setOwner(Component* owner) noexcept { owner_ = owner; }

    Component* owner_{nullptr};
};

} // namespace sasd::ui
