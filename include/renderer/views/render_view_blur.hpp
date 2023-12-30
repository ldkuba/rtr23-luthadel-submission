#pragma once

#include "render_view.hpp"
#include "systems/shader_system.hpp"
#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

class LightSystem;

class RenderViewBlur : public RenderView {
  public:
    RenderViewBlur(
        const Config&         config,
        Renderer* const       renderer,
        TextureSystem* const  texture_system,
        GeometrySystem* const geometry_system,
        ShaderSystem* const   shader_system
    );
    virtual ~RenderViewBlur() override;

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

    Shader*       _shader;
    Texture::Map* _ssao_map = nullptr;
    Geometry*     _full_screen_geometry {};

    // Uniforms
    struct UIndex {
        uint16 target_texture = -1;

        UIndex() {}
        UIndex(const Shader* const shader);
    };
    UIndex _u_index {};

    virtual void apply_globals(const uint64 frame_number) const;
};

} // namespace ENGINE_NAMESPACE