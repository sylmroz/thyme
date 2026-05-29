export module th.render_system.vulkan:uniform_buffer_object;

import std;
import vk_mem_alloc;
import vulkan;

import :buffer;
import :device;
#include <cassert>

export namespace th {

template <typename T>
class VulkanUniformBuffer final {
public:
    explicit VulkanUniformBuffer(const VulkanDevice& device)
        : m_uniformMemoryBuffer{ VulkanBufferMemory(device,
                                                    sizeof(T),
                                                    vk::BufferUsageFlagBits::eUniformBuffer,
                                                    vk::MemoryPropertyFlagBits::eHostVisible
                                                            | vk::MemoryPropertyFlagBits::eHostCoherent) },
          m_mappedMemoryBuffer{ m_uniformMemoryBuffer.getMemory().mapMemory(0, sizeof(T), vk::MemoryMapFlags()) }

    {}

    explicit VulkanUniformBuffer(const VulkanUniformBuffer&) = delete;
    explicit VulkanUniformBuffer(VulkanUniformBuffer&&) = default;
    auto operator=(const VulkanUniformBuffer&) -> VulkanUniformBuffer& = delete;
    auto operator=(VulkanUniformBuffer&&) -> VulkanUniformBuffer& = default;

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> vk::DescriptorBufferInfo {
        return vk::DescriptorBufferInfo(*m_uniformMemoryBuffer.getBuffer(), 0, sizeof(T));
    }

    void update(const T& obj) const noexcept {
        std::memcpy(m_mappedMemoryBuffer, &obj, sizeof(obj));
    }

    ~VulkanUniformBuffer() {
        m_uniformMemoryBuffer.unmapMemory();
    }

private:
    VulkanBufferMemory m_uniformMemoryBuffer;
    void* m_mappedMemoryBuffer{ nullptr };
};

template <typename T>
class VulkanUniformBuffer2 final {
    template <typename T>
    class UniformBuffer final {
    public:
        explicit UniformBuffer(const vma::raii::Allocator& allocator)
            : m_buffer(allocator.createBuffer(
                      vk::BufferCreateInfo{
                              .size = sizeof(T),
                              .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                              .sharingMode = vk::SharingMode::eExclusive,
                      },
                      vma::AllocationCreateInfo{ .usage = vma::MemoryUsage::eCpuToGpu })),
              m_mapped_memory_buffer(m_buffer.getAllocation().map()) {}

        void update(const T& obj) const noexcept {
            std::memcpy(m_mapped_memory_buffer, &obj, sizeof(obj));
        }

        [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> vk::DescriptorBufferInfo {
            const vk::raii::Buffer& vk_buf = m_buffer;
            return vk::DescriptorBufferInfo(*vk_buf, 0, sizeof(T));
        }

        void unmap() const {
            m_buffer.getAllocation().unmap();
        }

    private:
        vma::raii::Buffer m_buffer;
        void* m_mapped_memory_buffer{ nullptr };
    };

public:
    VulkanUniformBuffer2(const vma::raii::Allocator& allocator, std::size_t num_buffers_in_flight) {
        m_uniform_buffer_objects.reserve(num_buffers_in_flight);
        std::generate_n(std::back_inserter(m_uniform_buffer_objects), num_buffers_in_flight, [&allocator]() mutable {
            return UniformBuffer<T>(allocator);
        });
    }

    explicit VulkanUniformBuffer2(const VulkanUniformBuffer2&) = delete;
    explicit VulkanUniformBuffer2(VulkanUniformBuffer2&&) = default;
    auto operator=(const VulkanUniformBuffer2&) -> VulkanUniformBuffer2& = delete;
    auto operator=(VulkanUniformBuffer2&&) -> VulkanUniformBuffer2& = default;

    void update(const T& obj, uint32_t index) const noexcept {
        // assert(index < m_uniform_buffer_objects.size());
        m_uniform_buffer_objects[index].update(obj);
    }

    [[nodiscard]] auto getDescriptorBufferInfos() const noexcept -> std::vector<vk::DescriptorBufferInfo> {
        return m_uniform_buffer_objects | std::views::transform([](const auto& obj) -> vk::DescriptorBufferInfo {
                   return obj.getDescriptorBufferInfos();
               })
               | std::ranges::to<std::vector<vk::DescriptorBufferInfo>>();
    }

    ~VulkanUniformBuffer2() {
        for (const auto& obj : m_uniform_buffer_objects) {
            obj.unmap();
        }
    }
private:
    std::vector<UniformBuffer<T>> m_uniform_buffer_objects;
};


}// namespace th
