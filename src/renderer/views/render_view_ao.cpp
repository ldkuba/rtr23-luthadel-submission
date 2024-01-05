#include "renderer/views/render_view_ao.hpp"

#include "random.hpp"
#include "resources/mesh.hpp"
#include "multithreading/parallel.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_AO_LOG "RenderViewAO :: "

// Constructor & Destructor
RenderViewAO::RenderViewAO(
    const Config&         config,
    Renderer* const       renderer,
    TextureSystem* const  texture_system,
    GeometrySystem* const geometry_system,
    ShaderSystem* const   shader_system
)
    : RenderView(config), _renderer(renderer), _texture_system(texture_system) {
    auto res = shader_system->acquire(_shader_name);
    if (res.has_error()) {
        Logger::error(
            RENDER_VIEW_AO_LOG,
            "Shader `",
            _shader_name,
            "` does not exist. View creation is faulty. For now default "
            "AO shader will be used. This could result in some undefined "
            "behaviour."
        );
        res = shader_system->acquire(Shader::BuiltIn::AOShader);
    }

    // Setup values
    _shader = res.value();

    // Setup shader indices
    _u_index = { _shader };

    _near_clip = 0.1f;   // TODO: TEMP
    _far_clip  = 1000.f; // TODO: TEMP
    _fov       = glm::radians(45.f);

    // Default value, jic, so that it doesn't fail right away
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);

    // Setup SSAO parameters
    _sample_radius = 2.0f;
    _noise_scale   = glm::vec2(_width / 2.f, _height / 2.f);

    // Create full screen render packet
    // Size is [2, 2], since shader converts positions: [0, 2] -> [-1, 1]
    _full_screen_geometry =
        geometry_system->generate_ui_rectangle("full_screen_geom", 2, 2);

    //  Create texture maps
    create_texture_maps();

    // Generate kernel
    generate_kernel(_kernel_size);
}
RenderViewAO::~RenderViewAO() { destroy_texture_maps(); }

// ///////////////////////////// //
// RENDER VIEW AO PUBLIC METHODS //
// ///////////////////////////// //

RenderView::Packet* RenderViewAO::on_build_pocket() {
    return new (MemoryTag::Temp) Packet { this };
}

void RenderViewAO::on_resize(const uint32 width, const uint32 height) {
    if (width == _width && height == _height) return;

    _width       = width;
    _height      = height;
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);

    _noise_scale = glm::vec2(_width / 2.f, _height / 2.f);
}

void RenderViewAO::on_render(
    Renderer* const     renderer,
    const Packet* const packet,
    const uint64        frame_number,
    const uint64        render_target_index
) {
    // Transition used render targets
    const auto pt = static_cast<const PackedTexture*>(_depth_map->texture);
    pt->get_at(frame_number % VulkanSettings::max_frames_in_flight)
        ->transition_render_target(); // TODO: Vulkan agnostic

    for (const auto& pass : _passes) {
        // Bind pass
        pass->begin(frame_number % VulkanSettings::max_frames_in_flight);
        // pass->begin(render_target_index);

        // Setup shader
        _shader->use();

        // Apply globals
        apply_globals(frame_number);

        // Draw 1 geometry
        renderer->draw_geometry(_full_screen_geometry);

        // End pass
        pass->end();
    }
}

// //////////////////////////////// //
// RENDER VIEW AO PROTECTED METHODS //
// //////////////////////////////// //

