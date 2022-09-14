#include "renderer/vulkan/shaders/vulkan_shader.hpp"

VulkanShader::VulkanShader(VulkanDevice* device, vk::AllocationCallbacks* allocator)
    : _device(device), _allocator(allocator) {}
VulkanShader::~VulkanShader() {}

vk::ShaderModule VulkanShader::create_shader_module(std::vector<byte> code) {
    vk::ShaderModuleCreateInfo create_info{};
    create_info.setCodeSize(code.size());
    create_info.setPCode(reinterpret_cast<const uint32*> (code.data()));
    return _device->handle.createShaderModule(create_info, _allocator);
}

void VulkanShader::create_pipeline(
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages,
    std::vector<vk::VertexInputBindingDescription> binding_descriptions,
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions,
    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
    vk::RenderPass render_pass,
    vk::SampleCountFlagBits number_of_msaa_samples,
    bool is_wire_frame
) {
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
    rasterization_info.setPolygonMode(
        is_wire_frame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill
    );   // Determines how fragments are generated for geometry (feature required for changing)
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
    multisampling_info.setRasterizationSamples(number_of_msaa_samples);
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
    color_blend_attachment.setBlendEnable(true);
    color_blend_attachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    );
    // Color blend
    color_blend_attachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
    color_blend_attachment.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
    color_blend_attachment.setColorBlendOp(vk::BlendOp::eAdd);
    // Alpha blend
    color_blend_attachment.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha);
    color_blend_attachment.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
    color_blend_attachment.setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.setLogicOpEnable(false);
    color_blend_state_info.setLogicOp(vk::LogicOp::eCopy);
    color_blend_state_info.setAttachmentCount(1);
    color_blend_state_info.setPAttachments(&color_blend_attachment);

    // Dynamic state
    std::array<vk::DynamicState, 3> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eLineWidth
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.setDynamicStates(dynamic_states);

    // Vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.setVertexBindingDescriptionCount(1);
    vertex_input_info.setVertexBindingDescriptions(binding_descriptions);
    vertex_input_info.setVertexAttributeDescriptions(attribute_descriptions);

    // Pipeline layout for UNIFORM values
    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.setSetLayouts(descriptor_set_layouts);
    layout_info.setPushConstantRangeCount(0);
    layout_info.setPPushConstantRanges(nullptr);

    pipeline_layout = _device->handle.createPipelineLayout(layout_info, _allocator);

    // Create pipeline object
    vk::GraphicsPipelineCreateInfo create_info{};
    // Programable pipeline stages
    create_info.setStages(shader_stages);
    // Fixed-function stages
    create_info.setPVertexInputState(&vertex_input_info);
    create_info.setPInputAssemblyState(&input_assembly_info);
    create_info.setPViewportState(&viewport_state_info);
    create_info.setPRasterizationState(&rasterization_info);
    create_info.setPMultisampleState(&multisampling_info);
    create_info.setPDepthStencilState(&depth_stencil);
    create_info.setPColorBlendState(&color_blend_state_info);
    create_info.setPDynamicState(&dynamic_state_info);
    create_info.setPTessellationState(nullptr);
    // Pipeline layout handle
    create_info.setLayout(pipeline_layout);
    // Render passes
    create_info.setRenderPass(render_pass);
    create_info.setSubpass(0);
    // Other
    create_info.setBasePipelineHandle(VK_NULL_HANDLE);
    create_info.setBasePipelineIndex(-1);

    auto result = _device->handle.createGraphicsPipeline(VK_NULL_HANDLE, create_info, _allocator);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create graphics pipeline.");
    pipeline = result.value;
}