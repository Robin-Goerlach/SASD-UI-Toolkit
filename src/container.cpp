#include <sasd/ui/container.hpp>

#include <algorithm>
#include <stdexcept>

namespace sasd::ui {

Container::~Container() = default;

void Container::adopt(std::unique_ptr<Component> component) {
    if (!component) {
        throw std::invalid_argument("Container::adopt requires a non-null component");
    }
    if (component->owner() != nullptr) {
        throw std::invalid_argument("Container::adopt requires an unowned component");
    }

    /*
     * Establish the ownership relation only after the vector accepted the unique_ptr. vector
     * growth can allocate and therefore throw. Keeping owner/parent untouched until that succeeds
     * prevents a component from temporarily claiming this container as owner without actually
     * being stored in its ownership list.
     */
    auto* adopted = component.get();
    components_.push_back(std::move(component));

    adopted->setOwner(this);
    if (auto* widget = dynamic_cast<Widget*>(adopted)) {
        widget->setParent(this);

        /*
         * A new visual child may change the desired size of any layout-aware container. Invalidate
         * after the parent link exists so propagation can continue through this container's ancestors.
         */
        invalidateMeasure();
        invalidateVisual();
    }
}

std::unique_ptr<Component> Container::release(Component& component) noexcept {
    if (component.owner() != this) {
        return {};
    }

    const auto it = std::find_if(components_.begin(), components_.end(),
                                 [&component](const auto& candidate) {
                                     return candidate.get() == &component;
                                 });
    if (it == components_.end()) {
        // owner() can only be assigned by Container. Reaching this branch would therefore indicate
        // an internal invariant violation; returning empty keeps release() non-throwing and avoids
        // manufacturing ownership for an object that is not actually stored here.
        return {};
    }

    auto released = std::move(*it);
    components_.erase(it);

    // A released object must be fully detached from both relationship graphs. This is especially
    // important when the returned unique_ptr is immediately adopted by another Container.
    if (auto* widget = dynamic_cast<Widget*>(released.get())) {
        widget->setParent(nullptr);

        /*
         * Structural removal can change container measurement just like insertion. The detached child
         * must no longer propagate later invalidations into its former parent, so detach first and
         * invalidate the container explicitly afterwards.
         */
        invalidateMeasure();
        invalidateVisual();
    }
    released->setOwner(nullptr);

    return released;
}

std::size_t Container::childCount() const noexcept {
    std::size_t count = 0;
    for (const auto& component : components_) {
        if (dynamic_cast<const Widget*>(component.get()) != nullptr) {
            ++count;
        }
    }
    return count;
}

Widget& Container::childAt(std::size_t index) {
    std::size_t child_index = 0;
    for (auto& component : components_) {
        if (auto* widget = dynamic_cast<Widget*>(component.get())) {
            if (child_index == index) {
                return *widget;
            }
            ++child_index;
        }
    }

    throw std::out_of_range("Container::childAt visual child index out of range");
}

const Widget& Container::childAt(std::size_t index) const {
    std::size_t child_index = 0;
    for (const auto& component : components_) {
        if (const auto* widget = dynamic_cast<const Widget*>(component.get())) {
            if (child_index == index) {
                return *widget;
            }
            ++child_index;
        }
    }

    throw std::out_of_range("Container::childAt visual child index out of range");
}

} // namespace sasd::ui
