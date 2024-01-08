#pragma once

#include "renderer/render_pass.hpp"

namespace ENGINE_NAMESPACE {

class Renderer;

/**
 * @brief Generic render view class. Responsible for generation of render view
 * packets.
 */
class RenderView {
  public:
    /**
     * @brief Render view known types. They have logic associated.
     */
    enum class Type { DefaultPerspective, DefaultOrthographic, Custom };

    /**
     * @brief Configuration for creation of generic render view
     */
    struct Config {
        String name;
        uint32 width;
        uint32 height;
        Type   type;
    };

  public:
    /// @brief View width in pixels
    Property<uint32> width {
        GET { return _width; }
    };
    /// @brief View height in pixels
    Property<uint32> height {
        GET { return _height; }
    };
    /// @brief True if view was updated
    Property<bool> updated {
        GET { return _updated; }
    };
    /// @brief View projection matrix
    Property<glm::mat4> proj_matrix {
        GET { return _proj_matrix; }
    };
    /// @brief Inverse view projection matrix
    Property<glm::mat4> proj_inv_matrix {
        GET { return _proj_inv_matrix; }
    };

    RenderView(const Config& config)
        : _name(config.name), _width(config.width), _height(config.height),
          _type(config.type) {}
    virtual ~RenderView() {}

    /**
     * @brief Callback called upon screen resize
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void on_resize(const uint32 width, const uint32 height) {
        _width   = width;
        _height  = height;
        _updated = true;
    }

    /**
     * @brief Setup a list of objects which could potentially be seen by this
     * view. All non-mentioned meshes will be invisible to it.
     * @param meshes List of potentially visible meshes
     */
    void set_visible_meshes(const Vector<Mesh*>& meshes) {
        // Copy references over
        _potentially_visible_meshes.resize(meshes.size());
        for (uint64 i = 0; i < meshes.size(); i++)
            _potentially_visible_meshes[i] = meshes[i];
    }

    /**
     * @brief Get render data of geometries that are currently within view.
     * Represents a subset of all potentially visible geometries.
     * @param frame_number Index of the current frame. Internally values wont be
     * recomputed if prompted twice on the same frame.
     * @return Vector<GeometryRenderData>& Visible render data
     */
    virtual Vector<GeometryRenderData>& get_visible_render_data(
        const uint32 frame_number
    ) = 0;

  protected:
    String    _name;
    uint32    _width;
    uint32    _height;
    Type      _type;
    glm::mat4 _proj_matrix;
    glm::mat4 _proj_inv_matrix;
    uint64    _last_frame = -1;

    Vector<Mesh*>              _potentially_visible_meshes {};
    Vector<GeometryRenderData> _visible_render_data {};

    bool _updated;
};

} // namespace ENGINE_NAMESPACE