void RenderViewAO::create_texture_maps() {
    // Depth texture
    const auto depth_texture =
        _texture_system->acquire("DepthPrePassTarget", false);

    // Noise texture
    ubyte* const texture_data = new (MemoryTag::Temp) ubyte[16 * 4];
    for (uint8 i = 0; i < 16; i++) {
        texture_data[i * 4 + 0] = Random::uint8();
        texture_data[i * 4 + 1] = Random::uint8();
        texture_data[i * 4 + 2] = 128;
        texture_data[i * 4 + 3] = 255;
    }

    const auto noise_texture = _texture_system->create(
        { "SSAO_noise",
          4, // Width
          4, // Height
          4, // Channel count
          Texture::Format::RGBA8Unorm,
          false, // Mip-mapped
          false, // Transparency
          false, // Writable
          false, // Wrapped
          false, // Render target
          Texture::Type::T2D },
        (byte*) texture_data,
        true
    );

    // Maps
    _depth_map =
        _renderer->create_texture_map({ depth_texture,
                                        Texture::Use::Unknown,
                                        Texture::Filter::NearestNeighbour,
                                        Texture::Filter::NearestNeighbour,
                                        Texture::Repeat::ClampToEdge,
                                        Texture::Repeat::ClampToEdge,
                                        Texture::Repeat::ClampToEdge });
    _noise_map = _renderer->create_texture_map({ noise_texture,
                                                 Texture::Use::Unknown,
                                                 Texture::Filter::BiLinear,
                                                 Texture::Filter::BiLinear,
                                                 Texture::Repeat::Repeat,
                                                 Texture::Repeat::Repeat,
                                                 Texture::Repeat::Repeat });

    // Clean resources
    del(texture_data);
}
void RenderViewAO::destroy_texture_maps() {
    if (_depth_map) _renderer->destroy_texture_map(_depth_map);
    if (_noise_map) _renderer->destroy_texture_map(_noise_map);
}

void RenderViewAO::generate_kernel(const uint32) {
    uint32 i = 0;
    for (auto& sample : _kernel) {
        // Generate random sample
        sample.x = Random::float32(-1.0, 1.0);
        sample.y = Random::float32(-1.0, 1.0);
        sample.z = Random::float32_01();

        // Normalize it
        sample = glm::normalize(sample);

        // After normalization the sample points lie on the surface of the
        // hemisphere and each sample point vector has the same length. We want
        // to randomly change the sample points to sample more points inside the
        // hemisphere as close to our fragment as possible. we will use an
        // accelerating interpolation to do this.
        float32 scale               = (float32) i++ / _kernel_size;
        float32 interpolation_scale = 0.1 + 0.9 * scale * scale;

        sample *= interpolation_scale;
    }
}

#define uniform_index(uniform)                                                 \
    auto _u_##uniform##_id = shader->get_uniform_index(#uniform);              \
    if (_u_##uniform##_id.has_error()) {                                       \
        Logger::error(RENDER_VIEW_AO_LOG, _u_##uniform##_id.error().what());   \
    } else uniform = _u_##uniform##_id.value()

RenderViewAO::UIndex::UIndex(const Shader* const shader) {
    uniform_index(projection);
    uniform_index(projection_inverse);
    uniform_index(noise_scale);
    uniform_index(sample_radius);
    uniform_index(kernel);
    uniform_index(depth_texture);
    uniform_index(noise_texture);
}

#define uniform_set(uniform, value)                                            \
    if (_u_index.uniform != (uint16) -1) {                                     \
        const auto res_##uniform =                                             \
            _shader->set_uniform(_u_index.uniform, &value);                    \
        if (res_##uniform.has_error()) {                                       \
            Logger::error(RENDER_VIEW_AO_LOG, res_##uniform.error().what());   \
            return;                                                            \
        }                                                                      \
    }
#define sampler_set(sampler, texture_map)                                      \
    if (_u_index.sampler != (uint16) -1) {                                     \
        const auto res_##sampler =                                             \
            _shader->set_sampler(_u_index.sampler, texture_map);               \
        if (res_##sampler.has_error()) {                                       \
            Logger::error(RENDER_VIEW_AO_LOG, res_##sampler.error().what());   \
            return;                                                            \
        }                                                                      \
    }

void RenderViewAO::apply_globals(const uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Apply globals update
    uniform_set(projection, _proj_matrix);
    uniform_set(projection_inverse, _proj_inv_matrix);
    uniform_set(noise_scale, _noise_scale);
    uniform_set(sample_radius, _sample_radius);
    uniform_set(kernel, _kernel);
    sampler_set(depth_texture, _depth_map);
    sampler_set(noise_texture, _noise_map);

    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}

} // namespace ENGINE_NAMESPACE