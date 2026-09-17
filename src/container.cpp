#include <sasd/ui/container.hpp>

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

    component->setOwner(this);

    if (auto* widget = dynamic_cast<Widget*>(component.get())) {
        widget->setParent(this);
    }

    components_.push_back(std::move(component));
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
