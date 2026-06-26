module;

module th.render_system.render_graph;

auto th::RenderGraph::addTextureResource(const std::string_view texture_name, RenderTarget& resource)
        -> RenderGraphResource {
    const auto res = std::ranges::find_if(m_textures, [texture_name](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, RenderGraphPersistentTarget>) {
            return value.name == texture_name;
        }
        return false;
    });
    if (res != m_textures.end()) {
        return RenderGraphResource{ .id = static_cast<uint32_t>(std::distance(m_textures.begin(), res)) };
    }
    m_resources.emplace_back(RenderGraphPersistentTarget{ .target = resource, .name = std::string(texture_name) });
    return RenderGraphResource{ .id = static_cast<uint32_t>(m_resources.size() - 1) };
}

auto th::RenderGraph::addTextureResource(std::string_view texture_name) -> RenderGraphResource {
    const auto res = std::ranges::find_if(m_textures, [texture_name](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, RenderGraphTransientTarget>) {
            return value.name == texture_name;
        }
        return false;
    });
    if (res != m_textures.end()) {
        return RenderGraphResource{ .id = static_cast<uint32_t>(std::distance(m_textures.begin(), res)) };
    }
    m_resources.emplace_back(RenderGraphTransientTarget{ .name = std::string(texture_name) });
    return RenderGraphResource{ .id = static_cast<uint32_t>(m_resources.size() - 1) };
}

void th::RenderGraph::compile() {
    for (auto& [setup, pass_name] : m_passes) {
        RenderGraphBuilder render_graph_builder;
        const auto execute_pass = setup(render_graph_builder);
        auto& [exec, dependency_tracker] = m_execute_passes.emplace_back(execute_pass, DependencyTracker{});
        for (const auto resources = render_graph_builder.getWriteDependency2();
             auto& [handle, transition] : resources) {
            auto& texture = m_resources[handle.id];
            std::visit(
                    [&dependency_tracker, transition](auto&& arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, RenderGraphPersistentTarget>) {
                            dependency_tracker.addImageBarrier(arg.target.getImageMemoryBarrier(transition));
                        }
                    },
                    texture);
        }
    }
}
void th::RenderGraph::execute(const vk::CommandBuffer command_buffer,
                              const uint32_t frame_index,
                              const std::span<const GpuStaticMesh>
                                      meshes) {
    const RenderGraphContext render_graph_context{
        .frame_index = frame_index,
        .targets = m_resources,
        .meshes = meshes,
    };
    for (auto& [exec, dependencies] : m_execute_passes) {
        dependencies.flush(command_buffer);
        exec(render_graph_context, command_buffer);
    }
}