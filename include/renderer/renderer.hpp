#pragma once

#include "renderer/vulkan/vulkan_backend.hpp"

enum RendererBackendType { Vulkan };

class Renderer {
  public:
    Renderer(
        const RendererBackendType backend_type,
        Platform::Surface* const  surface,
        ResourceSystem* const     resource_system
    );
    ~Renderer();

    // Prevent accidental copying
    Renderer(Renderer const&)            = delete;
    Renderer& operator=(Renderer const&) = delete;

    // TODO: TEMP TEST CODE
    Geometry* current_geometry    = nullptr;
    Geometry* current_ui_geometry = nullptr;
    // TODO: TEMP TEST CODE END

    /**
     * @brief Inform renderer of a surface resize event
     *
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void on_resize(const uint32 width, const uint32 height);
    /**
     * @brief Draw to the surface
     *
     * @param delta_time Time in seconds since the last frame
     * @return true If draw operation fully completes
     * @return false Otherwise
     */
    Result<void, RuntimeError> draw_frame(const float32 delta_time);

    /**
     * @brief Create a texture and upload its relevant data to the GPU
     * @param texture Texture to be upload
     * @param data Raw texture image data
     */
    void create_texture(Texture* texture, const byte* const data);
    /**
     * @brief Destroy a texture and free its corresponding GPU resources
     * @param texture Texture to be destroy
     */
    void destroy_texture(Texture* texture);

    /**
     * @brief Create a material and upload its relevant data to the GPU
     * @param material Material to be uploaded
     */
    void create_material(Material* const material);
    /**
     * @brief Destroy material and free its corresponding GPU resources
     * @param material Material to be destroyed
     */
    void destroy_material(Material* const material);

    /**
     * @brief Create a geometry and upload its relevant data to the GPU
     * @tparam VertexType Vertex (Vertex3D) or Vertex2D
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    template<typename VertexType>
    void create_geometry(
        Geometry*                      geometry,
        const std::vector<VertexType>& vertices,
        const std::vector<uint32>&     indices
    );
    /**
     * @brief Destroy geometry and free its corresponding GPU resources
     * @param geometry Geometry to be destroyed
     */
    void destroy_geometry(Geometry* geometry);

  private:
    RendererBackend* _backend         = nullptr;
    ResourceSystem*  _resource_system = nullptr;

    float32   _near_plane = 0.01f;
    float32   _far_plane  = 1000.0f;
    glm::mat4 _projection = glm::perspective(
        glm::radians(45.0f), 800.0f / 600.0f, _near_plane, _far_plane
    );
    glm::mat4 _view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // UI
    glm::mat4 _projection_ui =
        glm::ortho(0.0f, 800.f, 600.0f, 0.0f, -100.0f, 100.0f);
    glm::mat4 _view_ui = glm::mat4(
        glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    );
};

template<typename VertexType>
void Renderer::create_geometry(
    Geometry*                      geometry,
    const std::vector<VertexType>& vertices,
    const std::vector<uint32>&     indices
) {
    static const char* const RENDERER_LOG = "Renderer :: ";
    Logger::trace(RENDERER_LOG, "Creating geometry.");
    _backend->create_geometry(geometry, vertices, indices);
    Logger::trace(RENDERER_LOG, "Geometry created.");
}