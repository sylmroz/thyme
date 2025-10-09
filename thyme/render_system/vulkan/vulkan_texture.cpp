module;

module th.render_system.vulkan;

namespace th {

[[nodiscard]] static auto createImageSampler(const vk::Device device, const float max_sampler_anisotropy,
                                             const uint32_t mip_levels) -> vk::UniqueSampler {
    return device.createSamplerUnique(vk::SamplerCreateInfo{ .magFilter = vk::Filter::eLinear,
                                                             .minFilter = vk::Filter::eLinear,
                                                             .mipmapMode = vk::SamplerMipmapMode::eLinear,
                                                             .addressModeU = vk::SamplerAddressMode::eRepeat,
                                                             .addressModeV = vk::SamplerAddressMode::eRepeat,
                                                             .addressModeW = vk::SamplerAddressMode::eRepeat,
                                                             .mipLodBias = 0.0f,
                                                             .anisotropyEnable = vk::True,
                                                             .maxAnisotropy = max_sampler_anisotropy,
                                                             .compareEnable = vk::False,
                                                             .compareOp = vk::CompareOp::eAlways,
                                                             .minLod = 0.0f,
                                                             .maxLod = static_cast<float>(mip_levels),
                                                             .borderColor = vk::BorderColor::eIntOpaqueBlack,
                                                             .unnormalizedCoordinates = vk::False });
}

void copyImage(const vk::CommandBuffer commandBuffer, const vk::Image src_image, const vk::Extent3D resolution,
               const vk::Image dst_image) {
    const auto copyRegion = vk::ImageCopy2{
        .extent = resolution,
    };
    commandBuffer.copyImage2(vk::CopyImageInfo2{ .srcImage = src_image,
                                                 .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                                                 .dstImage = dst_image,
                                                 .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                                                 .regionCount = 1u,
                                                 .pRegions = &copyRegion });
}

void blitImage(const vk::CommandBuffer command_buffer, const vk::Image src_image, const vk::Extent3D src_resolution,
               const vk::Image dst_image, const vk::Extent3D dst_resolution) {

    const auto blitRegion = vk::ImageBlit2{
        .srcSubresource =
                vk::ImageSubresourceLayers{
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                },
        .srcOffsets =
                std::array{
                        vk::Offset3D{},
                        vk::Offset3D{ .x = static_cast<int32_t>(src_resolution.width),
                                      .y = static_cast<int32_t>(src_resolution.height),
                                      .z = static_cast<int32_t>(src_resolution.depth) },
                },
        .dstSubresource =
                vk::ImageSubresourceLayers{
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .mipLevel = 0u,
                        .baseArrayLayer = 0u,
                        .layerCount = 1u,
                },
        .dstOffsets =
                std::array{
                        vk::Offset3D{},
                        vk::Offset3D{ .x = static_cast<int32_t>(dst_resolution.width),
                                      .y = static_cast<int32_t>(dst_resolution.height),
                                      .z = static_cast<int32_t>(dst_resolution.depth) },
                },
    };

    const auto blitInfo = vk::BlitImageInfo2{
        .srcImage = src_image,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = dst_image,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1u,
        .pRegions = &blitRegion,
        .filter = vk::Filter::eLinear,
    };
    command_buffer.blitImage2(blitInfo);
}

VulkanImageMemory::VulkanImageMemory(const VulkanDevice& device, const vk::Extent3D resolution, const vk::Format format,
                                     const vk::ImageUsageFlags image_usage_flags,
                                     const vk::MemoryPropertyFlags memory_property_flags,
                                     const vk::ImageAspectFlags aspect_flags, const vk::SampleCountFlagBits msaa,
                                     const uint32_t mip_levels)
    : m_format{ format }, m_imageUsageFlags{ image_usage_flags }, m_memoryPropertyFlags{ memory_property_flags },
      m_aspectFlags{ aspect_flags }, m_msaa{ msaa }, m_mipLevels{ mip_levels }, m_device{ device } {
    resize(resolution);
}

void VulkanImageMemory::resize(const vk::Extent2D resolution) {
    resize(vk::Extent3D{ .width = resolution.width, .height = resolution.height, .depth = 1 });
}

void VulkanImageMemory::resize(const vk::Extent3D resolution) {
    if (resolution == m_extent) {
        return;
    }
    m_extent = resolution;
    const auto createInfo = vk::ImageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = m_format,
        .extent = m_extent,
        .mipLevels = m_mipLevels,
        .arrayLayers = 1,
        .samples = m_msaa,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = m_imageUsageFlags,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    m_image = m_device.logical_device.createImageUnique(createInfo);
    vk::MemoryRequirements memoryRequirements;
    m_device.logical_device.getImageMemoryRequirements(m_image.get(), &memoryRequirements);
    m_memory = m_device.logical_device.allocateMemoryUnique(vk::MemoryAllocateInfo{
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = findMemoryType(
                    m_device.physical_device, memoryRequirements.memoryTypeBits, m_memoryPropertyFlags) });
    m_device.logical_device.bindImageMemory(m_image.get(), m_memory.get(), 0);
    m_imageView = m_device.logical_device.createImageViewUnique(
            vk::ImageViewCreateInfo{ .image = m_image.get(),
                                     .viewType = vk::ImageViewType::e2D,
                                     .format = m_format,
                                     .subresourceRange = vk::ImageSubresourceRange{ .aspectMask = m_aspectFlags,
                                                                                    .baseMipLevel = 0,
                                                                                    .levelCount = m_mipLevels,
                                                                                    .baseArrayLayer = 0,
                                                                                    .layerCount = 1 } });
}
void VulkanImageMemory::transitImageLayout(ImageLayoutTransition layout_transition) {
    m_device.singleTimeCommand(
            [layout_transition, image = m_image.get(), mipLevels = m_mipLevels](const vk::CommandBuffer commandBuffer) {
                th::transitImageLayout(commandBuffer, image, layout_transition, mipLevels);
            });
}
void VulkanImageMemory::transitImageLayout(const vk::CommandBuffer command_buffer,
                                           const ImageLayoutTransition layout_transition) {
    th::transitImageLayout(command_buffer, m_image.get(), layout_transition, m_mipLevels);
}

void VulkanImageMemory::copyTo(const vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image) {
    copyImage(command_buffer, m_image.get(), m_extent, dst_image.getImage());
}

void VulkanImageMemory::copyTo(const vk::CommandBuffer command_buffer, const vk::Image image) {
    copyImage(command_buffer, m_image.get(), m_extent, image);
}

void VulkanImageMemory::blitTo(const vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image) {
    blitImage(command_buffer, m_image.get(), m_extent, dst_image.getImage(), dst_image.getExtent());
}

void VulkanImageMemory::blitTo(const vk::CommandBuffer command_buffer, const vk::Image dst_image,
                               const vk::Extent3D dst_resolution) {
    blitImage(command_buffer, m_image.get(), m_extent, dst_image, dst_resolution);
}

VulkanDepthImageMemory::VulkanDepthImageMemory(const VulkanDevice& device, const vk::Extent2D resolution,
                                               const vk::Format format, const vk::SampleCountFlagBits msaa)
    : VulkanImageMemory(device, vk::Extent3D{ .width = resolution.width, .height = resolution.height, .depth = 1 },
                        format, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                        vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth, msaa, 1) {}

VulkanColorImageMemory::VulkanColorImageMemory(const VulkanDevice& device, const vk::Extent2D resolution,
                                               const vk::Format format, const vk::SampleCountFlagBits msaa)
    : VulkanImageMemory(device, vk::Extent3D{ .width = resolution.width, .height = resolution.height, .depth = 1 },
                        format,
                        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                | vk::ImageUsageFlagBits::eColorAttachment,
                        vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor, msaa, 1) {}

Vulkan2DTexture::Vulkan2DTexture(const VulkanDevice& device, const TextureData& texture, const vk::Format format)
    : m_imageMemory{ VulkanImageMemory(
              device,
              vk::Extent3D{ .width = texture.getResolution().x, .height = texture.getResolution().y, .depth = 1 },
              format,
              vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                      | vk::ImageUsageFlagBits::eSampled,
              vk::MemoryPropertyFlagBits::eDeviceLocal,
              vk::ImageAspectFlagBits::eColor,
              vk::SampleCountFlagBits::e1,
              texture.getMipLevels()) },
      m_sampler{ createImageSampler(device.logical_device,
                                    device.physical_device.getProperties().limits.maxSamplerAnisotropy,
                                    texture.getMipLevels()) },
      m_format{ format }, m_device{ device } {
    setData(texture);
}

void Vulkan2DTexture::setData(const TextureData& texture) {
    m_extent = vk::Extent2D(texture.getResolution().x, texture.getResolution().y);
    m_imageMemory.resize(vk::Extent3D{ .width = m_extent.width, .height = m_extent.height, .depth = 1 });
    m_mipLevels = texture.getMipLevels();

    const auto data = texture.getData();
    const auto stagingMemoryBuffer =
            VulkanBufferMemory(m_device.logical_device,
                               m_device.physical_device,
                               data.size(),
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = m_device.logical_device.mapMemory(
            stagingMemoryBuffer.getMemory().get(), 0, data.size(), vk::MemoryMapFlags(), &mappedMemory);
    std::memcpy(mappedMemory, data.data(), data.size());
    m_device.logical_device.unmapMemory(stagingMemoryBuffer.getMemory().get());

    m_imageMemory.transitImageLayout(ImageLayoutTransition{
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
    });

    const auto graphicsQueue = m_device.getGraphicQueue();
    copyBufferToImage(m_device.logical_device,
                      m_device.command_pool,
                      graphicsQueue,
                      stagingMemoryBuffer.getBuffer().get(),
                      m_imageMemory.getImage(),
                      texture.getResolution());
    generateMipmaps();
}

void Vulkan2DTexture::generateMipmaps() const {
    const auto device = m_device.logical_device;
    const auto physicalDevice = m_device.physical_device;
    const auto commandPool = m_device.command_pool;
    const auto graphicsQueue = m_device.getGraphicQueue();
    if (!(physicalDevice.getFormatProperties(m_format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(device, commandPool, graphicsQueue, [&](const vk::CommandBuffer& commandBuffer) {
        const auto getImageBarrier =
                [image = m_imageMemory.getImage()](const uint32_t mipLevel) -> vk::ImageMemoryBarrier {
            return vk::ImageMemoryBarrier({ .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                                            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                                            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                                            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                            .image = image,
                                            .subresourceRange = vk::ImageSubresourceRange(
                                                    vk::ImageAspectFlagBits::eColor, mipLevel, 1, 0, 1) });
        };
        const auto getImageBlit =
                [](const uint32_t mipLevel, const int mipWidth, const int mipHeight) -> vk::ImageBlit {
            return vk::ImageBlit(
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel, 0, 1),
                    std::array{ vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ mipWidth, mipHeight, 1 } },
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel + 1, 0, 1),
                    std::array{
                            vk::Offset3D{ 0, 0, 0 },
                            vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 } });
        };
        int mipWidth = static_cast<int>(m_extent.width);
        int mipHeight = static_cast<int>(m_extent.height);
        for (uint32_t mipLevel = 0; mipLevel < m_mipLevels - 1; ++mipLevel, mipWidth /= 2, mipHeight /= 2) {
            auto barrier = getImageBarrier(mipLevel);
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eTransfer,
                                          vk::DependencyFlags(),
                                          {},
                                          {},
                                          { barrier });

            const auto blit = getImageBlit(mipLevel, mipWidth, mipHeight);
            commandBuffer.blitImage(m_imageMemory.getImage(),
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    m_imageMemory.getImage(),
                                    vk::ImageLayout::eTransferDstOptimal,
                                    { blit },
                                    vk::Filter::eLinear);
            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eFragmentShader,
                                          vk::DependencyFlags(),
                                          {},
                                          {},
                                          { barrier });
        }
        auto barrier = getImageBarrier(m_mipLevels - 1);
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eFragmentShader,
                                      vk::DependencyFlags(),
                                      {},
                                      {},
                                      { barrier });
    });
}

}// namespace th
