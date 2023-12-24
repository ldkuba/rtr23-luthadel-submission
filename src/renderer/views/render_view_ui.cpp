#include "renderer/views/render_view_ui.hpp"

#include "resources/mesh.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_UI_LOG "RenderViewUI :: "

RenderViewUI::RenderViewUI(
    ShaderSystem* const shader_system, const Config& config
)
    : RenderView(config) {
    auto res = shader_system->acquire(_shader_name);
    if (res.has_error()) {
        Logger::error(
            RENDER_VIEW_UI_LOG,
            "Shader `",
            _shader_name,
            "` does not exist. View creation is faulty. For now default UI "
            "shader will be used. This could result in some undefined "
            "behaviour."
        );
        res = shader_system->acquire(Shader::BuiltIn::UIShader);
    }

    // Setup values
    _shader    = res.value();
    _near_clip = -100.f; // TODO: TEMP
    _far_clip  = 100.f;  // TODO: TEMP

    // Default value, jic, so that it doesn't fail right away
    _view_matrix = glm::identity<glm::mat4>();
    _proj_matrix = glm::ortho(
        0.0f, (float32) _width, (float32) _height, 0.0f, _near_clip, _far_clip
    );
}
RenderViewUI::~RenderViewUI() {}

// ///////////////////////////// //
// RENDER VIEW UI PUBLIC METHODS //
// ///////////////////////////// //

RenderView::Packet* RenderViewUI::on_build_pocket() {
    _geom_data.clear();

    // Check if render data is set
    if (_render_data == nullptr) {
        Logger::warning(
            RENDER_VIEW_UI_LOG,
            "Render data not set for view `",
            _name,
            "`. Not much will be drawn."
        );
        return new (MemoryTag::Temp) Packet { this };
    }

    // Update render data
    for (const auto& mesh : _render_data->meshes) {
        const auto model_matrix = mesh->transform.world();
        for (auto* const geom : mesh->geometries())
            _geom_data.push_back({ geom, geom->material, model_matrix });
    }

    return new (MemoryTag::Temp) Packet { this };
}

void RenderViewUI::on_resize(const uint32 width, const uint32 height) {
    if (width == _width && height == _height) return;

    _width       = width;
    _height      = height;
    _proj_matrix = glm::ortho(
        0.0f, (float32) _width, (float32) _height, 0.f, _near_clip, _far_clip
    );
}

void RenderViewUI::on_render(
    Renderer* const     renderer,
    const Packet* const packet,
    const uint64        frame_number,
    const uint64        render_target_index
) {
    for (const auto& pass : _passes) {
        // Bind pass
        pass->begin(render_target_index);

        // Setup shader
        _shader->use();

        // Apply globals
        apply_globals(frame_number);

        // Draw geometries
        for (const auto& geo_data : _geom_data) {
            // Update material instance
            Material* const geo_material = geo_data.geometry->material;
            geo_material->apply_instance();

            const auto a = _proj_matrix * glm::vec4(0, 0, 0, 1);
            const auto b = _proj_matrix * glm::vec4(128, 128, 0, 1);

            // Apply local
            apply_locals(geo_data.model);

            // Draw geometry
            renderer->draw_geometry(geo_data.geometry);
        }

        // End pass
        pass->end();
    }
}

// //////////////////////////////// //
// RENDER VIEW UI PROTECTED METHODS //
// //////////////////////////////// //

#define set_uniform(uniform_name, uniform_value)                               \
    {                                                                          \
        auto uniform_id_res = _shader->get_uniform_index(uniform_name);        \
        if (uniform_id_res.has_error()) {                                      \
            Logger::error(                                                     \
                RENDER_VIEW_UI_LOG,                                            \
                "Shader set_uniform method failed. No uniform is named \"",    \
                uniform_name,                                                  \
                "\". Nothing was done."                                        \
            );                                                                 \
            return;                                                            \
        }                                                                      \
        auto uniform_id = uniform_id_res.value();                              \
        auto set_result = _shader->set_uniform(uniform_id, &uniform_value);    \
        if (set_result.has_error()) {                                          \
            Logger::error(                                                     \
                RENDER_VIEW_UI_LOG,                                            \
                "Shader set_uniform method failed for \"",                     \
                uniform_name,                                                  \
                "\". Nothing was done"                                         \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

void RenderViewUI::apply_globals(const uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Apply globals update
    set_uniform("projection", _proj_matrix);
    set_uniform("view", _view_matrix);
    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}
void RenderViewUI::apply_locals(const glm::mat4 model) const {
    set_uniform("model", model);
}

} // namespace ENGINE_NAMESPACE