export module th.render_system.renderer;

import std;
import vk_mem_alloc;
import vulkan;

import th.render_system.vulkan;
import th.core.logger;
import th.render_system.render_graph;
import th.render_system.vulkan;

namespace th {

export template <typename T>
class UniformBuffer;

export class Renderer {
public:
    Renderer(const vk::raii::Device& device, std::uint32_t graphic_queue_index, std::uint32_t max_frames_in_flight,
             Logger& logger);

    [[nodiscard]] auto getCurrentFrameIndex() const noexcept -> uint32_t {
        return m_command_buffers_pool.currentIndex();
    }

    [[nodiscard]] auto getFramesInFlightCount() const noexcept -> uint32_t {
        return static_cast<uint32_t>(m_command_buffers_pool.size());
    }

    template <typename T>
    [[nodiscard]] auto createUniformBuffer(const vma::raii::Allocator& allocator) -> UniformBuffer<T>;

    void beginFrame(const vk::raii::Device& device, vk::Semaphore frame_semaphore);
    void draw(const vk::raii::Device& device, RenderGraph& render_graph);
    void endFrame(vk::Semaphore frame_render_semaphore);

    void addMesh(GpuStaticMesh&& mesh) {
        m_meshes.push_back(std::forward<GpuStaticMesh>(mesh));
    }


    vk::raii::CommandPool m_command_pool;
private:
    vk::raii::Queue m_queue;
    VulkanCommandBuffersPool2 m_command_buffers_pool;

    std::vector<GpuStaticMesh> m_meshes;
};

template <typename T>
class UniformBuffer {
public:
    UniformBuffer(Renderer& renderer, const vma::raii::Allocator& allocator)
        : m_renderer(renderer), m_uniform_buffer_array(allocator, renderer.getFramesInFlightCount()) {}

    void update(const T& value) {
        m_uniform_buffer_array.update(value, m_renderer.getCurrentFrameIndex());
    }

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> std::vector<vk::DescriptorBufferInfo> {
        return m_uniform_buffer_array.getDescriptorBufferInfos();
    }

private:
    Renderer& m_renderer;
    VulkanUniformBuffer2<T> m_uniform_buffer_array;
};

template <typename T>
auto Renderer::createUniformBuffer(const vma::raii::Allocator& allocator) -> UniformBuffer<T> {
    return UniformBuffer<T>{*this, allocator};
}

}// namespace th
