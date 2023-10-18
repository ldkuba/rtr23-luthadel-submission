#pragma once

#include "platform/platform.hpp"

#include "renderer_types.hpp"
#include "resources/shader.hpp"

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
    virtual ~RendererBackend() {}

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
     * @brief Inform renderer backend of a surface resize event
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void resized(const uint32 width, const uint32 height) {}

    /**
     * @brief Preform operations in preparation for frame rendering
     * @param delta_time Time in seconds since the last frame
     * @return false If a problem is encountered during the method execution
     * @return true Otherwise
     */
    virtual Result<void, RuntimeError> begin_frame(const float32 delta_time) {
        return {};
    }
    /**
     * @brief Complete all rendering operations for this frame
     * @param delta_time Time in seconds since the last frame
     * @return false If a problem is encountered during the method execution
     * @return true Otherwise
     */
    virtual Result<void, RuntimeError> end_frame(const float32 delta_time) {
        return {};
    }

    /**
     * @brief Start recording of render pass commands
     *
     * @param render_pass_id Render pass id
     */
    virtual void begin_render_pass(uint8 render_pass_id) {}
    /**
     * @brief End recording of render pass commands
     *
     * @param render_pass_id Render pass id
     */
    virtual void end_render_pass(uint8 render_pass_id) {}

    /**
     * @brief Draw command for specified geometry
     * @param geometry Geometry to draw
     */
    virtual void draw_geometry(Geometry* const geometry) {}

    /**
     * @brief Create a texture and upload its relevant data to the GPU
     *
     * @param texture Texture to be upload
     * @param data Raw texture image data
     */
    virtual void create_texture(Texture* texture, const byte* const data) {}
    /**
     * @brief Destroy a texture and free its corresponding GPU resources
     *
     * @param texture Texture to be destroy
     */
    virtual void destroy_texture(Texture* texture) {}

    /**
     * @brief Create a geometry and upload its relevant data to the GPU
     *
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    virtual void create_geometry(
        Geometry*             geometry,
        const Vector<Vertex>& vertices,
        const Vector<uint32>& indices
    ) {}
    /**
     * @brief Create a 2D geometry and upload its relevant data to the GPU
     *
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    virtual void create_geometry(
        Geometry*               geometry,
        const Vector<Vertex2D>& vertices,
        const Vector<uint32>&   indices
    ) {}
    /**
     * @brief Destroy geometry and free its corresponding GPU resources
     *
     * @param geometry Geometry to be destroyed
     */
    virtual void destroy_geometry(Geometry* geometry) {}

    /**
     * @brief Create a shader object and upload relevant data to the GPU
     * @param config Shader configuration
     * @return Pointer referencing the shader created object
     */
    virtual Shader* create_shader(const ShaderConfig config) { return nullptr; }
    /**
     * @brief Destroy shader and free its corresponding GPU resources
     * @param shader Shader to be destroyed.
     */
    virtual void    destroy_shader(Shader* shader) {}

  private:
    uint64 _frame_number = 0;
};

} // namespace ENGINE_NAMESPACE