module;

module th.render_system.vulkan;

import :utils;

import th.scene.model;

namespace th {

VulkanModel::VulkanModel(const Model& model, const VulkanDevice& device)
    : m_vertex_memory_buffer{ VulkanBufferMemory(device, model.mesh.vertices, vk::BufferUsageFlagBits::eVertexBuffer) },
      m_index_memory_buffer{ VulkanBufferMemory(device, model.mesh.indices, vk::BufferUsageFlagBits::eIndexBuffer) },
      m_uniform_buffer_object{ device }, m_texture{ device, model.texture },
      m_indices_size{ static_cast<uint32_t>(model.mesh.indices.size()) } {}

void VulkanModel::draw(const vk::CommandBuffer command_buffer) const noexcept {
    // command_buffer.bindVertexBuffers(0, { *m_vertex_memory_buffer.getBuffer() }, { 0 });
    command_buffer.bindIndexBuffer(*m_index_memory_buffer.getBuffer(), 0, vk::IndexType::eUint32);
    command_buffer.drawIndexed(m_indices_size, 1, 0, 0, 0);
}

auto GpuStaticMesh::create(const vma::raii::Allocator& allocator, const vk::Device device,
                           const vk::CommandPool command_pool, const vk::Queue graphic_queue,
                           const std::span<const uint32_t> indices, const std::span<const Vertex> vertices)
        -> GpuStaticMesh {
    const auto vertex_buffer_size = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);
    auto vertex_buffer = createVertexBuffer(allocator, vertex_buffer_size);

    const auto indices_buffer_size = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);
    auto index_buffer = createIndexBuffer(allocator, indices_buffer_size);

    const auto staging_buffer = createStagingBuffer(allocator, vertex_buffer_size + indices_buffer_size);
    const auto data = staging_buffer.getAllocation().map();
    std::memcpy(data, vertices.data(), vertex_buffer_size);
    std::memcpy(static_cast<char*>(data) + vertex_buffer_size, indices.data(), indices_buffer_size);
    staging_buffer.getAllocation().unmap();

    singleTimeCommand(device, command_pool, graphic_queue, [&](const vk::CommandBuffer command) {
        const auto vertex_region = vk::BufferCopy2{ .srcOffset = 0, .dstOffset = 0, .size = vertex_buffer_size };
        command.copyBuffer2(vk::CopyBufferInfo2{
                .srcBuffer = *staging_buffer,
                .dstBuffer = vertex_buffer,
                .regionCount = 1,
                .pRegions = &vertex_region,
        });

        const auto indices_region =
                vk::BufferCopy2{ .srcOffset = vertex_buffer_size, .dstOffset = 0, .size = indices_buffer_size };
        command.copyBuffer2(vk::CopyBufferInfo2{
                .srcBuffer = *staging_buffer,
                .dstBuffer = index_buffer,
                .regionCount = 1,
                .pRegions = &indices_region,
        });
    });

    const auto vertex_buffer_address = device.getBufferAddress(vk::BufferDeviceAddressInfo{ .buffer = *vertex_buffer });

    return { .vertex_buffer = std::move(vertex_buffer),
             .index_buffer = std::move(index_buffer),
             .address = vertex_buffer_address,
             .indices_size = static_cast<uint32_t>(indices.size()) };
}

}// namespace th