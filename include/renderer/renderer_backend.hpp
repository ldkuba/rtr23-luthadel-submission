#pragma once

#include "platform/platform.hpp"

#include "systems/resource_system.hpp"
#include "renderer_types.hpp"

class RendererBackend {
  public:
    RendererBackend(
        Platform::Surface* const surface, ResourceSystem* const resource_system
    )
        : _resource_system(resource_system) {}
    virtual ~RendererBackend() {}

    // Prevent accidental copying
    RendererBackend(RendererBackend const&)            = delete;
    RendererBackend& operator=(RendererBackend const&) = delete;

    void         increment_frame_number() { _frame_number++; }
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
    virtual bool begin_frame(const float32 delta_time) { return false; }
    /**
     * @brief Complete all rendering operations for this frame
     * @param delta_time Time in seconds since the last frame
     * @return false If a problem is encountered during the method execution
     * @return true Otherwise
     */
    virtual bool end_frame(const float32 delta_time) { return false; }

    virtual void begin_render_pass(uint8 render_pass_id) {}
    virtual void end_render_pass(uint8 render_pass_id) {}

    /**
     * @brief Updates the global world state of the renderer
     * TODO:
     * @param projection
     * @param view
     * @param view_position
     * @param ambient_color
     * @param mode
     */
    virtual void update_global_world_state(
        const glm::mat4 projection,
        const glm::mat4 view,
        const glm::vec3 view_position,
        const glm::vec4 ambient_color,
        const int32     mode
    ) {}
    /**
     * @brief Update global state of renderer UI
     *
     * @param projection
     * @param view
     * @param mode
     */
    virtual void update_global_ui_state(
        const glm::mat4 projection, const glm::mat4 view, const int32 mode
    ) {}

    /**
     * @brief Draw command for specified geometry
     * @param data Draw info
     */
    virtual void draw_geometry(const GeometryRenderData data) {}

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
     * @brief Create a material and upload its relevant data to the GPU
     *
     * @param material Material to be uploaded
     */
    virtual void create_material(Material* const material) {}
    /**
     * @brief Destroy material and free its corresponding GPU resources
     *
     * @param material Material to be destroyed
     */
    virtual void destroy_material(Material* const material) {}

    /**
     * @brief Create a geometry and upload its relevant data to the GPU
     *
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    virtual void create_geometry(
        Geometry*                 geometry,
        const std::vector<Vertex> vertices,
        const std::vector<uint32> indices
    ) {}
    /**
     * @brief Destroy geometry and free its corresponding GPU resources
     *
     * @param geometry Geometry to be destroyed
     */
    virtual void destroy_geometry(Geometry* geometry) {}

  protected:
    ResourceSystem* _resource_system;

  private:
    uint64 _frame_number = 0;
};