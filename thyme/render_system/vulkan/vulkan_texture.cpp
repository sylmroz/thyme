module;

module th.render_system.vulkan;

namespace th {

[[nodiscard]] static auto createImageSampler(const vk::raii::Device& device, const float max_sampler_anisotropy,
                                             const uint32_t mip_levels) -> vk::raii::Sampler {
    return device.createSampler(vk::SamplerCreateInfo{ .magFilter = vk::Filter::eLinear,
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

void copyImage(const vk::CommandBuffer command_buffer, const vk::Image src_image, const vk::Extent3D src_resolution,
               const vk::Image dst_image) {
    const auto copy_region = vk::ImageCopy2{
        .extent = src_resolution,
    };
    command_buffer.copyImage2(vk::CopyImageInfo2{ .srcImage = src_image,
                                                  .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                                                  .dstImage = dst_image,
                                                  .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                                                  .regionCount = 1u,
                                                  .pRegions = &copy_region });
}

void blitImage(const vk::CommandBuffer command_buffer, const vk::Image src_image, const vk::Extent3D src_resolution,
               const vk::Image dst_image, const vk::Extent3D dst_resolution) {

    const auto blit_region = vk::ImageBlit2{
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

    const auto blit_info = vk::BlitImageInfo2{
        .srcImage = src_image,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = dst_image,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1u,
        .pRegions = &blit_region,
        .filter = vk::Filter::eLinear,
    };
    command_buffer.blitImage2(blit_info);
}
VulkanImageMemoryCreator::VulkanImageMemoryCreator(const vk::Format format, const vk::ImageUsageFlags image_usage_flags,
                                                   const vk::MemoryPropertyFlags memory_property_flags,
                                                   const vk::ImageAspectFlags aspect_flags,
                                                   const vk::SampleCountFlagBits msaa, const uint32_t mip_levels)
    : m_format{ format }, m_image_usage_flags{ image_usage_flags }, m_memory_property_flags{ memory_property_flags },
      m_aspect_flags{ aspect_flags }, m_msaa{ msaa }, m_mip_levels{ mip_levels } {}

auto VulkanImageMemoryCreator::create(const VulkanDeviceRAII& device, const vk::Extent3D resolution) const
        -> ImageMemoryImageView {
    const auto createInfo = vk::ImageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = m_format,
        .extent = resolution,
        .mipLevels = m_mip_levels,
        .arrayLayers = 1,
        .samples = m_msaa,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = m_image_usage_flags,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    auto image = device.logical_device.createImage(createInfo);
    const vk::MemoryRequirements memory_requirements = image.getMemoryRequirements();
    auto memory = device.logical_device.allocateMemory(vk::MemoryAllocateInfo{
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = findMemoryType(
                    device.physical_device, memory_requirements.memoryTypeBits, m_memory_property_flags) });
    image.bindMemory(memory, 0);
    auto image_view = device.logical_device.createImageView(
            vk::ImageViewCreateInfo{ .image = image,
                                     .viewType = vk::ImageViewType::e2D,
                                     .format = m_format,
                                     .subresourceRange = vk::ImageSubresourceRange{ .aspectMask = m_aspect_flags,
                                                                                    .baseMipLevel = 0,
                                                                                    .levelCount = m_mip_levels,
                                                                                    .baseArrayLayer = 0,
                                                                                    .layerCount = 1 } });
    return ImageMemoryImageView{ .image = std::move(image),
                                 .memory = std::move(memory),
                                 .image_view = std::move(image_view) };
}

VulkanImageMemory::VulkanImageMemory(const VulkanDeviceRAII& device, const vk::Extent3D resolution,
                                     VulkanImageMemoryCreator memory_creator, const ImageTransition& image_transition)
    : m_image_memory_image_view(memory_creator.create(device, resolution)), m_extent(resolution),
      m_image_memory_creator(std::move(memory_creator)),
      m_image_layout_transition(m_image_memory_image_view.image, m_image_memory_creator.getImageAspectFlags(),
                                m_image_memory_creator.getMipLevels(), image_transition),
      m_initial_state{ image_transition } {
    resize(device, resolution);
}

void VulkanImageMemory::resize(const VulkanDeviceRAII& device, const vk::Extent2D resolution) {
    resize(device, vk::Extent3D{ .width = resolution.width, .height = resolution.height, .depth = 1 });
}

void VulkanImageMemory::resize(const VulkanDeviceRAII& device, const vk::Extent3D resolution) {
    if (resolution == m_extent) {
        return;
    }
    m_extent = resolution;
    m_image_memory_image_view = m_image_memory_creator.create(device, m_extent);
    m_image_layout_transition = ImageLayoutTransitionState(m_image_memory_image_view.image,
                                                           m_image_memory_creator.getImageAspectFlags(),
                                                           m_image_memory_creator.getMipLevels(),
                                                           m_initial_state);
}

void VulkanImageMemory::transitImageLayout(const VulkanDeviceRAII& device,
                                           ImageLayoutTransition layout_transition) const {
    device.singleTimeCommand(
            [layout_transition,
             image = *m_image_memory_image_view.image,
             mipLevels = m_image_memory_creator.getMipLevels(),
             usage_flags = m_image_memory_creator.getImageAspectFlags()](const vk::CommandBuffer command_buffer) {
                th::transitImageLayout(command_buffer, image, layout_transition, mipLevels, usage_flags);
            });
}

void VulkanImageMemory::transitImageLayout(const vk::CommandBuffer command_buffer,
                                           const ImageLayoutTransition layout_transition) const {
    th::transitImageLayout(command_buffer,
                           m_image_memory_image_view.image,
                           layout_transition,
                           m_image_memory_creator.getMipLevels(),
                           m_image_memory_creator.getImageAspectFlags());
}
void VulkanImageMemory::transitImageLayout(const VulkanDeviceRAII& device, const ImageTransition& transition) {
    device.singleTimeCommand([&](const vk::CommandBuffer command_buffer) {
        m_image_layout_transition.transitTo(command_buffer, transition);
    });
}

void VulkanImageMemory::transitImageLayout(const vk::CommandBuffer command_buffer, const ImageTransition& transition) {
    m_image_layout_transition.transitTo(command_buffer, transition);
}

void VulkanImageMemory::copyTo(const vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image) const {
    copyImage(command_buffer, m_image_memory_image_view.image, m_extent, dst_image.getImage());
}

void VulkanImageMemory::copyTo(const vk::CommandBuffer command_buffer, const vk::Image image) const {
    copyImage(command_buffer, m_image_memory_image_view.image, m_extent, image);
}

void VulkanImageMemory::blitTo(const vk::CommandBuffer command_buffer, const VulkanImageMemory& dst_image) const {
    blitImage(command_buffer, m_image_memory_image_view.image, m_extent, dst_image.getImage(), dst_image.getExtent());
}

void VulkanImageMemory::blitTo(const vk::CommandBuffer command_buffer, const vk::Image dst_image,
                               const vk::Extent3D dst_resolution) const {
    blitImage(command_buffer, m_image_memory_image_view.image, m_extent, dst_image, dst_resolution);
}

VulkanDepthImageMemory::VulkanDepthImageMemory(const VulkanDeviceRAII& device, const vk::Extent2D resolution,
                                               const vk::Format format, const vk::SampleCountFlagBits msaa)
    : VulkanImageMemory(device, vk::Extent3D{ .width = resolution.width, .height = resolution.height, .depth = 1 },
                        VulkanImageMemoryCreator(format, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                 vk::ImageAspectFlagBits::eDepth, msaa, 1),
                        ImageTransition{ .pipeline_stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                                         .access_flag_bits = vk::AccessFlagBits2::eNone }) {}

VulkanColorImageMemory::VulkanColorImageMemory(const VulkanDeviceRAII& device, const vk::Extent2D resolution,
                                               const vk::Format format, const vk::SampleCountFlagBits msaa)
    : VulkanImageMemory(device, vk::Extent3D{ .width = resolution.width, .height = resolution.height, .depth = 1 },
                        VulkanImageMemoryCreator(
                                format,
                                vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                        | vk::ImageUsageFlagBits::eColorAttachment,
                                vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor, msaa, 1),
                        ImageTransition{ .pipeline_stage = vk::PipelineStageFlagBits2::eAllCommands }) {}

Vulkan2DTexture::Vulkan2DTexture(const VulkanDeviceRAII& device, const TextureData& texture, const vk::Format format)
    : m_imageMemory{ VulkanImageMemory(
              device,
              vk::Extent3D{ .width = texture.getResolution().x, .height = texture.getResolution().y, .depth = 1 },
              VulkanImageMemoryCreator(format,
                                       vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                               | vk::ImageUsageFlagBits::eSampled,
                                       vk::MemoryPropertyFlagBits::eDeviceLocal,
                                       vk::ImageAspectFlagBits::eColor,
                                       vk::SampleCountFlagBits::e1,
                                       texture.getMipLevels()),
              ImageTransition{ .pipeline_stage = vk::PipelineStageFlagBits2::eTopOfPipe }) },
      m_sampler{ createImageSampler(device.logical_device,
                                    device.physical_device.getProperties().limits.maxSamplerAnisotropy,
                                    texture.getMipLevels()) },
      m_format{ format } {
    setData(device, texture);
}

void Vulkan2DTexture::setData(const VulkanDeviceRAII& device, const TextureData& texture) {
    m_extent = vk::Extent2D(texture.getResolution().x, texture.getResolution().y);
    m_imageMemory.resize(device, vk::Extent3D{ .width = m_extent.width, .height = m_extent.height, .depth = 1 });
    m_mipLevels = texture.getMipLevels();

    const auto data = texture.getData();
    const auto stagingMemoryBuffer =
            VulkanBufferMemory(device.logical_device,
                               device.physical_device,
                               data.size(),
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = stagingMemoryBuffer.getMemory().mapMemory(0, data.size(), vk::MemoryMapFlags());
    std::memcpy(mappedMemory, data.data(), data.size());
    stagingMemoryBuffer.getMemory().unmapMemory();

    m_imageMemory.transitImageLayout(device,
                                     ImageLayoutTransition{
                                             .oldLayout = vk::ImageLayout::eUndefined,
                                             .newLayout = vk::ImageLayout::eTransferDstOptimal,
                                     });

    /*m_imageMemory.transitImageLayout(device,
                                     ImageTransition{ .layout = vk::ImageLayout::eTransferDstOptimal,
                                                      .pipeline_stage = vk::PipelineStageFlagBits2::eTransfer,
                                                      .access_flag_bits = vk::AccessFlagBits2::eTransferWrite });*/

    const auto graphicsQueue = device.getGraphicQueue();
    copyBufferToImage(device.logical_device,
                      device.command_pool,
                      graphicsQueue,
                      stagingMemoryBuffer.getBuffer(),
                      m_imageMemory.getImage(),
                      texture.getResolution());
    generateMipmaps(device);
}

void Vulkan2DTexture::generateMipmaps(const VulkanDeviceRAII& device) const {
    const auto& logical_device = device.logical_device;
    const auto physicalDevice = device.physical_device;
    const auto& commandPool = device.command_pool;
    const auto graphicsQueue = device.getGraphicQueue();
    if (!(physicalDevice.getFormatProperties(m_format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(logical_device, commandPool, graphicsQueue, [&](const vk::CommandBuffer& commandBuffer) {
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
