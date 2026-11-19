module;

module th.render_system.passes;

namespace th {

namespace {
    struct Info {
        vk::Image image;
        vk::Extent3D resolution;
    };
}// namespace
static auto returnTargetInfo(const RenderGraphTarget& target) -> Info {
    return std::visit(Overload{ [](const RenderGraphPersistentTarget& persistent_target) {
                                   const auto [width, height] = persistent_target.target.getResolution();
                                   return Info{ .image = persistent_target.target.getImage(),
                                                .resolution = vk::Extent3D(width, height, 1) };
                               },
                                [](const RenderGraphTransientTarget& transient_target) {
                                    // should call kind of texture manager to create texture on demand, if you need
                                    return Info{
                                        .image = transient_target.image,
                                    };
                                } },
                      target);
};

void copyTextureToTexturePass(std::string name, RenderGraph& render_graph, RenderGraphResource from,
                              RenderGraphResource to) {
    render_graph.addPass(std::move(name), [from, to](RenderGraphBuilder& builder) -> ExecuteFunction {
        builder.read(from,
                     ImageTransition{ .layout = vk::ImageLayout::eTransferSrcOptimal,
                                      .pipeline_stage = vk::PipelineStageFlagBits2::eAllCommands,
                                      .access_flag_bits = vk::AccessFlagBits2::eTransferRead });
        builder.write(to,
                      ImageTransition{
                              .layout = vk::ImageLayout::eTransferDstOptimal,
                              .pipeline_stage = vk::PipelineStageFlagBits2::eAllCommands,
                              .access_flag_bits = vk::AccessFlagBits2::eTransferWrite,
                      });
        return [=](const RenderGraphContext& context, const vk::CommandBuffer command_buffer) -> void
        {
            const auto [from_image, from_resolution] = returnTargetInfo(context.targets[from.id]);
            const auto [to_image, to_resolution] = returnTargetInfo(context.targets[to.id]);
            if (from_resolution.width != to_resolution.width || from_resolution.height != to_resolution.height) {
                throw std::runtime_error(std::format("Cannot copy image if resolutions are different! Src: {}, Dst: {}", from_resolution, to_resolution));
            }
            copyImage(command_buffer, from_image, from_resolution, to_image);
        };
    });
}
}// namespace th