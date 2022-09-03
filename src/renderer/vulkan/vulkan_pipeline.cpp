#include "renderer/vulkan/vulkan_backend.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"
#include <fstream>

std::vector<byte> read_file(const std::string& filepath);

void VulkanBackend::create_render_pass() {
    // Color attachment
    vk::AttachmentDescription color_attachment{};
    color_attachment.setFormat(_swapchain_format);
    color_attachment.setSamples(_msaa_samples);
    color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    color_attachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    color_attachment_ref.setAttachment(0);

    // Depth attachment
    vk::AttachmentDescription depth_attachment{};
    depth_attachment.setFormat(find_depth_format());
    depth_attachment.setSamples(_msaa_samples);
    depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depth_attachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    depth_attachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference depth_attachment_ref{};
    depth_attachment_ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    depth_attachment_ref.setAttachment(1);

    // Resolve attachment
    vk::AttachmentDescription color_attachment_resolve{};
    color_attachment_resolve.setFormat(_swapchain_format);
    color_attachment_resolve.setSamples(vk::SampleCountFlagBits::e1);
    color_attachment_resolve.setLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment_resolve.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment_resolve.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment_resolve.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment_resolve.setInitialLayout(vk::ImageLayout::eUndefined);
    color_attachment_resolve.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_attachment_resolve_ref{};
    color_attachment_resolve_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    color_attachment_resolve_ref.setAttachment(2);

    // Subpass
    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(&color_attachment_ref);
    subpass.setPDepthStencilAttachment(&depth_attachment_ref);
    subpass.setPResolveAttachments(&color_attachment_resolve_ref);

    // Subpass dependencies
    vk::SubpassDependency dependency{};
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setDstStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    dependency.setDstAccessMask(
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    // Create subpass
    std::array<vk::AttachmentDescription, 3> attachments = {
        color_attachment,
        depth_attachment,
        color_attachment_resolve
    };

    vk::RenderPassCreateInfo create_info{};
    create_info.setAttachments(attachments);
    create_info.setSubpassCount(1);
    create_info.setPSubpasses(&subpass);
    create_info.setDependencyCount(1);
    create_info.setPDependencies(&dependency);

    auto result = _device->handle.createRenderPass(&create_info, _allocator, &_render_pass);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create a render pass.");
}

void VulkanBackend::create_pipeline() {
    auto vertex_code = read_file("shaders/simple_vertex_shader.vert.spv");
    auto fragment_code = read_file("shaders/simple_fragment_shader.frag.spv");

    // Vertex and fragment shaders
    auto vertex_shader_module = create_shader_module(vertex_code);
    auto fragment_shader_module = create_shader_module(fragment_code);

    vk::PipelineShaderStageCreateInfo vertex_shader_stage_info;
    vertex_shader_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
    vertex_shader_stage_info.setModule(vertex_shader_module);
    vertex_shader_stage_info.setPName("main");
    vertex_shader_stage_info.setPSpecializationInfo(nullptr);           // Set initial shader constants

    vk::PipelineShaderStageCreateInfo fragment_shader_stage_info;
    fragment_shader_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
    fragment_shader_stage_info.setModule(fragment_shader_module);
    fragment_shader_stage_info.setPName("main");
    fragment_shader_stage_info.setPSpecializationInfo(nullptr);


    vk::PipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info, fragment_shader_stage_info };

    // Vertex input
    auto binding_description = Vertex::get_binding_description();
    auto attribute_description = Vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.setVertexBindingDescriptionCount(1);
    vertex_input_info.setPVertexBindingDescriptions(&binding_description);
    vertex_input_info.setVertexAttributeDescriptionCount(static_cast<uint32>(attribute_description.size()));
    vertex_input_info.setVertexAttributeDescriptions(attribute_description);

    // Input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.setTopology(vk::PrimitiveTopology::eTriangleList);
    input_assembly_info.setPrimitiveRestartEnable(false);

    // Viewport and scissors
    vk::PipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.setViewportCount(1);
    viewport_state_info.setScissorCount(1);

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterization_info{};
    rasterization_info.setDepthClampEnable(false);               // Clamp values beyond far/near planes instead of discarding them (feature required for enabling)
    rasterization_info.setRasterizerDiscardEnable(false);        // Disable output to framebuffer (feature required for enabling)
    rasterization_info.setPolygonMode(vk::PolygonMode::eFill);   // Determines how fragments are generated for geometry (feature required for changing)
    rasterization_info.setLineWidth(1.0f);                       // Line thickness (feature required for values above 1)
    rasterization_info.setCullMode(vk::CullModeFlagBits::eBack); // Triangle face to cull
    rasterization_info.setFrontFace(vk::FrontFace::eCounterClockwise); // Set vertex order of front-facing triangles
    // Change depth information in some manner
    rasterization_info.setDepthBiasEnable(false);
    rasterization_info.setDepthBiasConstantFactor(0.0f);
    rasterization_info.setDepthBiasClamp(0.0f);
    rasterization_info.setDepthBiasSlopeFactor(0.0f);

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.setSampleShadingEnable(true);
    multisampling_info.setRasterizationSamples(_msaa_samples);
    multisampling_info.setMinSampleShading(0.2f);
    multisampling_info.setPSampleMask(nullptr);
    multisampling_info.setAlphaToCoverageEnable(false);
    multisampling_info.setAlphaToOneEnable(false);

    // Depth and stencil testing
    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.setDepthTestEnable(true);
    depth_stencil.setDepthWriteEnable(true);
    depth_stencil.setDepthCompareOp(vk::CompareOp::eLess);
    depth_stencil.setDepthBoundsTestEnable(false);
    depth_stencil.setMinDepthBounds(0.0f);
    depth_stencil.setMaxDepthBounds(1.0f);
    depth_stencil.setStencilTestEnable(false);
    depth_stencil.setFront({});
    depth_stencil.setBack({});

    // Color blending
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    // Since blend is disabled, no blend will be preformed
    color_blend_attachment.setBlendEnable(false);
    color_blend_attachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    );
    // Color blend
    color_blend_attachment.setSrcColorBlendFactor(vk::BlendFactor::eOne);
    color_blend_attachment.setDstColorBlendFactor(vk::BlendFactor::eZero);
    color_blend_attachment.setColorBlendOp(vk::BlendOp::eAdd);
    // Alpha blend
    color_blend_attachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
    color_blend_attachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
    color_blend_attachment.setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.setLogicOpEnable(false);
    color_blend_state_info.setLogicOp(vk::LogicOp::eOr);
    color_blend_state_info.setAttachmentCount(1);
    color_blend_state_info.setPAttachments(&color_blend_attachment);

    // Pipeline layout fo UNIFORM values
    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.setSetLayoutCount(1);
    layout_info.setPSetLayouts(&_descriptor_set_layout);
    layout_info.setPushConstantRangeCount(0);
    layout_info.setPPushConstantRanges(nullptr);

    _pipeline_layout = _device->handle.createPipelineLayout(layout_info, _allocator);

    // Dynamic state
    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.setDynamicStateCount(static_cast<uint32>(dynamic_states.size()));
    dynamic_state_info.setPDynamicStates(dynamic_states.data());

    // Create pipeline object
    vk::GraphicsPipelineCreateInfo create_info{};
    // Programable pipeline stages
    create_info.setStageCount(2);
    create_info.setPStages(shader_stages);
    // Fixed-function stages
    create_info.setPVertexInputState(&vertex_input_info);
    create_info.setPInputAssemblyState(&input_assembly_info);
    create_info.setPViewportState(&viewport_state_info);
    create_info.setPRasterizationState(&rasterization_info);
    create_info.setPMultisampleState(&multisampling_info);
    create_info.setPDepthStencilState(&depth_stencil);
    create_info.setPColorBlendState(&color_blend_state_info);
    create_info.setPDynamicState(&dynamic_state_info);
    // Pipeline layout handle
    create_info.setLayout(_pipeline_layout);
    // Render passes
    create_info.setRenderPass(_render_pass);
    create_info.setSubpass(0);
    // Other
    create_info.setBasePipelineHandle(VK_NULL_HANDLE);
    create_info.setBasePipelineIndex(-1);

    auto result = _device->handle.createGraphicsPipeline(VK_NULL_HANDLE, create_info, _allocator);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create graphics pipeline.");
    _graphics_pipeline = result.value;

    // Free unused objects
    _device->handle.destroyShaderModule(vertex_shader_module, _allocator);
    _device->handle.destroyShaderModule(fragment_shader_module, _allocator);
}

std::vector<byte> read_file(const std::string& filepath) {
    std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<byte> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

vk::ShaderModule VulkanBackend::create_shader_module(const std::vector<byte>& code) {
    vk::ShaderModuleCreateInfo create_info{};
    create_info.setCodeSize(code.size());
    create_info.setPCode(reinterpret_cast<const uint32*> (code.data()));
    return _device->handle.createShaderModule(create_info, _allocator);
}