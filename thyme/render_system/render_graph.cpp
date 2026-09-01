module;

module th.render_system.render_graph;

auto th::RenderGraph::addTextureResource(const std::string_view texture_name, RenderTarget& resource)
        -> RenderGraphResource {
    return getResourceIfExist(texture_name)
            .or_else([&](auto) -> std::expected<RenderGraphResource, std::monostate> {
                m_resources.emplace_back(
                        RenderGraphPersistentTarget{ .target = resource, .name = std::string(texture_name) });
                return RenderGraphResource{ .id = static_cast<uint32_t>(m_resources.size() - 1) };
            })
            .value();
}

auto th::RenderGraph::addTextureResource(const std::string_view texture_name) -> RenderGraphResource {
    return getResourceIfExist(texture_name)
            .or_else([&](auto) -> std::expected<RenderGraphResource, std::monostate> {
                m_resources.emplace_back(RenderGraphTransientTarget{ .name = std::string(texture_name) });
                return RenderGraphResource{ .id = static_cast<uint32_t>(m_resources.size() - 1) };
            })
            .value();
}

void th::RenderGraph::compile() {

    // first lets make setup functions and build
    setupPasses();
    const auto adjacency_list = buildAdjacencyList();

    for ([[maybe_unused]] auto& [setup, pass_name] : m_passes) {
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

auto extractName(auto& value) {
    return std::visit(
            [](auto&& texture) {
                return texture.name;
            },
            value);
}

auto th::RenderGraph::getResourceIfExist(std::string_view texture_name)
        -> std::expected<RenderGraphResource, std::monostate> {
    const auto res = std::ranges::find_if(m_resources, [texture_name](auto&& value) {
        return extractName(value) == texture_name;
    });
    if (res != m_resources.end()) {
        return RenderGraphResource{ .id = static_cast<uint32_t>(std::distance(m_resources.begin(), res)) };
    }
    return std::unexpected(std::monostate{});
}

void th::RenderGraph::setupPasses() {
    for ([[maybe_unused]] auto& [setup, pass_name] : m_passes) {
        RenderGraphBuilder render_graph_builder;
        const auto execute_pass = setup(render_graph_builder);
        m_setup_passes.emplace_back(std::move(execute_pass), render_graph_builder);
    }
}

auto th::RenderGraph::buildAdjacencyList() -> std::vector<std::vector<int>> {
    const auto pick_rgb = [](auto&& arg) -> RenderGraphBuilder& {
        return arg.render_graph_builder;
    };
    std::vector<std::vector<int>> adjacency_list;
    adjacency_list.reserve(m_passes.size());
    for (int node_id{ 0 }; auto& node : m_setup_passes | std::views::transform(pick_rgb)) {
        auto& adjacency_node_dependencies = adjacency_list.emplace_back();
        const auto written_resources = node.getWriteDependency2();
        for (int other_node_id{ 0 }; auto& other_node : m_setup_passes | std::views::transform(pick_rgb)) {
            if (node_id == other_node_id) {
                ++other_node_id;
                continue;
            }
            for (const auto& read_dependency : other_node.getReadDependency2()) {
                const auto read_id = read_dependency.resource.id;
                if (auto it = std::ranges::find_if(written_resources,
                                                   [read_id](const auto& resource) {
                                                       return resource.resource.id == read_id;
                                                   });
                    it != written_resources.end()) {
                    adjacency_node_dependencies.push_back(other_node_id);
                }
            }
            ++other_node_id;
        }
        ++node_id;
    }
    return adjacency_list;
}
