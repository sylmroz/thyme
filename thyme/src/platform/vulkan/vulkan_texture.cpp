#include <thyme/platform/vulkan/vulkan_texture.hpp>

namespace th::vulkan {

static void generateMipmaps(const Device& device, vk::Image image, vk::Format format, Resolution resolution,
                            uint32_t mipLevels);

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

Vulkan2DTexture::Vulkan2DTexture(const Device& device, const TextureData& texture)
    : m_imageMemory{ ImageMemory(device,
                                 texture.getResolution(),
                                 vk::Format::eR8G8B8A8Srgb,
                                 vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                         | vk::ImageUsageFlagBits::eSampled,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 vk::ImageAspectFlagBits::eColor,
                                 vk::SampleCountFlagBits::e1,
                                 texture.getMipLevels()) },
      m_device{ device.logicalDevice.get() }, m_physicalDevice{ device.physicalDevice },
      m_sampler{ createImageSampler(device, texture.getMipLevels()) } {
    const auto data = texture.getData();
    const auto stagingMemoryBuffer =
            BufferMemory(device,
                         data.size(),
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = device.logicalDevice->mapMemory(
            stagingMemoryBuffer.getMemory().get(), 0, data.size(), vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), data.size());
    device.logicalDevice->unmapMemory(stagingMemoryBuffer.getMemory().get());

    const auto commandPool = device.commandPool.get();
    transitImageLayout(device,
                       device.commandPool.get(),
                       m_imageMemory.getImage(),
                       vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal,
                       texture.getMipLevels());
    copyBufferToImage(device,
                      commandPool,
                      stagingMemoryBuffer.getBuffer().get(),
                      m_imageMemory.getImage(),
                      texture.getResolution());
    generateMipmaps(device,
                    m_imageMemory.getImage(),
                    vk::Format::eR8G8B8A8Srgb,
                    texture.getResolution(),
                    texture.getMipLevels());
}
void Vulkan2DTexture::setData(const TextureData& texture) {
}

static void generateMipmaps(const Device& device, const vk::Image image, const vk::Format format,
                            const Resolution resolution, const uint32_t mipLevels) {
    if (!(device.physicalDevice.getFormatProperties(format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(device, [&](const vk::CommandBuffer& commandBuffer) {
        const auto getImageBarrier = [&image](const uint32_t mipLevel) -> vk::ImageMemoryBarrier {
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
        int mipWidth = static_cast<int>(resolution.width);
        int mipHeight = static_cast<int>(resolution.height);
        for (uint32_t mipLevel = 0; mipLevel < mipLevels - 1; ++mipLevel, mipWidth /= 2, mipHeight /= 2) {
            auto barrier = getImageBarrier(mipLevel);
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eTransfer,
                                          vk::DependencyFlags(),
                                          {},
                                          {},
                                          { barrier });

            const auto blit = getImageBlit(mipLevel, mipWidth, mipHeight);
            commandBuffer.blitImage(image,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    image,
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
        auto barrier = getImageBarrier(mipLevels - 1);
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