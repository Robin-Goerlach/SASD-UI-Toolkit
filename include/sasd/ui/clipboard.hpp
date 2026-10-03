#pragma once

#include <optional>
#include <string>

namespace sasd::ui {

/**
 * Backend-neutral text clipboard service.
 *
 * Clipboard deliberately models only UTF-8 text in the first M4 foundation. Rich text, images,
 * arbitrary MIME/data formats, ownership notifications and asynchronous transfer protocols all need
 * stronger contracts and remain later extensions. Keeping the initial surface small lets widgets use
 * copy/paste semantics without exposing Win32, Cocoa, X11, Wayland, SDL or terminal-specific handles.
 *
 * readText() returns an owned string. A caller may therefore retain the result after the platform
 * clipboard changes or after the backend releases any native temporary storage. std::nullopt means
 * that no textual clipboard payload is currently available. An empty std::string is intentionally a
 * present text payload and is therefore distinct from std::nullopt.
 *
 * Concrete platform implementations may throw when the underlying system clipboard cannot be read
 * or written. This interface does not invent a cross-platform error taxonomy before real adapters
 * demonstrate which failure distinctions applications need.
 */
class Clipboard {
public:
    virtual ~Clipboard() = default;

    Clipboard(const Clipboard&) = delete;
    Clipboard& operator=(const Clipboard&) = delete;
    Clipboard(Clipboard&&) = delete;
    Clipboard& operator=(Clipboard&&) = delete;

    /** Returns an owned UTF-8 text payload, or std::nullopt when no text is available. */
    [[nodiscard]] virtual std::optional<std::string> readText() const = 0;

    /**
     * Replaces the current clipboard text with an owned UTF-8 payload.
     *
     * Passing by value makes the ownership transfer explicit and keeps implementations free to move
     * the bytes into retained storage or copy them into a native platform API. The semantic contract
     * treats the string as UTF-8; validation/normalization policy remains at the concrete adapter
     * boundary because some native clipboards can contain malformed external data.
     */
    virtual void writeText(std::string text) = 0;

    /** Removes the textual clipboard payload when the backend supports the operation. */
    virtual void clear() = 0;

protected:
    Clipboard() = default;
};

} // namespace sasd::ui
