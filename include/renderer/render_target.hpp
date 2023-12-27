#pragma once

#include "resources/texture.hpp"
#include "renderer/renderer_types.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Represents a render target. Used when rendering to a texture, or a set
 * of textures.
 *
 */
class RenderTarget {
  public:
    /**
     * @brief Render tag configuration
     * @a width Render target width in pixels
     * @a height TRender target height in pixels
     * @a attachments Array of target attachments (Textures)
     */
    struct Config {
        const uint32           width {};
        const uint32           height {};
        const Vector<Texture*> attachments {};
        const bool             one_per_frame_in_flight = false;
    };

  public:
    /// @brief Reference to the underlying framebuffer
    Property<FrameBuffer*> framebuffer {
        GET { return _framebuffer; }
    };

    /// @brief Render area width in pixels
    Property<uint32> width {
        GET { return _width; }
    };
    /// @brief Render area height in pixels
    Property<uint32> height {
        GET { return _height; }
    };

    RenderTarget(
        const Vector<Texture*>& attachments,
        FrameBuffer* const      framebuffer,
        const uint32            width,
        const uint32            height
    );
    ~RenderTarget();

    /**
     * @brief Resizes render target
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void resize(const uint32 width, const uint32 height);

    /**
     * @brief Extends attachment list of this render target
     * @param attachments Attachments to add
     */
    void add_attachments(const Vector<Texture*>& attachments);
    /**
     * @brief Clears attachment list of all attachments
     */
    void free_attachments();

  private:
    uint32           _width, _height;
    Vector<Texture*> _attachments {};
    FrameBuffer*     _framebuffer {};
    bool             _sync_to_window_resize;
};

} // namespace ENGINE_NAMESPACE