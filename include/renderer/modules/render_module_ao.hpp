#pragma once

#include "render_module_full_screen.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleAO : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String  g_pre_pass_texture;
        String  depth_texture;
        float32 sample_radius = 1.5f;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view,
            String                           g_pre_pass_texture,
            String                           depth_texture
        )
            : RenderModuleFullScreen::Config(passes, perspective_view),
              g_pre_pass_texture(g_pre_pass_texture),
              depth_texture(depth_texture) {
        }
    };

  public:
    using RenderModuleFullScreen::RenderModuleFullScreen;

    void initialize(const Config& config);

  protected:
    void on_render(
        const ModulePacket* const packet,
        const uint64              frame_number,
        uint32                    rp_index
    ) override;
    void apply_globals(uint32 rp_index) const override;

  private:
    Texture::Map* _g_map     = nullptr;
    Texture::Map* _depth_map = nullptr;
    Texture::Map* _noise_map = nullptr;
    glm::vec2     _noise_scale;
    float32       _sample_radius;

    const static uint8                  _kernel_size = 64;
    std::array<glm::vec3, _kernel_size> _kernel;

    struct Uniforms {
        UNIFORM_NAME(projection);
        UNIFORM_NAME(projection_inverse);
        UNIFORM_NAME(noise_scale);
        UNIFORM_NAME(sample_radius);
        UNIFORM_NAME(kernel);
        UNIFORM_NAME(g_pre_pass_texture);
        UNIFORM_NAME(depth_texture);
        UNIFORM_NAME(noise_texture);
    };
    Uniforms _u_names {};

    void setup_texture_maps(
        const String& g_pre_pass_texture, const String& depth_texture
    );
    void generate_kernel();
    void compute_noise_scale();
};

} // namespace ENGINE_NAMESPACE