#pragma once

#include "renderer/renderer.hpp"
#include "systems/geometry_system.hpp"
#include "systems/light_system.hpp"
#include "systems/shader_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModule {
  public:
    enum class Update {};

    struct Config {
        const String shader;
        const String render_pass;
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
    void render(const ModulePacket* const packet, const uint64 frame_number);

    virtual void initialize(const Config& config) {};

  protected:
    Renderer* const       _renderer;
    ShaderSystem* const   _shader_system;
    TextureSystem* const  _texture_system;
    GeometrySystem* const _geometry_system;
    LightSystem* const    _light_system;

    RenderPass* _renderpass;
    Shader*     _shader;

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
        const ModulePacket* const packet, const uint64 frame_number
    ) = 0;

    virtual void apply_globals() const {}

    Texture::Map* create_texture_map(
        const String&          texture,
        const Texture::Use&    use,
        const Texture::Filter& filter_minify,
        const Texture::Filter& filter_magnify,
        const Texture::Repeat& repeat_u,
        const Texture::Repeat& repeat_v,
        const Texture::Repeat& repeat_w
    );

  private:
    void apply_globals(uint64 frame_number) const;

    Vector<Texture*>      _own_textures {};
    Vector<Texture::Map*> _own_maps {};
};

/**
 * @brief Render data required by given render module. Used by renderer during
 * frame draw.
 */
struct ModulePacket {
    RenderModule* const module;
};

#define SETUP_UNIFORM_INDEX(uniform)                                           \
    auto _u_##uniform##_id = _shader->get_uniform_index(#uniform);             \
    if (_u_##uniform##_id.has_error())                                         \
        Logger::error("ShaderModule :: ", _u_##uniform##_id.error().what());   \
    else _u_index.uniform = _u_##uniform##_id.value()

} // namespace ENGINE_NAMESPACE