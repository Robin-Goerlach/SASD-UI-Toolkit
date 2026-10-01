#pragma once

#include <sasd/ui/container.hpp>
#include <sasd/ui/label.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace sasd::ui {

/**
 * Typed references returned by FormLayout::emplaceRow().
 *
 * The references are non-owning views into components owned by the FormLayout. They remain valid
 * until either component is explicitly released or the FormLayout is destroyed.
 */
template <typename T>
    requires std::derived_from<T, Widget>
struct FormRowRef {
    Label& label;
    T& field;
};

/**
 * Two-column form container with a shared label column and an expanding field column.
 *
 * Visual children are interpreted as stable label/field pairs in visual adoption order:
 * child 0 is row 0's label cell, child 1 its field cell, child 2 is row 1's label cell, and so on.
 * Pairing is based on structural order rather than on visibility, so hiding one cell never shifts all
 * following rows into different semantic columns. An odd final visual child forms a label-only row.
 *
 * emplaceRow() is the preferred convenience API for ordinary forms because it creates the Label and
 * field in the correct order with rollback if adoption of the second component fails. Direct
 * Container::emplace()/adopt() remain available for custom label widgets or intentionally unusual
 * rows; callers using them are responsible for preserving the pair ordering contract.
 *
 * The label column keeps its intrinsic maximum width. The field column consumes the remaining final
 * width after label width and column spacing, making common forms useful when the parent provides more
 * width than their intrinsic minimum. Row heights remain intrinsic and are clipped in row order when
 * vertical space is insufficient.
 */
class FormLayout final : public Container {
public:
    FormLayout() = default;

    /** Returns the number of structural label/field pairs, including an optional final label-only row. */
    [[nodiscard]] std::size_t rowCount() const noexcept {
        return (childCount() + 1U) / 2U;
    }

    [[nodiscard]] Coordinate columnSpacing() const noexcept {
        return column_spacing_;
    }

    [[nodiscard]] Coordinate rowSpacing() const noexcept {
        return row_spacing_;
    }

    /**
     * Creates one Label plus one Widget-derived field and adopts both atomically at the row level.
     *
     * If field adoption throws after the Label was adopted, the Label is released again before the
     * exception propagates. The FormLayout may remain measurement-invalidated, but it never retains a
     * half-created row.
     */
    template <typename T, typename... Args>
        requires std::derived_from<T, Widget>
    FormRowRef<T> emplaceRow(std::string label_text, Args&&... args) {
        auto label = std::make_unique<Label>(std::move(label_text));
        auto field = std::make_unique<T>(std::forward<Args>(args)...);

        Label* const label_ptr = label.get();
        T* const field_ptr = field.get();

        adopt(std::move(label));
        try {
            adopt(std::move(field));
        } catch (...) {
            /*
             * release() is noexcept and restores both owner() and visual parent(). Keeping the
             * returned unique_ptr alive until stack unwinding leaves this scope also guarantees that
             * the rolled-back Label is destroyed exactly once.
             */
            auto rollback = release(*label_ptr);
            (void)rollback;
            throw;
        }

        return {*label_ptr, *field_ptr};
    }

    /** Sets non-negative logical spacing between the shared label and field columns. */
    void setColumnSpacing(Coordinate spacing);

    /** Sets non-negative logical spacing between adjacent active form rows. */
    void setRowSpacing(Coordinate spacing);

protected:
    [[nodiscard]] Size onMeasure(const MeasureConstraints& constraints) override;
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;
    void onArrange(Rect final_bounds) override;

private:
    Coordinate column_spacing_{1};
    Coordinate row_spacing_{0};
};

} // namespace sasd::ui
