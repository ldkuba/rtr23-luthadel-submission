#include "renderer/modules/render_module_ao.hpp"

#include "random.hpp"

namespace ENGINE_NAMESPACE {

// /////////////////////////////// //
// RENDER MODULE AO PUBLIC METHODS //
// /////////////////////////////// //

void RenderModuleAO::initialize(const Config& config) {
    RenderModuleFullScreen::initialize(config);
    _sample_radius = config.sample_radius;

    setup_texture_maps(config.g_pre_pass_texture);
    compute_noise_scale();
    generate_kernel();

    SETUP_UNIFORM_INDEX(projection);
    SETUP_UNIFORM_INDEX(projection_inverse);
    SETUP_UNIFORM_INDEX(noise_scale);
    SETUP_UNIFORM_INDEX(sample_radius);
    SETUP_UNIFORM_INDEX(kernel);
    SETUP_UNIFORM_INDEX(depth_texture);
    SETUP_UNIFORM_INDEX(noise_texture);
}

// ////////////////////////////////// //
// RENDER MODULE AO PROTECTED METHODS //
// ////////////////////////////////// //

void RenderModuleAO::on_render(
    const ModulePacket* const packet, const uint64 frame_number
) {
    // Update noise scale if resolution changed
    if (_perspective_view->updated()) compute_noise_scale();

    // Perform default full screen render
    RenderModuleFullScreen::on_render(packet, frame_number);
}

void RenderModuleAO::apply_globals() const {
    _shader->set_uniform(
        _u_index.projection, &_perspective_view->proj_matrix()
    );
    _shader->set_uniform(
        _u_index.projection_inverse, &_perspective_view->proj_inv_matrix()
    );
    _shader->set_uniform(_u_index.noise_scale, &_noise_scale);
    _shader->set_uniform(_u_index.sample_radius, &_sample_radius);
    _shader->set_uniform(_u_index.kernel, &_kernel);
    _shader->set_sampler(_u_index.depth_texture, _g_map);
    _shader->set_sampler(_u_index.noise_texture, _noise_map);
}

// //////////////////////////////// //
// RENDER MODULE AO PRIVATE METHODS //
// //////////////////////////////// //

void RenderModuleAO::setup_texture_maps(const String& g_pre_pass_texture) {
    // Create noise texture
    ubyte* const texture_data = new (MemoryTag::Temp) ubyte[16 * 4];
    for (uint8 i = 0; i < 16; i++) {
        texture_data[i * 4 + 0] = Random::uint8();
        texture_data[i * 4 + 1] = Random::uint8();
        texture_data[i * 4 + 2] = 128;
        texture_data[i * 4 + 3] = 255;
    }

    const auto noise_texture = _texture_system->create(
        {
            .name          = "SSAO_noise",
            .width         = 4,    // Width
            .height        = 4,    // Height
            .channel_count = 4,    // Channel count
            .is_wrapped    = true, // Writable
        },
        (byte*) texture_data
    );

    // Create maps
    _g_map = create_texture_map(
        g_pre_pass_texture,
        Texture::Use::MapPassResult,
        Texture::Filter::BiLinear,
        Texture::Filter::BiLinear,
        Texture::Repeat::ClampToEdge,
        Texture::Repeat::ClampToEdge,
        Texture::Repeat::ClampToEdge
    );
    _noise_map = create_texture_map(
        noise_texture->name,
        Texture::Use::MapPassResult,
        Texture::Filter::BiLinear,
        Texture::Filter::BiLinear,
        Texture::Repeat::Repeat,
        Texture::Repeat::Repeat,
        Texture::Repeat::Repeat
    );

    // Clean resources
    del(texture_data);
}

void RenderModuleAO::generate_kernel() {
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

void RenderModuleAO::compute_noise_scale() {
    auto width   = _perspective_view->width();
    auto height  = _perspective_view->height();
    width        = std::max(width / 2, 1u);
    height       = std::max(height / 2, 1u);
    _noise_scale = glm::vec2(width, height);
}

} // namespace ENGINE_NAMESPACE