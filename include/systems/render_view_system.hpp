#pragma once

#include "renderer/views/render_view.hpp"
#include "unordered_map.hpp"

namespace ENGINE_NAMESPACE {

class ShaderSystem;

/**
 * @brief System for render view management
 */
class RenderViewSystem {
  public:
    RenderViewSystem(
        Renderer* const renderer, Platform::Surface* const surface
    );
    ~RenderViewSystem();

    /**
     * @brief Create render view
     *
     * @param config Render view creation configuration. Name must be unique.
     * @return RenderView* if render view creation concluded successfully
     * @throw RuntimeException Otherwise
     */
    Result<RenderView*, RuntimeError> create(const RenderView::Config& config);
    /**
     * @brief Acquire preexisting render view
     *
     * @param name Name of requested render view
     * @return RenderView* If render view with a given name exists
     * @throw RuntimeError Otherwise
     */
    Result<RenderView*, RuntimeError> acquire(const String& name);

  private:
    Renderer* const _renderer;

    UnorderedMap<String, RenderView*> _registered_views;

    void on_window_resize(const uint32 width, const uint32 height);
    Result<void, RuntimeError> name_is_valid(const String& name);
};

} // namespace ENGINE_NAMESPACE