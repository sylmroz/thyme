export module th.render_system.vulkan:buffer;

import std;

import vulkan;
import vk_mem_alloc;

import :device;
import :utils;

namespace th {

[[nodiscard]] auto createVertexBuffer(const vma::raii::Allocator& allocator, const uint32_t size) -> vma::raii::Buffer {
    return allocator.createBuffer(
            vk::BufferCreateInfo{
                    .size = size,
                    .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer
                             | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                    .sharingMode = vk::SharingMode::eExclusive,
            },
            vma::AllocationCreateInfo{ .flags = vma::AllocationCreateFlagBits::eMapped,
                                       .usage = vma::MemoryUsage::eGpuOnly });
}

export [[nodiscard]] auto createIndexBuffer(const vma::raii::Allocator& allocator, const size_t size)
        -> vma::raii::Buffer {
    return allocator.createBuffer(
            vk::BufferCreateInfo{
                    .size = size,
                    .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
            },
            vma::AllocationCreateInfo{ .flags = vma::AllocationCreateFlagBits::eMapped,
                                       .usage = vma::MemoryUsage::eGpuOnly });
}

export [[nodiscard]] auto createStagingBuffer(const vma::raii::Allocator& allocator, const size_t size)
        -> vma::raii::Buffer {
    return allocator.createBuffer(
            vk::BufferCreateInfo{
                    .size = size,
                    .usage = vk::BufferUsageFlagBits::eTransferSrc,
                    .sharingMode = vk::SharingMode::eExclusive,
            },
            vma::AllocationCreateInfo{ .usage = vma::MemoryUsage::eCpuOnly });
}

}// namespace th
