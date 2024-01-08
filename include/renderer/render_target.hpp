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
     * @brief Render target surface size syncronization mode
     *  * None - Dont synchronize render target to render surface size
     *  * Resolution - Synchronize target resolution to render surface size
     * (full resolution rendering)
     *  * HalfResolution - Synchronize target resolution to half the render
     * surface size (half resolution rendering)
     */
    enum class SynchMode { None, Resolution, HalfResolution };

    /**
     * @brief Render tag configuration
     * @a width Render target width in pixels
     * @a height TRender target height in pixels
     * @a attachments Array of target attachments (Textures)
     * @a sync_mode Render surface size syncronization mode
     */
    struct Config {
        const uint32     width {};
        const uint32     height {};
        Vector<Texture*> attachments {};
        const bool       one_per_frame_in_flight = false;
        const SynchMode  sync_mode = RenderTarget::SynchMode::Resolution;
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

    RenderTarget(FrameBuffer* const framebuffer, const Config& config);
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
    SynchMode        _sync_mode;
};

} // namespace ENGINE_NAMESPACE