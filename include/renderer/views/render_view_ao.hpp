#pragma once

#include "render_view.hpp"
#include "systems/shader_system.hpp"
#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

class LightSystem;

class RenderViewAO : public RenderView {
  public:
    RenderViewAO(
        const Config&         config,
        Renderer* const       renderer,
        TextureSystem* const  texture_system,
        GeometrySystem* const geometry_system,
        ShaderSystem* const   shader_system
    );
    virtual ~RenderViewAO() override;

    virtual Packet* on_build_pocket() override;
    virtual void    on_resize(const uint32 width, const uint32 height) override;
    virtual void    on_render(
           Renderer* const     renderer,
           const Packet* const packet,
           const uint64        frame_number,
           const uint64        render_target_index
       ) override;

  protected:
    Renderer*      _renderer;
    TextureSystem* _texture_system;

    Shader*   _shader;
    float32   _fov;
    float32   _near_clip;
    float32   _far_clip;
    glm::mat4 _proj_matrix;
    glm::mat4 _proj_inv_matrix;
    glm::vec2 _noise_scale;
    float32   _sample_radius;

    Texture::Map* _depth_map = nullptr;
    Texture::Map* _noise_map = nullptr;

    const static uint8                  _kernel_size = 20;
    std::array<glm::vec3, _kernel_size> _kernel;

    Geometry* _full_screen_geometry {};

    void create_texture_maps();
    void destroy_texture_maps();
    void generate_kernel(const uint32 sample_count);

    // Uniforms
    struct UIndex {
        uint16 projection         = -1;
        uint16 projection_inverse = -1;
        uint16 noise_scale        = -1;
        uint16 sample_radius      = -1;
        uint16 kernel             = -1;
        uint16 depth_texture      = -1;
        uint16 noise_texture      = -1;

        UIndex() {}
        UIndex(const Shader* const shader);
    };
    UIndex _u_index {};

    virtual void apply_globals(const uint64 frame_number) const;
};

} // namespace ENGINE_NAMESPACE