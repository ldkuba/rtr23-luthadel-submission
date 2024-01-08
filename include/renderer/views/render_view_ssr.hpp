// #pragma once

// #include "render_view.hpp"
// #include "systems/shader_system.hpp"
// #include "systems/geometry_system.hpp"

// namespace ENGINE_NAMESPACE {

// class LightSystem;

// class RenderViewSSR : public RenderView {
//   public:
//     RenderViewSSR(
//         const Config&         config,
//         Renderer* const       renderer,
//         TextureSystem* const  texture_system,
//         GeometrySystem* const geometry_system,
//         ShaderSystem* const   shader_system
//     );
//     virtual ~RenderViewSSR() override;

//     virtual Packet* on_build_pocket() override;
//     virtual void    on_resize(const uint32 width, const uint32 height)
//     override; virtual void    on_render(
//            Renderer* const     renderer,
//            const Packet* const packet,
//            const uint64        frame_number,
//            const uint64        render_target_index
//        ) override;

//   protected:
//     Renderer*      _renderer;
//     TextureSystem* _texture_system;

//     Shader*   _shader;
//     float32   _fov;
//     float32   _near_clip;
//     float32   _far_clip;
//     glm::mat4 _proj_matrix;
//     glm::mat4 _proj_inv_matrix;

//     Texture::Map* _color_map = nullptr;
//     Texture::Map* _depth_map = nullptr;

//     Geometry* _full_screen_geometry {};

//     void create_texture_maps();
//     void destroy_texture_maps();

//     // Uniforms
//     struct UIndex {
//         uint16 projection_inverse = -1;
//         uint16 color_texture      = -1;
//         uint16 depth_texture      = -1;

//         UIndex() {}
//         UIndex(const Shader* const shader);
//     };
//     UIndex _u_index {};

//     virtual void apply_globals(const uint64 frame_number) const;
// };

// } // namespace ENGINE_NAMESPACE