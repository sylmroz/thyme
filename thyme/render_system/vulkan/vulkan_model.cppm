export module th.render_system.vulkan:model;

import std;

import glm;
import vulkan;
import vk_mem_alloc;

import th.scene.model;

import :buffer;
import :device;
import :uniform_buffer_object;
import :texture;


namespace th {

export class GpuStaticMesh {
public:
    static [[nodiscard]] auto create(const vma::raii::Allocator& allocator, vk::Device device,
                                     vk::CommandPool command_pool, vk::Queue graphic_queue,
                                     std::span<const uint32_t> indices, std::span<const Vertex> vertices)
            -> GpuStaticMesh;

    vma::raii::Buffer vertex_buffer{ nullptr };
    vma::raii::Buffer index_buffer{ nullptr };
    vk::DeviceAddress address{};
    std::size_t indices_size{};
};

}// namespace th
