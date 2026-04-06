export module th.render_system.vulkan:renderer;

import std;

import vulkan;

import th.scene.camera;
import th.scene.model;
import th.core.logger;

import :buffer;
import :command_buffers;
import :device;
import :graphic_pipeline;
import :graphic_context;
import :swapchain;
import :model;
import :texture;
import :uniform_buffer_object;
import :utils;
import :gui;

export namespace th {

class VulkanRenderer {
public:
    explicit VulkanRenderer(const VulkanDevice& device, VulkanSwapchain& swapchain, ModelStorage& model_storage,
                            Camera& camera, Gui& gui, const VulkanGraphicContext& context,
                            VulkanCommandBuffersPool& command_buffers_pool, Logger& logger) noexcept;

    void draw(const VulkanDevice& device);

private:
    Gui& m_gui;
    VulkanSwapchain& m_swapchain;
    VulkanCommandBuffersPool& m_command_buffers_pool;
    std::vector<std::unique_ptr<VulkanGraphicPipeline>> m_pipelines;
    std::unique_ptr<GradientPipeline> m_gradient_pipeline;

    VulkanDepthImageMemory m_depth_image_memory;
    VulkanColorImageMemory m_color_image_memory;
    VulkanColorImageMemory m_resolve_color_image_memory;

    std::vector<VulkanModel> m_models;
    std::reference_wrapper<Camera> m_camera;

    std::reference_wrapper<ModelStorage> m_model_storage;
    VulkanUniformBuffer<CameraMatrices> m_camera_matrices;
};

class MyRenderPass {
public:
    MyRenderPass();
    void execute(vk::CommandBuffer command_buffer);
    void setTargetSize();

private:
    VulkanDepthImageMemory m_depth_image_memory;
    VulkanColorImageMemory m_color_image_memory;
    VulkanColorImageMemory m_resolve_color_image_memory;

    VulkanScenePipeline m_scene_pipeline;
};

export class VulkanRenderer2 {
public:
    explicit VulkanRenderer2(const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device);

private:
};

struct RenderGraphImageResource {
    vk::Image image;
    vk::ImageView view;
    vk::Extent2D size;
    vk::ImageUsageFlagBits usage;
    vk::AccessFlagBits2 access;
    vk::ImageLayout initial_layout;
    vk::ImageLayout final_layout;
};

struct RenderGraphImageResource2 {
    ImageTransition transition;
    std::string name;
};

class RenderGraphBuilder {
public:
    void read(std::string_view name) {
        m_read_textures.emplace_back(name);
    }

    void write(const std::string_view name, const ImageTransition& transition) {
        m_write_textures.emplace_back(transition, std::string(name));
    }

    auto getReadDependency() -> std::span<const std::string> {
        return m_read_textures;
    }

    auto getWriteDependency() -> std::span<const RenderGraphImageResource2> {
        return m_write_textures;
    }

private:
    std::vector<std::string> m_read_textures;
    std::vector<RenderGraphImageResource2> m_write_textures;
};

struct RenderGraphContext {};

class RenderGraph {

    using execute_function = std::function<void(RenderGraphContext&, vk::CommandBuffer)>;
    using setup_function = std::function<execute_function(RenderGraphBuilder&)>;

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

    void addTextureResource(const std::string_view texture_name, RenderTarget* resource) {
        m_textures.emplace(texture_name, resource);
    }

    void compile() {
        for (auto& [setup, pass_name] : m_passes) {
            RenderGraphBuilder render_graph_builder;
            const auto execute_pass = setup(render_graph_builder);
            auto& [exec, dependency_tracker] = m_execute_passes.emplace_back(execute_pass, DependencyTracker{});
            for (const auto resources = render_graph_builder.getWriteDependency();
                 auto [transition, name] : resources) {
                const auto texture = m_textures[name];
                dependency_tracker.addImageBarrier(texture->getImageMemoryBarrier(transition));
            }
        }
    };

    void execute(const vk::CommandBuffer command_buffer) {
        RenderGraphContext render_graph_context;
        for (auto& [exec, dependencies] : m_execute_passes) {
            dependencies.flush(command_buffer);
            exec(render_graph_context, command_buffer);
        }
    }

private:
    std::vector<Pass> m_passes;

    std::vector<ExecutePass> m_execute_passes;

    std::unordered_map<std::string, RenderTarget*> m_textures;
};

}// namespace th
