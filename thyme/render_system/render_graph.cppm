export module th.render_system.render_graph;

import std;
import vulkan;

import th.render_system.vulkan;

export namespace th {

struct RenderGraphResource {
    uint32_t id;
};

struct RenderGraphImageResource {
    ImageTransition transition;
    std::string name;
};

struct RenderGraphImageResource2 {
    RenderGraphResource resource;
    ImageTransition transition;
};

class RenderGraphBuilder {
public:
    void read(const RenderGraphResource resource, const ImageTransition& transition) {
        m_read_textures.emplace_back(resource, transition);
    }

    auto write(RenderGraphResource resource, const ImageTransition& transition) -> RenderGraphResource {
        m_write_textures.emplace_back(resource, transition);
        return resource;
    }

    [[nodiscard]] auto getReadDependency() const -> std::span<const RenderGraphImageResource2> {
        return m_read_textures;
    }

    [[nodiscard]] auto getWriteDependency() const -> std::span<const RenderGraphImageResource2> {
        return m_write_textures;
    }

private:
    std::vector<RenderGraphImageResource2> m_read_textures;
    std::vector<RenderGraphImageResource2> m_write_textures;
};

struct RenderGraphTextureCreateInfo {
    vk::Extent3D extent;
    vk::Format format;
    vk::ImageUsageFlags usage;
    vk::ImageCreateFlags flags;
    vk::ImageUsageFlags m_image_usage_flags;
    vk::MemoryPropertyFlags m_memory_property_flags;
    vk::ImageAspectFlags m_aspect_flags;
    vk::SampleCountFlagBits m_msaa;
    vk::ImageTiling tiling;
    uint32_t m_mip_levels{ 1 };
    std::string name;
};

struct RenderGraphTransientTarget {
    std::string name;
};

struct RenderGraphPersistentTarget {
    RenderTarget& target;
    std::string name;
};

using RenderGraphTarget = std::variant<RenderGraphPersistentTarget, RenderGraphTransientTarget>;

struct RenderGraphContext {
    uint32_t frame_index;
    std::span<const RenderGraphTarget> targets;
    std::span<const GpuStaticMesh> meshes;
};

using ExecuteFunction = std::function<void(const RenderGraphContext&, vk::CommandBuffer)>;
using setup_function = std::function<ExecuteFunction(RenderGraphBuilder&)>;

class RenderGraph {

    struct Pass {
        setup_function setup;
        std::string name;
    };

    struct ExecutePass {
        ExecuteFunction exec;
        DependencyTracker dependency_tracker;
    };

    enum struct AccessType {
        Read,
        Write
    };

    struct TouchType {
        uint32_t pass_index;
        AccessType access_type;
    };

    struct ResourceLifetime {
        uint32_t first{ reinterpret_cast<uint32_t>(std::numeric_limits<uint32_t>::max) };
        uint32_t last{ 0 };
    };

public:
    void addPass(const std::string_view pass_name, setup_function setup) {
        m_passes.emplace_back(std::move(setup), std::string(pass_name));
    }

    [[nodiscard]] auto addTextureResource(std::string_view texture_name, RenderTarget& resource) -> RenderGraphResource;

    [[nodiscard]] auto addTextureResource(std::string_view texture_name) -> RenderGraphResource;

    void compile();

    void execute(vk::CommandBuffer command_buffer, uint32_t frame_index, std::span<const GpuStaticMesh> meshes);

private:
    [[nodiscard]] auto getResourceIfExist(std::string_view texture_name)
            -> std::expected<RenderGraphResource, std::monostate>;
    void setupPasses();

    [[nodiscard]] auto buildAdjacencyList() -> std::vector<std::vector<int>>;
    [[nodiscard]] auto topologicalSort(const std::vector<std::vector<int>>& adjacency_list) -> std::vector<int>;
    [[nodiscard]] auto recordResourcesTouches() const -> std::vector<std::vector<TouchType>>;
    [[nodiscard]] auto computeResourceLifetime(const std::vector<std::vector<TouchType>>& touches_list) const
            -> std::vector<ResourceLifetime>;
    [[nodiscard]] auto cullPasses(const std::vector<std::vector<TouchType>>& touches_list) const -> std::vector<bool>;

private:
    std::vector<Pass> m_passes;
    std::vector<RenderGraphBuilder> m_setup_passes;

    std::vector<ExecutePass> m_execute_passes;

    std::unordered_map<std::string, RenderTarget*> m_textures;

    std::vector<RenderGraphTarget> m_resources;
};

}// namespace th
