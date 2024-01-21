#include "systems/render_view_system.hpp"

#include "renderer/views/render_view_perspective.hpp"
#include "renderer/views/render_view_orthographic.hpp"
#include "renderer/views/render_view_directional_shadow.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_SYS_LOG "RenderViewSystem :: "

// Constructor & Destructor
RenderViewSystem::RenderViewSystem(
    Renderer* const renderer, Platform::Surface* const surface
)
    : _renderer(renderer) {
    surface->resize_event.subscribe(this, &RenderViewSystem::on_window_resize);
}
RenderViewSystem::~RenderViewSystem() {
    for (const auto& view : _registered_views)
        del(view.second);
}

// ///////////////////////////////// //
// RENDER VIEW SYSTEM PUBLIC METHODS //
// ///////////////////////////////// //

Result<RenderView*, RuntimeError> RenderViewSystem::create(
    const RenderView::Config& config
) {
    Logger::trace(
        RENDER_VIEW_SYS_LOG,
        "Creation of render view `",
        config.name,
        "` requested."
    );

    // Validate config
    const auto name_check_res = name_is_valid(config.name);
    if (name_check_res.has_error()) return Failure(name_check_res.error());

    // Make sure this name isn't used already
    if (_registered_views.contains(config.name)) {
        const auto error_message = String::build(
            "View creation failed. RenderView name provided by the config is "
            "already taken. RenderView `",
            config.name,
            "` already exists"
        );
        Logger::error(RENDER_VIEW_SYS_LOG, error_message);
        return Failure(error_message);
    }

    // Create new render view
    RenderView* view {};
    // TODO: temp, should be done by a factory
    switch (config.type) {
    case RenderView::Type::DefaultPerspective:
        view = new (MemoryTag::RenderView) RenderViewPerspective(config);
        break;
    case RenderView::Type::DefaultOrthographic:
        // TODO: temp just default camera
        view = new (MemoryTag::RenderView) RenderViewOrthographic(config);
        break;
    case RenderView::Type::DirectionalShadow:
        view = new (MemoryTag::RenderView) RenderViewDirectionalShadow(config);
        break;
    default:
        const auto error_message = String::build(
            "View creation failed. Unimplemented view type of `",
            (uint32) config.type,
            "` passed while creating render view `",
            config.name,
            "`."
        );
        Logger::error(RENDER_VIEW_SYS_LOG, error_message);
        return Failure(error_message);
    }

    // Register
    _registered_views[config.name] = view;

    // Return
    Logger::trace(
        RENDER_VIEW_SYS_LOG, "Render view `", config.name, "` created."
    );
    return view;
}

Result<RenderView*, RuntimeError> RenderViewSystem::acquire( //
    const String& name
) {
    Logger::trace(RENDER_VIEW_SYS_LOG, "Render view `", name, "` requested.");

    // Check if name is valid
    const auto name_check_res = name_is_valid(name);
    if (name_check_res.has_error()) return Failure(name_check_res.error());

    // Find requested view
    const auto it = _registered_views.find(name);

    // Not found
    if (it == _registered_views.end()) {
        const auto error_message = String::build(
            "View acquisition failed. No RenderView named `", name, "` found."
        );
        Logger::error(RENDER_VIEW_SYS_LOG, error_message);
        return Failure(error_message);
    }

    // View is found, return it
    Logger::trace(RENDER_VIEW_SYS_LOG, "Render view `", name, "` acquired.");
    return it->second;
}

// ////////////////////////////////// //
// RENDER VIEW SYSTEM PRIVATE METHODS //
// ////////////////////////////////// //

void RenderViewSystem::on_window_resize(
    const uint32 width, const uint32 height
) {
    for (auto& view : _registered_views)
        view.second->on_resize(width, height);
}

Result<void, RuntimeError> RenderViewSystem::name_is_valid(const String& name) {
    if (name.empty()) {
        const auto error = String::build(
            "Render view system operation failed. Empty name not allowed."
        );
        Logger::error(RENDER_VIEW_SYS_LOG, error);
        return Failure(error);
    }
    return {};
}

} // namespace ENGINE_NAMESPACE