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
    const auto touches = recordResourcesTouches();
    const auto resource_lifetime = computeResourceLifetime(touches);

    const auto sorted = topologicalSort(adjacency_list);

    for (const auto index : sorted) {

        auto& render_graph_builder = m_setup_passes[index];
        auto& dependency_tracker = m_execute_passes[index].dependency_tracker;

        for (const auto resources = render_graph_builder.getWriteDependency(); auto& [handle, transition] : resources) {
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
        for (const auto resources = render_graph_builder.getReadDependency(); auto& [handle, transition] : resources) {
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
        auto& render_graph_builder = m_setup_passes.emplace_back();
        m_execute_passes.emplace_back(setup(render_graph_builder), DependencyTracker{});
    }
}

auto th::RenderGraph::buildAdjacencyList() -> std::vector<std::vector<int>> {
    std::vector<std::vector<int>> adjacency_list;
    adjacency_list.reserve(m_passes.size());
    for (int node_id{ 0 }; auto& node : m_setup_passes) {
        auto& adjacency_node_dependencies = adjacency_list.emplace_back();
        const auto written_resources = node.getWriteDependency();
        for (int other_node_id{ 0 }; auto& other_node : m_setup_passes) {
            if (node_id == other_node_id) {
                ++other_node_id;
                continue;
            }
            for (const auto& read_dependency : other_node.getReadDependency()) {
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

auto th::RenderGraph::topologicalSort(const std::vector<std::vector<int>>& adjacency_list) -> std::vector<int> {
    std::vector<int> indegree(adjacency_list.size(), 0);
    for (const auto adj : adjacency_list) {
        for (const auto next : adj) {
            ++indegree[next];
        }
    }

    std::queue<int> q;
    for (int i{ 0 }; i < indegree.size(); i++) {
        if (indegree[i] == 0) {
            q.push(i);
        }
    }

    std::vector<int> list;
    while (!q.empty()) {
        const auto top = q.front();
        q.pop();
        list.push_back(top);
        for (const auto next : adjacency_list[top]) {
            --indegree[next];
            if (indegree[next] == 0) {
                q.push(next);
            }
        }
    }
    return list;
}

auto th::RenderGraph::recordResourcesTouches() const -> std::vector<std::vector<TouchType>> {
    std::vector<std::vector<TouchType>> touchList;
    const auto touch = [&touchList](const uint32_t pass_index, const uint32_t rid, const AccessType access_type) {
        if (rid >= touchList.size()) {
            touchList.resize(rid + 1);
        }
        touchList[rid].emplace_back(pass_index, access_type);
    };
    for (uint32_t pass_id{ 0 }; const auto& builder : m_setup_passes) {
        for (const auto resource : builder.getReadDependency()) {
            touch(pass_id, resource.resource.id, AccessType::Read);
        }
        for (const auto resource : builder.getWriteDependency()) {
            touch(pass_id, resource.resource.id, AccessType::Write);
        }
        ++pass_id;
    }
    return touchList;
}

auto th::RenderGraph::computeResourceLifetime(const std::vector<std::vector<TouchType>>& touches_list) const
        -> std::vector<ResourceLifetime> {
    std::vector<ResourceLifetime> resource_lifetimes(m_resources.size());
    for (uint32_t r{ 0 }; r < m_resources.size(); ++r) {
        const auto& touches = touches_list[r];
        if (touches.empty()) {
            continue;
        }
        resource_lifetimes[r] =
                ResourceLifetime{ .first = std::ranges::min(touches, {}, &TouchType::pass_index).pass_index,
                                  .last = std::ranges::max(touches, {}, &TouchType::pass_index).pass_index };
    }

    return resource_lifetimes;
}

/*auto th::RenderGraph::cullPasses(const std::vector<std::vector<TouchType>>& touches_list) const -> std::vector<bool> {
    std::vector needed(m_setup_passes.size(), false);
    std::vector<std::unordered_set<uint32_t>> dependencies(m_setup_passes.size());
    std::queue<uint32_t> q;

    for (auto touches : touches_list) {
        if (touches.size() < 2) {
            continue;
        }
        std::ranges::sort(touches, [](const TouchType& a, const TouchType& b) {
            return a.pass_index < b.pass_index;
        });
        auto last_writer{ -1 };
        for (const auto [pass_idx, access] : touches) {
            if (access == AccessType::Read && last_writer != -1 && last_writer != pass_idx) {
                dependencies[pass_idx].insert(last_writer);
            }
            if (access == AccessType::Write) {
                last_writer = pass_idx;
            }
        }
    }

    return needed;
}*/
