#pragma once

#include <sasd/ui/widget.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace sasd::ui {

/**
 * A visual widget that owns a set of child components.
 *
 * Ownership and visual parenting are deliberately separate concepts: every adopted component gets
 * this container as its owner, while Widget-derived components additionally get this container as
 * their visual parent. This distinction keeps the model suitable for future non-visual components.
 */
class Container : public Widget {
public:
    Container() = default;
    ~Container() override;

    template <typename T, typename... Args>
        requires std::derived_from<T, Component>
    T& emplace(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        auto& result = *component;
        adopt(std::move(component));
        return result;
    }

    void adopt(std::unique_ptr<Component> component);

    /** Returns the number of components owned by this container, visual or non-visual. */
    [[nodiscard]] std::size_t componentCount() const noexcept { return components_.size(); }
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
