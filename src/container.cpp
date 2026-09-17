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

} // namespace sasd::ui
