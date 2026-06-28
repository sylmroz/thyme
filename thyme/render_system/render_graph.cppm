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
    void read(std::string_view name) {
        m_read_textures.emplace_back(name);
    }

    void write(const std::string_view name, const ImageTransition& transition) {
        m_write_textures.emplace_back(transition, std::string(name));
    }

    auto write(RenderGraphResource resource, const ImageTransition& transition) -> RenderGraphResource {
        m_write_textures_2.emplace_back(resource, transition);
        return resource;
    }

    auto getReadDependency() -> std::span<const std::string> {
        return m_read_textures;
    }

    auto getWriteDependency() -> std::span<const RenderGraphImageResource> {
        return m_write_textures;
    }
    auto getWriteDependency2() -> std::span<const RenderGraphImageResource2> {
        return m_write_textures_2;
    }

private:
    std::vector<std::string> m_read_textures;
    std::vector<RenderGraphImageResource> m_write_textures;

    std::vector<RenderGraphImageResource2> m_write_textures_2;
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

using execute_function = std::function<void(const RenderGraphContext&, vk::CommandBuffer)>;
using setup_function = std::function<execute_function(RenderGraphBuilder&)>;

class RenderGraph {

    struct Pass {
        setup_function setup;
        std::string name;
    };

    struct ExecutePass {
        execute_function exec;
        DependencyTracker dependency_tracker;
    };

public:
    void addPass(const std::string_view pass_name, setup_function setup) {
        m_passes.emplace_back(std::move(setup), std::string(pass_name));
    }

    [[nodiscard]] auto addTextureResource(std::string_view texture_name, RenderTarget& resource) -> RenderGraphResource;

    [[nodiscard]] auto addTextureResource(std::string_view texture_name) -> RenderGraphResource;

    void compile();

    void execute(const vk::CommandBuffer command_buffer, const uint32_t frame_index,
                 const std::span<const GpuStaticMesh> meshes);

private:
    [[nodiscard]] auto getResourceIfExist(std::string_view texture_name) -> std::expected<RenderGraphResource, std::monostate>;
private:
    std::vector<Pass> m_passes;

    std::vector<ExecutePass> m_execute_passes;

    std::unordered_map<std::string, RenderTarget*> m_textures;

    std::vector<RenderGraphTarget> m_resources;
};

}// namespace th
