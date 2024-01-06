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
        StringEnum WorldPass  = "Renderpass.Builtin.World";
        StringEnum UIPass     = "Renderpass.Builtin.UI";
        StringEnum SkyboxPass = "Renderpass.Builtin.Skybox";
        StringEnum AOPass     = "Renderpass.Builtin.AO";
        StringEnum DepthPass  = "Renderpass.Builtin.Depth";
        StringEnum BlurPass   = "Renderpass.Builtin.Blur";
        StringEnum ShadowmapDirectionalPass = "Renderpass.Builtin.ShadowmapDirectional";
        StringEnum ShadowmapSamplingPass = "Renderpass.Builtin.ShadowmapSampling";
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
        Stencil = 0x4,
        Resolve = 0x8
    };

    /**
     * @brief Render pass configuration structure. Used for initialization of
     * render basses per renderer configuration.
     */
    struct Config {
        String    name;
        glm::vec2 render_offset;
        glm::vec4 clear_color;
        bool      depth_testing;
        bool      multisampling;
    };

  public:
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

    /// @brief True if multisampling is used
    Property<bool> uses_multisampling {
        GET { return _multisampling_enabled; }
    };
    /// @brief True if depth testing is used
    Property<bool> uses_depth_testing {
        GET { return _depth_testing_enabled; }
    };

    /**
     * @brief Construct a new generic Render Pass object
     *
     * @param id Unique render pass identifier
     * @param config Render pass configurations
     */
    RenderPass(const uint16 id, const Config& config)
        : _id(id), _name(config.name), //
          _render_offset(config.render_offset),
          _clear_color(config.clear_color),
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
     * @brief Creates and adds new render targets to this pass. Uses default
     * window attachments for this task. If more than one render target is used,
     * more than one render target will be added.
     */
    virtual void add_window_as_render_target() = 0;

    /**
     * @brief Creates and adds new render target to this pass. Render targets
     * are fully initialized only after render pass initialization.
     *
     * @param config Render target configuration
     */
    virtual void add_render_target(const RenderTarget::Config config) {
        _render_target_configs.push_back(config);
    }

    /**
     * @brief Clear render pass of all associated targets. Targets will be
     * destroyed.
     */
    virtual void clear_render_targets() = 0;

    /**
     * @brief Disable color output for this render pass. Makes render pass
     * output nothing to its color attachments. Should be set before
     * initialization. This function does nothing after initialization.
     */
    void disable_color_output() { _color_output = false; }

    /// @brief Get color attachment index
    virtual uint8 get_color_index()   = 0;
    /// @brief Get depth attachment index
    virtual uint8 get_depth_index()   = 0;
    /// @brief Get resolve attachment index
    virtual uint8 get_resolve_index() = 0;

  private:
    struct RenderPassInitializer {
        RenderPass*   pass;
        ClearFlagType clear_flags {};

        RenderPassInitializer       operator>>(RenderPass* const pass) const;
        const RenderPassInitializer operator>>(const String& clear_flags) const;
        RenderPassInitializer&      operator>>(RenderPass* const next_pass);
        RenderPassInitializer&      operator>>(const String& clear_flags);
        void operator>>(const RenderPassInitializer& init);

      private:
        static ClearFlagType parse_clear_flags(const String& clear_flags);
        void                 update_attachment_info();
    };

  public:
    // Initializer
    /// @brief Start initializing render passes. Signifies frame entry.
    const static RenderPassInitializer start;
    /// @brief Finish render pass initialization. Signifies completed frame.
    const static RenderPassInitializer finish;

  protected:
    uint16                _id;
    glm::vec2             _render_offset {};
    glm::vec4             _clear_color {};
    ClearFlagType         _clear_flags {};
    bool                  _multisampling_enabled;
    bool                  _depth_testing_enabled;
    Vector<RenderTarget*> _render_targets {};

    // State
    bool _color_output = true;
    bool _init_color   = true;
    bool _init_depth   = true;
    bool _init_resolve = true;

    String _name = "";
    String _prev = "";
    String _next = "";

    Vector<RenderTarget::Config> _render_target_configs {};

    virtual void initialize()                = 0;
    virtual void initialize_render_targets() = 0;
};

} // namespace ENGINE_NAMESPACE