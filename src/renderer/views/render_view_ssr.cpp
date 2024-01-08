// #include "renderer/views/render_view_ssr.hpp"

// namespace ENGINE_NAMESPACE {

// #define RENDER_VIEW_SSR_LOG "RenderViewSSR :: "

// // Constructor & Destructor
// RenderViewSSR::RenderViewSSR(
//     const Config&         config,
//     Renderer* const       renderer,
//     TextureSystem* const  texture_system,
//     GeometrySystem* const geometry_system,
//     ShaderSystem* const   shader_system
// )
//     : RenderView(config), _renderer(renderer),
//     _texture_system(texture_system) { auto res =
//     shader_system->acquire(_shader_name); if (res.has_error()) {
//         Logger::error(
//             RENDER_VIEW_SSR_LOG,
//             "Shader `",
//             _shader_name,
//             "` does not exist. View creation is faulty. For now default "
//             "AO shader will be used. This could result in some undefined "
//             "behaviour."
//         );
//         res = shader_system->acquire(Shader::BuiltIn::AOShader);
//     }

//     // Setup values
//     _shader = res.value();

//     // Setup shader indices
//     _u_index = { _shader };

//     _near_clip = 0.1f;   // TODO: TEMP
//     _far_clip  = 1000.f; // TODO: TEMP
//     _fov       = glm::radians(45.f);

//     // Default value, jic, so that it doesn't fail right away
//     _proj_matrix = glm::perspective(
//         _fov, (float32) _width / _height, _near_clip, _far_clip
//     );
//     _proj_inv_matrix = glm::inverse(_proj_matrix);

//     // Create full screen render packet
//     // Size is [2, 2], since shader converts positions: [0, 2] -> [-1, 1]
//     _full_screen_geometry =
//         geometry_system->generate_ui_rectangle("full_screen_geom", 2, 2);

//     //  Create texture maps
//     create_texture_maps();
// }
// RenderViewSSR::~RenderViewSSR() { destroy_texture_maps(); }

// // ///////////////////////////// //
// // RENDER VIEW AO PUBLIC METHODS //
// // ///////////////////////////// //

// RenderView::Packet* RenderViewSSR::on_build_pocket() {
//     return new (MemoryTag::Temp) Packet { this };
// }

// void RenderViewSSR::on_resize(const uint32 width, const uint32 height) {
//     if (width == _width && height == _height) return;

//     _width       = width;
//     _height      = height;
//     _proj_matrix = glm::perspective(
//         _fov, (float32) _width / _height, _near_clip, _far_clip
//     );
//     _proj_inv_matrix = glm::inverse(_proj_matrix);
// }

// void RenderViewSSR::on_render(
//     Renderer* const     renderer,
//     const Packet* const packet,
//     const uint64        frame_number,
//     const uint64        render_target_index
// ) {
//     // Transition used render targets
//     const auto pt1 = static_cast<const PackedTexture*>(_color_map->texture);
//     const auto pt2 = static_cast<const PackedTexture*>(_depth_map->texture);
//     pt1->get_at(frame_number % VulkanSettings::max_frames_in_flight)
//         ->transition_render_target(); // TODO: Vulkan agnostic
//     pt2->get_at(frame_number % VulkanSettings::max_frames_in_flight)
//         ->transition_render_target();

//     for (const auto& pass : _passes) {
//         // Bind pass
//         pass->begin(frame_number % VulkanSettings::max_frames_in_flight);

//         // Setup shader
//         _shader->use();

//         // Apply globals
//         apply_globals(frame_number);

//         // Draw 1 geometry
//         renderer->draw_geometry(_full_screen_geometry);

//         // End pass
//         pass->end();
//     }
// }

// // //////////////////////////////// //
// // RENDER VIEW AO PROTECTED METHODS //
// // //////////////////////////////// //

// void RenderViewSSR::create_texture_maps() {
//     // Depth texture
//     const auto color_texture =
//         _texture_system->acquire("WorldColorTarget", false);

//     // Depth texture
//     const auto depth_texture =
//         _texture_system->acquire("DepthPrePassTarget", false);

//     // Maps
//     _color_map =
//         _renderer->create_texture_map({ color_texture,
//                                         Texture::Use::Unknown,
//                                         Texture::Filter::NearestNeighbour,
//                                         Texture::Filter::NearestNeighbour,
//                                         Texture::Repeat::ClampToEdge,
//                                         Texture::Repeat::ClampToEdge,
//                                         Texture::Repeat::ClampToEdge });
//     _depth_map =
//         _renderer->create_texture_map({ depth_texture,
//                                         Texture::Use::Unknown,
//                                         Texture::Filter::NearestNeighbour,
//                                         Texture::Filter::NearestNeighbour,
//                                         Texture::Repeat::ClampToEdge,
//                                         Texture::Repeat::ClampToEdge,
//                                         Texture::Repeat::ClampToEdge });
// }
// void RenderViewSSR::destroy_texture_maps() {
//     if (_color_map) _renderer->destroy_texture_map(_color_map);
//     if (_depth_map) _renderer->destroy_texture_map(_depth_map);
// }

// #define uniform_index(uniform)                                                 \
//     auto _u_##uniform##_id = shader->get_uniform_index(#uniform);              \
//     if (_u_##uniform##_id.has_error()) {                                       \
//         Logger::error(RENDER_VIEW_SSR_LOG, _u_##uniform##_id.error().what());  \
//     } else uniform = _u_##uniform##_id.value()

// RenderViewSSR::UIndex::UIndex(const Shader* const shader) {
//     uniform_index(projection_inverse);
//     uniform_index(color_texture);
//     uniform_index(depth_texture);
// }

// void RenderViewSSR::apply_globals(const uint64 frame_number) const {
//     // Globals can be updated only once per frame
//     if (frame_number == _shader->rendered_frame_number) return;

//     // Apply globals update
//     _shader->set_uniform(_u_index.projection_inverse, &_proj_inv_matrix);
//     _shader->set_sampler(_u_index.color_texture, _color_map);
//     _shader->set_sampler(_u_index.depth_texture, _depth_map);

//     _shader->apply_global();

//     // Update render frame number
//     _shader->rendered_frame_number = frame_number;
// }

// } // namespace ENGINE_NAMESPACE