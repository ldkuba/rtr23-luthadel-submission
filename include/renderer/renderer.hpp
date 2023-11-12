#pragma once

#include "renderer/vulkan/vulkan_backend.hpp"
#include "camera.hpp"
#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief List of supported backend APIs
 */
enum RendererBackendType { Vulkan };

/**
 * @brief The renderer frontend. Interacts with the device using the backend, in
 * a API agnostic way.
 */
class Renderer {
  public:
    struct Builtin {
        constexpr static const char* const WorldPass =
            "Renderpass.Builtin.World";
        constexpr static const char* const UIPass = "Renderpass.Builtin.UI";
    };

  public:
    // TODO: TEMP TEST CODE BEGIN
    Shader* material_shader = nullptr;
    Shader* ui_shader       = nullptr;
    // TODO: TEMP TEST CODE END

    /**
     * @brief Construct a new Renderer object
     *
     * @param backend_type Rendering API used for backend.
     * @param surface A pointer to the render surface
     */
    Renderer(
        const RendererBackendType backend_type, Platform::Surface* const surface
    );
    ~Renderer();

    // Prevent accidental copying
    Renderer(Renderer const&)            = delete;
    Renderer& operator=(Renderer const&) = delete;

    /**
     * @brief Draw to the surface
     *
     * @param render_data Data about everything that needs to be rendered
     * @param delta_time Time in seconds since the last frame
     * @throws RuntimeError If draw operation encounters issues
     */
    Result<void, RuntimeError> draw_frame(
        const RenderPacket* const render_data, const float32 delta_time
    );

    void draw_geometry(Geometry* const geometry);

    /**
     * @brief Inform renderer of a surface resize event
     *
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void on_resize(const uint32 width, const uint32 height);

    /**
     * @brief Create a texture and upload its relevant data to the GPU
     * @param texture Texture to be upload
     * @param data Raw texture image data
     */
    void create_texture(Texture* texture, const byte* const data);
    /**
     * @brief Create a writable texture object with no initial data.
     * @param texture Texture to be uploaded
     */
    void create_writable_texture(Texture* texture);
    /**
     * @brief Destroy a texture and free its corresponding GPU resources
     * @param texture Texture to be destroy
     */
    void destroy_texture(Texture* texture);

    /**
     * @brief Resizes a texture. Internally texture is destroyed and recreated.
     * @param texture Texture to be resized
     * @param width New width in pixels
     * @param height New Height in pixels
     */
    void resize_texture(
        Texture* const texture, const uint32 width, const uint32 height
    );

    /**
     * @brief Write data to provided texture. NOTE: This code wont block write
     * requests for non-writable textures.
     * @param texture Texture to be written to
     * @param data Raw data to be written
     * @param offset Offset in bytes from which write starts
     */
    void texture_write_data(
        Texture* const texture, const Vector<byte>& data, const uint32 offset
    );

    /**
     * @brief Write data to provided texture. NOTE: This code wont block write
     * requests for non-writable textures.
     * @param texture Texture to be written to
     * @param data Raw data bytes to be written
     * @param size Data size in bytes
     * @param offset Offset in bytes from which write starts
     */
    void texture_write_data(
        Texture* const    texture,
        const byte* const data,
        const uint32      size,
        const uint32      offset
    );

    /**
     * @brief Create a geometry and upload its relevant data to the GPU
     * @tparam VertexType Vertex (Vertex3D) or Vertex2D
     * @param geometry Geometry to be uploaded
     * @param vertices Array of vertex data used by the geometry
     * @param indices Array of index data used by the geometry
     */
    template<uint8 Dim>
    void create_geometry(
        Geometry*                  geometry,
        const Vector<Vertex<Dim>>& vertices,
        const Vector<uint32>&      indices
    );
    /**
     * @brief Destroy geometry and free its corresponding GPU resources
     * @param geometry Geometry to be destroyed
     */
    void destroy_geometry(Geometry* geometry);

    /**
     * @brief Create a shader object and upload relevant data to the GPU
     * @param config Shader configuration
     * @return Pointer referencing the shader created object
     */
    Shader* create_shader(const Shader::Config config);
    /**
     * @brief Destroy shader and free its corresponding GPU resources
     * @param shader Shader to be destroyed.
     */
    void    destroy_shader(Shader* shader);

    /**
     * @brief Create a render target object.
     * @param pass Associated render pass
     * @param width Render target width in pixels
     * @param height Render target height in pixels
     * @param attachments Array of target attachments (Textures)
     * @returns RenderTarget* Created render target
     */
    RenderTarget* create_render_target(
        RenderPass* const       pass,
        const uint32            width,
        const uint32            height,
        const Vector<Texture*>& attachments
    );
    /**
     * @brief Destroy provided render target
     * @param render_target Target to be destroyed
     * @param free_internal_data If true also frees internal render target GPU
     * memory
     */
    void destroy_render_target(
        RenderTarget* const render_target, const bool free_internal_data = true
    );

    /**
     * @brief Create a render pass object
     * @param config Render pass configurations
     * @returns RenderPass* Created render pass
     */
    RenderPass* create_render_pass(const RenderPass::Config& config);
    /**
     * @brief Destroy provided render pass
     * @param pass Render pass to be destroyed
     */
    void        destroy_render_pass(RenderPass* const pass);
    /**
     * @brief Get renderpass with a given name
     *
     * @param name Renderpass name identifier
     * @return RenderPass* Requested renderpass
     * @throws RuntimeError If pass under that name doesn't exist
     */
    Result<RenderPass*, RuntimeError> get_renderpass(const String& name);

  private:
    RendererBackend* _backend = nullptr;

    // TODO: View configurable
    RenderPass* _world_renderpass;
    RenderPass* _ui_renderpass;
};

template<uint8 Dim>
void Renderer::create_geometry(
    Geometry*                  geometry,
    const Vector<Vertex<Dim>>& vertices,
    const Vector<uint32>&      indices
) {
    static const char* const RENDERER_LOG = "Renderer :: ";
    Logger::trace(RENDERER_LOG, "Creating geometry.");
    _backend->create_geometry(geometry, vertices, indices);
    Logger::trace(RENDERER_LOG, "Geometry created [", geometry->name(), "].");
}

} // namespace ENGINE_NAMESPACE