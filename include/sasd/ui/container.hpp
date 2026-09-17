#pragma once

#include <sasd/ui/widget.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace sasd::ui {

/**
 * A visual widget that owns a set of child components.
 *
 * Ownership and visual parenting are deliberately separate concepts: every adopted component gets
 * this container as its owner, while Widget-derived components additionally get this container as
 * their visual parent. This distinction keeps the model suitable for future non-visual components
 * such as commands, timers and data sources.
 */
class Container : public Widget {
public:
    Container() = default;
    ~Container() override;

    /**
     * Constructs a component directly inside this container and transfers exclusive ownership to it.
     *
     * Widget-derived components also become visual children. Non-visual components only participate
     * in the ownership tree. The returned reference remains valid until the component is released or
     * the owning container is destroyed.
     */
    template <typename T, typename... Args>
        requires std::derived_from<T, Component>
    T& emplace(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        auto& result = *component;
        adopt(std::move(component));
        return result;
    }

    /**
     * Transfers exclusive ownership of an unowned component into this container.
     *
     * A Widget receives this container as its visual parent in addition to its ownership relation.
     * Null pointers and components that already have an owner are rejected because silently sharing
     * ownership would make lifetime semantics ambiguous.
     */
    void adopt(std::unique_ptr<Component> component);

    /**
     * Removes a component owned by this container and returns exclusive ownership to the caller.
     *
     * Both owner() and, for Widget instances, parent() are reset before the component is returned.
     * This operation is therefore the inverse of adopt() and permits explicit transfer between
     * containers without introducing raw owning pointers. If the component is not owned by this
     * container, an empty unique_ptr is returned and no state is changed.
     */
    [[nodiscard]] std::unique_ptr<Component> release(Component& component) noexcept;

    /** Returns the number of components owned by this container, visual or non-visual. */
    [[nodiscard]] std::size_t componentCount() const noexcept { return components_.size(); }

    /** Returns an owned component by ownership-order index. Throws std::out_of_range if invalid. */
    [[nodiscard]] Component& componentAt(std::size_t index) { return *components_.at(index); }
    [[nodiscard]] const Component& componentAt(std::size_t index) const {
        return *components_.at(index);
    }

    /** Returns the number of visually parented Widget children. */
    [[nodiscard]] std::size_t childCount() const noexcept;

    /**
     * Returns a visual child by its visual-child index.
     *
     * Non-visual owned components are deliberately skipped. The order follows adoption order among
     * Widget-derived components. Throws std::out_of_range when index is outside the visual children.
     */
    [[nodiscard]] Widget& childAt(std::size_t index);
    [[nodiscard]] const Widget& childAt(std::size_t index) const;

private:
    std::vector<std::unique_ptr<Component>> components_;
};

} // namespace sasd::ui
