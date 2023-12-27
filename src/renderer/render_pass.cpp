#include "renderer/render_pass.hpp"

namespace ENGINE_NAMESPACE {

#define RP RenderPass
#define RPI RenderPass::RenderPassInitializer

// -----------------------------------------------------------------------------
// Render pass initializer
// -----------------------------------------------------------------------------

const RPI RP::start  = {};
const RPI RP::finish = {};

RPI RPI::operator>>(RP* const pass) const {
    // Set clear colors
    pass->_clear_flags = clear_flags;
    // Create initializer that we will use from now on
    return { pass, {} };
}
const RPI RPI::operator>>(const String& clear_flags) const {
    return { nullptr,
             (ClearFlagType) (this->clear_flags | parse_clear_flags(clear_flags)
             ) };
}
RPI& RPI::operator>>(RP* const next_pass) {
    // Update attachment info
    update_attachment_info();

    // Initialize this pass
    pass->_next = next_pass->_name;
    pass->initialize();
    pass->initialize_render_targets();

    // Update next pass
    next_pass->_prev = pass->_name;
    next_pass->_clear_flags |= clear_flags;

    // Remember next pass
    pass        = next_pass;
    clear_flags = {};
    return *this;
}
RPI& RPI::operator>>(const String& clear_flags) {
    // Remember gathered clear flags
    this->clear_flags |= parse_clear_flags(clear_flags);
    return *this;
}
void RPI::operator>>(const RPI& init) {
    update_attachment_info();
    pass->_next = "";
    pass->initialize();
    pass->initialize_render_targets();
}

RP::ClearFlagType RPI::parse_clear_flags(const String& clear_flags) {
    ClearFlagType flags = {};
    for (const auto& flag : clear_flags) {
        switch (flag) {
        case 'C': flags |= RP::ClearFlags::Color; break;
        case 'D': flags |= RP::ClearFlags::Depth; break;
        case 'S': flags |= RP::ClearFlags::Stencil; break;
        }
    }
    return flags;
}

void RPI::update_attachment_info() {
    // Go trough all render targets and their attachments and check whether
    // previous pass used that attachment. If it did make sure to note that
    for (const auto& config : pass->_render_target_configs) {
        for (uint32 i = 0; i < config.attachments.size(); i++) {
            const auto& att = config.attachments[i];

            // If this is the first use of this attachment do nothing
            if (!att->used_by_render_pass) continue;

            // This attachment was already seen before.
            // Turn off initialization for this attachment
            switch (i) {
            case 0:
                if (pass->_color_output) pass->_init_color = false;
                else pass->_init_depth = false;
                break;
            case 1:
                if (pass->_depth_testing_enabled) pass->_init_depth = false;
                else pass->_init_resolve = false;
                break;
            case 2: pass->_init_resolve = false; break;
            };
        }
    }
    // Mark all attachments as used
    for (const auto& config : pass->_render_target_configs) {
        for (const auto& att : config.attachments)
            att->used_by_render_pass = true;
    }
}

} // namespace ENGINE_NAMESPACE