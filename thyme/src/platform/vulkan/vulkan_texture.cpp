#include <thyme/platform/vulkan/vulkan_texture.hpp>

namespace th::vulkan {

static void generateMipmaps(const Device& device, vk::Image image, vk::Format format, Resolution resolution,
                            uint32_t mipLevels);

[[nodiscard]] inline auto createImageSampler(const Device& device, const uint32_t mipLevels) -> vk::UniqueSampler {
    return device.logicalDevice->createSamplerUnique(
            vk::SamplerCreateInfo(vk::SamplerCreateFlags(),
                                  vk::Filter::eLinear,
                                  vk::Filter::eLinear,
                                  vk::SamplerMipmapMode::eLinear,
                                  vk::SamplerAddressMode::eRepeat,
                                  vk::SamplerAddressMode::eRepeat,
                                  vk::SamplerAddressMode::eRepeat,
                                  0.0f,
                                  vk::True,
                                  device.physicalDevice.getProperties().limits.maxSamplerAnisotropy,
                                  vk::False,
                                  vk::CompareOp::eAlways,
                                  0.0f,
                                  static_cast<float>(mipLevels),
                                  vk::BorderColor::eIntOpaqueBlack,
                                  vk::False));
}

ImageMemory::ImageMemory(const Device& device, const Resolution resolution, const vk::Format format,
                         const vk::ImageUsageFlags imageUsageFlags, const vk::MemoryPropertyFlags memoryPropertyFlags,
                         const vk::ImageAspectFlags aspectFlags, const vk::SampleCountFlagBits msaa,
                         const uint32_t mipLevels)
    : m_device{ device.logicalDevice.get() }, m_physicalDevice{ device.physicalDevice },
      m_commandPool{ device.commandPool.get() }, m_format{ format }, m_imageUsageFlags{ imageUsageFlags },
      m_memoryPropertyFlags{ memoryPropertyFlags }, m_aspectFlags{ aspectFlags }, m_msaa{ msaa },
      m_mipLevels{ mipLevels } {
    resize(resolution);
}

void ImageMemory::resize(const Resolution resolution) {
    if (resolution.width == m_resolution.height || resolution.height == m_resolution.height) {
        return;
    }
    m_resolution = resolution;
    m_image = m_device.createImageUnique(vk::ImageCreateInfo(vk::ImageCreateFlags(),
                                                             vk::ImageType::e2D,
                                                             m_format,
                                                             vk::Extent3D(m_resolution.width, m_resolution.height, 1),
                                                             m_mipLevels,
                                                             1,
                                                             m_msaa,
                                                             vk::ImageTiling::eOptimal,
                                                             m_imageUsageFlags,
                                                             vk::SharingMode::eExclusive));
    vk::MemoryRequirements memoryRequirements;
    m_device.getImageMemoryRequirements(m_image.get(), &memoryRequirements);
    m_memory = m_device.allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(m_physicalDevice, memoryRequirements.memoryTypeBits, m_memoryPropertyFlags)));
    m_device.bindImageMemory(m_image.get(), m_memory.get(), 0);
    m_imageView = m_device.createImageViewUnique(
            vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),
                                    m_image.get(),
                                    vk::ImageViewType::e2D,
                                    m_format,
                                    vk::ComponentMapping(),
                                    vk::ImageSubresourceRange(m_aspectFlags, 0, m_mipLevels, 0, 1)));
}

Vulkan2DTexture::Vulkan2DTexture(const Device& device, const TextureData& texture, vk::Format format)
    : m_imageMemory{ ImageMemory(device,
                                 texture.getResolution(),
                                 format,
                                 vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                         | vk::ImageUsageFlagBits::eSampled,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 vk::ImageAspectFlagBits::eColor,
                                 vk::SampleCountFlagBits::e1,
                                 texture.getMipLevels()) },
      m_sampler{ createImageSampler(device, texture.getMipLevels()) }, m_device{ device.logicalDevice.get() },
      m_physicalDevice{ device.physicalDevice }, m_commandPool{ device.commandPool.get() },
      m_graphicsQueue{ device.getGraphicQueue() }, m_format{ format } {
    setData(texture);
}
void Vulkan2DTexture::setData(const TextureData& texture) {
    m_imageMemory.resize(texture.getResolution());
    m_extent = vk::Extent2D(texture.getResolution().width, texture.getResolution().height);
    m_mipLevels = texture.getMipLevels();
    const auto data = texture.getData();
    const auto stagingMemoryBuffer =
            BufferMemory(m_device,
                         m_physicalDevice,
                         data.size(),
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = m_device.mapMemory(
            stagingMemoryBuffer.getMemory().get(), 0, data.size(), vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), data.size());
    m_device.unmapMemory(stagingMemoryBuffer.getMemory().get());

    transitImageLayout(m_device,
                       m_commandPool,
                       m_graphicsQueue,
                       m_imageMemory.getImage(),
                       vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal,
                       texture.getMipLevels());
    copyBufferToImage(m_device,
                      m_commandPool,
                      m_graphicsQueue,
                      stagingMemoryBuffer.getBuffer().get(),
                      m_imageMemory.getImage(),
                      texture.getResolution());
    generateMipmaps();
}

void Vulkan2DTexture::generateMipmaps() const {
    if (!(m_physicalDevice.getFormatProperties(m_format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(m_device, m_commandPool, m_graphicsQueue, [&](const vk::CommandBuffer& commandBuffer) {
        const auto getImageBarrier =
                [image = m_imageMemory.getImage()](const uint32_t mipLevel) -> vk::ImageMemoryBarrier {
            return vk::ImageMemoryBarrier(
                    vk::AccessFlagBits::eTransferWrite,
                    vk::AccessFlagBits::eTransferRead,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eTransferSrcOptimal,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    image,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mipLevel, 1, 0, 1));
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

}// namespace th::vulkan