#pragma once

#include "renderer/renderer.hpp"
#include "systems/geometry_system.hpp"
#include "systems/light_system.hpp"
#include "systems/shader_system.hpp"
#include "containers/unordered_map.hpp"

namespace ENGINE_NAMESPACE {

class RenderModule {
  public:
    enum class Update {};

    struct PassConfig {
        String shader_instance {};
        String shader {};
        String render_pass {};

        PassConfig() = delete;

        PassConfig(String shader_instance, String shader, String render_pass)
            : shader_instance(shader_instance),
              shader(shader),
              render_pass(render_pass) {}

        PassConfig(String shader, String render_pass) : PassConfig(shader, shader, render_pass) {}
    };

    struct Config {
        Vector<PassConfig> passes;

        Config(Vector<PassConfig> passes) : passes(passes) {}
    };

  public:
    RenderModule(
        Renderer* const       renderer,
        ShaderSystem* const   shader_system,
        TextureSystem* const  texture_system,
        GeometrySystem* const geometry_system,
        LightSystem* const    light_system,
        const Config&         config
    );
    virtual ~RenderModule();

  public:

    /**
     * @brief Build render packet and update internal state.
     * @return Packet* Resulting render packet
     */
    ModulePacket* build_pocket();

    /**
     * @brief Render provided render data
     *
     * @param packet Render data
     * @param frame_number Current frame index. Used for internal
     * synchronization.
     */
    virtual void render(
        const ModulePacket* const packet, const uint64 frame_number
    );

  protected:
    Renderer* const       _renderer;
    ShaderSystem* const   _shader_system;
    TextureSystem* const  _texture_system;
    GeometrySystem* const _geometry_system;
    LightSystem* const    _light_system;

    struct PassInfo {
        Shader*                      shader;
        RenderPass*                  renderpass;
        UnorderedMap<String, uint16> u_index;
    };
    Vector<PassInfo> _renderpasses {};

    void initialize_passes(const Config& config);

    void setup_uniform_index(String uniform, uint32 rp_index);
    void setup_uniform_indices(String uniform);

    /**
     * @brief Build render packet and update internal state.
     * @return Packet* Resulting render packet
     */
    virtual ModulePacket* on_build_pocket();

    /**
     * @brief Render provided render data
     *
     * @param packet Render data
     * @param frame_number Current frame index. Used for internal
     * synchronization.
     */
    virtual void on_render(
        const ModulePacket* const packet,
        const uint64              frame_number,
        uint32                    rp_index
    ) = 0;

    virtual void apply_globals(uint32 rp_index) const {}

    Texture::Map* create_texture_map(
        const String&          texture,
        const Texture::Use&    use,
        const Texture::Filter& filter_minify,
        const Texture::Filter& filter_magnify,
        const Texture::Repeat& repeat_u,
        const Texture::Repeat& repeat_v,
        const Texture::Repeat& repeat_w
    );

    Vector<Texture::Map*> _own_maps {};

  private:
    void apply_globals(uint64 frame_number, uint32 rp_index) const;

    Vector<Texture*> _own_textures {};
};

/**
 * @brief Render data required by given render module. Used by renderer during
 * frame draw.
 */
struct ModulePacket {
    RenderModule* const module;
};

#define UNIFORM_ID(uniform) _renderpasses.at(rp_index).u_index.at(_u_names.uniform)

#define UNIFORM_NAME(uniform) const String uniform = #uniform

} // namespace ENGINE_NAMESPACE