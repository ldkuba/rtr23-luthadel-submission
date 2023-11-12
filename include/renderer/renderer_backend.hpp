#pragma once

#include "platform/platform.hpp"

#include "renderer_types.hpp"
#include "resources/shader.hpp"
#include "render_pass.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief General renderer backend interface. Renderer backend is responsible
 * for making calls to the underlying graphics API. Frontend only interacts with
 * device with this structure.
 */
class RendererBackend {
  public:
    /**
     * @brief Construct a new Renderer Backend object
     *
     * @param surface A pointer to the render surface
     */
    RendererBackend(Platform::Surface* const surface) {}
    virtual ~RendererBackend() {};

    // Prevent accidental copying
    RendererBackend(RendererBackend const&)            = delete;
    RendererBackend& operator=(RendererBackend const&) = delete;

    /**
     * @brief Increments frame counter
     */
    void increment_frame_number() { _frame_number++; }

    /**
     * @brief Get current value of the frame counter
     * @return uint64 Current frame counter value
     */
    uint64 get_current_frame() { return _frame_number; }

    /**
     * @brief Preform operations in preparation for frame rendering
     * @param delta_time Time in seconds since the last frame
     * @throws RuntimeError If a problem is encountered during the method
     * execution
     */
    virtual Result<void, RuntimeError> begin_frame( //
        const float32 delta_time
    )                                                                      = 0;
    /**
     * @brief Complete all rendering operations for this frame
     * @param delta_time Time in seconds since the last frame
     * @throws RuntimeError If a problem is encountered during the method
     * execution
     */
    virtual Result<void, RuntimeError> end_frame(const float32 delta_time) = 0;

    /**
     * @brief Inform renderer backend of a surface resize event
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void resized(const uint32 width, const uint32 height) = 0;

    /**
     * @brief Create a texture and upload its relevant data to the GPU
     *
     * @param texture Texture to be uploaded
     * @param data Raw texture image data
     */
    virtual void create_texture(
        Texture* const texture, const byte* const data
    )                                                            = 0;
    /**
     * @brief Create a writable texture object with no initial data.
     * @param texture Texture to be uploaded
     */
    virtual void create_writable_texture(Texture* const texture) = 0;
    /**
     * @brief Destroy a texture and free its corresponding GPU resources
     *
     * @param texture Texture to be destroy
     */
    virtual void destroy_texture(Texture* const texture)         = 0;

    /**
     * @brief Resizes a texture. Internally texture is destroyed and recreated.
     * @param texture Texture to be resized
     * @param width New width in pixels
     * @param height New Height in pixels
     */
    virtual void resize_texture(
        Texture* const texture, const uint32 width, const uint32 height
    ) = 0;

    /**
     * @brief Write data to provided texture. NOTE: This code wont block write
     * requests for non-writable textures.
     * @param texture Texture to be written to
     * @param data Raw data bytes to be written
     * @param size Data size in bytes
     * @param offset Offset in bytes from which write starts
     */
    virtual void texture_write_data(
        Texture* const    texture,
        const byte* const data,
        const uint32      size,
        const uint32      offset
    ) = 0;

    /**
     * @brief Create a geometry and upload its relevant data to the GPU
     *
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    virtual void create_geometry(
        Geometry* const         geometry,
        const Vector<Vertex3D>& vertices,
        const Vector<uint32>&   indices
    ) = 0;
    /**
     * @brief Create a 2D geometry and upload its relevant data to the GPU
     *
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    virtual void create_geometry(
        Geometry* const         geometry,
        const Vector<Vertex2D>& vertices,
        const Vector<uint32>&   indices
    )                                                    = 0;
    /**
     * @brief Destroy geometry and free its corresponding GPU resources
     *
     * @param geometry Geometry to be destroyed
     */
    virtual void destroy_geometry(Geometry* geometry)    = 0;
    /**
     * @brief Draw command for specified geometry
     * @param geometry Geometry to draw
     */
    virtual void draw_geometry(Geometry* const geometry) = 0;

    /**
     * @brief Create a shader object and upload relevant data to the GPU
     * @param config Shader configuration
     * @return Pointer referencing the shader created object
     */
    virtual Shader* create_shader(const Shader::Config config) = 0;
    /**
     * @brief Destroy shader and free its corresponding GPU resources
     * @param shader Shader to be destroyed.
     */
    virtual void    destroy_shader(Shader* const shader)       = 0;

    /**
     * @brief Create a render target object.
     * @param pass Associated render pass
     * @param width Render target width in pixels
     * @param height Render target height in pixels
     * @param attachments Array of target attachments (Textures)
     * @returns RenderTarget* Created render target
     */
    virtual RenderTarget* create_render_target(
        RenderPass* const       pass,
        const uint32            width,
        const uint32            height,
        const Vector<Texture*>& attachments
    ) = 0;
    /**
     * @brief Destroy provided render target
     * @param render_target Target to be destroyed
     * @param free_internal_data If true also frees internal render target GPU
     * memory
     */
    virtual void destroy_render_target(
        RenderTarget* const render_target, const bool free_internal_data = true
    ) = 0;

    /**
     * @brief Create a render pass object
     * @param config Render pass configurations
     * @returns RenderPass* Created render pass
     */
    virtual RenderPass* create_render_pass(const RenderPass::Config& config
    )                                                               = 0;
    /**
     * @brief Destroy provided render pass
     * @param pass Render pass to be destroyed
     */
    virtual void        destroy_render_pass(RenderPass* const pass) = 0;
    /**
     * @brief Get a reference to a render pass object by name
     *
     * @param name Render pass name
     * @return RenderPass* If render pass is found
     * @throws RuntimeError otherwise
     */
    virtual Result<RenderPass*, RuntimeError> get_render_pass(const String& name
    ) const                                                         = 0;

    /**
     * @return uint8 Current window attachment index
     */
    virtual uint8 get_current_window_attachment_index() const = 0;
    /**
     * @return uint8 Window attachment count
     */
    virtual uint8 get_window_attachment_count() const         = 0;

    /**
     * @brief Get the window attachment texture at the given index
     * @param index Index of attachment we want to get. Must be within
     * window attachment range.
     * @return Texture* Requested window attachment texture if successful
     * @throws RuntimeError otherwise
     */
    virtual Texture* get_window_attachment(const uint8 index) const = 0;
    /**
     * @return Texture* Main depth attachment texture if dept testing is enabled
     */
    virtual Texture* get_depth_attachment() const                   = 0;
    /**
     * @return Texture* Get main resolve color attachment if multisampling is
     * enabled
     */
    virtual Texture* get_color_attachment() const                   = 0;

  private:
    uint64 _frame_number = 0;
};

} // namespace ENGINE_NAMESPACE