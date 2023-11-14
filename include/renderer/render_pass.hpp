#pragma once

#include "render_target.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Generic render pass representation
 */
class RenderPass {
  public:
    /**
     * @brief List of standard builtin render passes
     */
    struct BuiltIn {
        StringEnum WorldPass = "Renderpass.Builtin.World";
        StringEnum UIPass    = "Renderpass.Builtin.UI";
    };

    /// @brief Type used by clear flags
    typedef uint8 ClearFlagType;

    /**
     * @brief Types of clearing available to the render pass. Combinations will
     * enable multiple clearing types.
     */
    enum ClearFlags : ClearFlagType {
        None    = 0x0,
        Color   = 0x1,
        Depth   = 0x2,
        Stencil = 0x4
    };

    /// @brief Unique render pass identifier
    Property<uint16> id {
        GET { return _id; }
    };

    /// @brief List of associated render targets
    Property<Vector<RenderTarget*>> render_targets {
        GET { return _render_targets; }
    };

    /// @brief Offset from top right of the screen from which we will render in
    /// this pass. Width and height is taken from specified render target
    Property<glm::vec2> render_offset {
        GET { return _render_offset; }
        SET { _render_offset = value; }
    };

    /**
     * @brief Render pass configuration structure. Used for initialization of
     * render basses per renderer configuration.
     */
    struct Config {
        String        name;
        String        prev_name;
        String        next_name;
        glm::vec2     render_offset;
        glm::vec4     clear_color;
        ClearFlagType clear_flags;
        bool          depth_testing;
        bool          multisampling;
    };

    RenderPass(const uint16 id, const Config& config)
        : _id(id), _render_offset(config.render_offset),
          _clear_color(config.clear_color), _clear_flags(config.clear_flags),
          _multisampling_enabled(config.multisampling),
          _depth_testing_enabled(config.depth_testing) {}
    virtual ~RenderPass() {}

    /**
     * @brief Start recording of render pass commands
     * @param index Render target to be used (index of)
     */
    void begin(const uint32 index) {
        if (index >= _render_targets.size())
            Logger::fatal(
                "RenderPass :: Render target index out of range. Target count "
                "is `",
                _render_targets.size(),
                "`, but index `",
                index,
                "` was passed."
            );
        begin(_render_targets[index]);
    }

    /**
     * @brief Start recording of render pass commands
     * @param target Render target to be used
     */
    virtual void begin(RenderTarget* const render_target) = 0;
    /**
     * @brief End recording of render pass commands
     */
    virtual void end()                                    = 0;

    /**
     * @brief Add render target to its associated render pass
     * @param target Target to be added
     */
    void add_render_target(RenderTarget* const target) {
        _render_targets.push_back(target);
    }
    /**
     * @brief Clear render pass of all associated targets
     */
    void clear_render_targets() { _render_targets.clear(); }

  protected:
    uint16                _id;
    glm::vec2             _render_offset {};
    glm::vec4             _clear_color {};
    ClearFlagType         _clear_flags {};
    bool                  _multisampling_enabled;
    bool                  _depth_testing_enabled;
    Vector<RenderTarget*> _render_targets {};
};

} // namespace ENGINE_NAMESPACE