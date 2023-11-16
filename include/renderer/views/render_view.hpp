#pragma once

#include "renderer/renderer.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Generic render view class. Responsible for generation of render view
 * packets.
 */
class RenderView {
  public:
    /**
     * @brief Render view known types. They have logic associated.
     */
    enum class Type { World, UI, Skybox, Custom };
    /**
     * @brief Render view known `view matrix` source.
     */
    enum class ViewMatrixSource { SceneCamera, UICamera, LightCamera, Custom };
    /**
     * @brief Render view known `projection matrix` source.
     */
    enum class ProjectionMatrixSource {
        DefaultPerspective,
        DefaultOrthographic,
        Custom
    };

    struct Config {
        String name;
        String shader_name;
        uint32 width;  // Set to 0 for 100% width
        uint32 height; // Set to 0 for 100% height
        Type   type;

        ViewMatrixSource       view_matrix_src;
        ProjectionMatrixSource proj_matrix_src;

        Vector<RenderPass*> passes;
    };

  public:
    RenderView(const Config& config)
        : _name(config.name), _width(config.width), _height(config.height),
          _type(config.type), _shader_name(config.shader_name) {
        _passes.resize(config.passes.size());
        for (uint32 i = 0; i < _passes.size(); i++)
            _passes[i] = config.passes[i];
    }
    virtual ~RenderView() {}

    virtual RenderViewPacket on_build_pocket()                      = 0;
    virtual void on_resize(const uint32 width, const uint32 height) = 0;
    virtual void on_render(
        Renderer* const         renderer,
        const RenderViewPacket& packet,
        const uint64            frame_number,
        const uint64            render_target_index
    ) = 0;

  protected:
    String _name;
    uint32 _width;
    uint32 _height;
    Type   _type;
    String _shader_name;

    Vector<RenderPass*> _passes {};
};

} // namespace ENGINE_NAMESPACE