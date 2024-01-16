#pragma once

#include "render_module_full_screen.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleAO : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String  g_pre_pass_texture;
        String  depth_texture;
        float32 sample_radius = 1.5f;
    };

  public:
    using RenderModuleFullScreen::RenderModuleFullScreen;

    void initialize(const Config& config);

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override;
    void apply_globals() const override;

  private:
    Texture::Map* _g_map     = nullptr;
    Texture::Map* _depth_map = nullptr;
    Texture::Map* _noise_map = nullptr;
    glm::vec2     _noise_scale;
    float32       _sample_radius;

    const static uint8                  _kernel_size = 64;
    std::array<glm::vec3, _kernel_size> _kernel;

    struct UIndex {
        uint16 projection         = -1;
        uint16 projection_inverse = -1;
        uint16 noise_scale        = -1;
        uint16 sample_radius      = -1;
        uint16 kernel             = -1;
        uint16 g_pre_pass_texture = -1;
        uint16 depth_texture      = -1;
        uint16 noise_texture      = -1;
    };
    UIndex _u_index {};

    void setup_texture_maps(
        const String& g_pre_pass_texture, const String& depth_texture
    );
    void generate_kernel();
    void compute_noise_scale();
};

} // namespace ENGINE_NAMESPACE