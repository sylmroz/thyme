#include "thyme/platform/vulkan/vulkan_buffer.hpp"
#include <thyme/platform/vulkan/utils.hpp>
#include <thyme/platform/vulkan/vulkan_texture.hpp>

namespace th::vulkan {

[[nodiscard]] static auto createImageSampler(const vk::Device device, const float maxSamplerAnisotropy,
                                             const uint32_t mipLevels) -> vk::UniqueSampler {
    return device.createSamplerUnique(vk::SamplerCreateInfo(vk::SamplerCreateFlags(),
                                                            vk::Filter::eLinear,
                                                            vk::Filter::eLinear,
                                                            vk::SamplerMipmapMode::eLinear,
                                                            vk::SamplerAddressMode::eRepeat,
                                                            vk::SamplerAddressMode::eRepeat,
                                                            vk::SamplerAddressMode::eRepeat,
                                                            0.0f,
                                                            vk::True,
                                                            maxSamplerAnisotropy,
                                                            vk::False,
                                                            vk::CompareOp::eAlways,
                                                            0.0f,
                                                            static_cast<float>(mipLevels),
                                                            vk::BorderColor::eIntOpaqueBlack,
                                                            vk::False));
}

ImageMemory::ImageMemory(const VulkanDevice& device, const vk::Extent2D resolution, const vk::Format format,
                         const vk::ImageUsageFlags imageUsageFlags, const vk::MemoryPropertyFlags memoryPropertyFlags,
                         const vk::ImageAspectFlags aspectFlags, const vk::SampleCountFlagBits msaa,
                         const uint32_t mipLevels)
    : m_format{ format }, m_imageUsageFlags{ imageUsageFlags }, m_memoryPropertyFlags{ memoryPropertyFlags },
      m_aspectFlags{ aspectFlags }, m_msaa{ msaa }, m_device{ device }, m_mipLevels{ mipLevels } {
    resize(resolution);
}

void ImageMemory::resize(const vk::Extent2D resolution) {
    if (resolution.width == m_resolution.width && resolution.height == m_resolution.height) {
        return;
    }
    m_resolution = resolution;
    m_image = m_device.logicalDevice.createImageUnique(
            vk::ImageCreateInfo(vk::ImageCreateFlags(),
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
    m_device.logicalDevice.getImageMemoryRequirements(m_image.get(), &memoryRequirements);
    m_memory = m_device.logicalDevice.allocateMemoryUnique(vk::MemoryAllocateInfo(
            memoryRequirements.size,
            findMemoryType(m_device.physicalDevice, memoryRequirements.memoryTypeBits, m_memoryPropertyFlags)));
    m_device.logicalDevice.bindImageMemory(m_image.get(), m_memory.get(), 0);
    m_imageView = m_device.logicalDevice.createImageViewUnique(
            vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),
                                    m_image.get(),
                                    vk::ImageViewType::e2D,
                                    m_format,
                                    vk::ComponentMapping(),
                                    vk::ImageSubresourceRange(m_aspectFlags, 0, m_mipLevels, 0, 1)));
}
void ImageMemory::transitImageLayout(ImageLayoutTransition layoutTransition) {
    m_device.singleTimeCommand(
            [layoutTransition, image = m_image.get(), mipLevels = m_mipLevels](const vk::CommandBuffer commandBuffer) {
                vulkan::transitImageLayout(
                        commandBuffer, image, layoutTransition.oldLayout, layoutTransition.newLayout, mipLevels);
            });
}

DepthImageMemory::DepthImageMemory(const VulkanDevice& device, const vk::Extent2D resolution, const vk::Format format,
                                   const vk::SampleCountFlagBits msaa)
    : ImageMemory(device, resolution, format, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                  vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth, msaa, 1) {}

ColorImageMemory::ColorImageMemory(const VulkanDevice& device, const vk::Extent2D resolution, const vk::Format format,
                                   const vk::SampleCountFlagBits msaa)
    : ImageMemory(device, resolution, format,
                  vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                  vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor, msaa, 1) {}

Vulkan2DTexture::Vulkan2DTexture(const VulkanDevice& device, const TextureData& texture, const vk::Format format)
    : m_imageMemory{ ImageMemory(device,
                                 vk::Extent2D(texture.getResolution().width, texture.getResolution().height),
                                 format,
                                 vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
                                         | vk::ImageUsageFlagBits::eSampled,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 vk::ImageAspectFlagBits::eColor,
                                 vk::SampleCountFlagBits::e1,
                                 texture.getMipLevels()) },
      m_sampler{ createImageSampler(device.logicalDevice,
                                    device.physicalDevice.getProperties().limits.maxSamplerAnisotropy,
                                    texture.getMipLevels()) },
      m_format{ format }, m_device{ device } {
    setData(texture);
}

void Vulkan2DTexture::setData(const TextureData& texture) {
    m_extent = vk::Extent2D(texture.getResolution().width, texture.getResolution().height);
    m_imageMemory.resize(m_extent);
    m_mipLevels = texture.getMipLevels();

    const auto data = texture.getData();
    const auto stagingMemoryBuffer =
            BufferMemory(m_device.logicalDevice,
                         m_device.physicalDevice,
                         data.size(),
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mappedMemory = nullptr;
    [[maybe_unused]] const auto result = m_device.logicalDevice.mapMemory(
            stagingMemoryBuffer.getMemory().get(), 0, data.size(), vk::MemoryMapFlags(), &mappedMemory);
    memcpy(mappedMemory, data.data(), data.size());
    m_device.logicalDevice.unmapMemory(stagingMemoryBuffer.getMemory().get());

    m_imageMemory.transitImageLayout(ImageLayoutTransition{
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
    });

    const auto graphicsQueue = m_device.getGraphicQueue();
    copyBufferToImage(m_device.logicalDevice,
                      m_device.commandPool,
                      graphicsQueue,
                      stagingMemoryBuffer.getBuffer().get(),
                      m_imageMemory.getImage(),
                      texture.getResolution());
    generateMipmaps();
}

void Vulkan2DTexture::generateMipmaps() const {
    const auto device = m_device.logicalDevice;
    const auto physicalDevice = m_device.physicalDevice;
    const auto commandPool = m_device.commandPool;
    const auto graphicsQueue = m_device.getGraphicQueue();
    if (!(physicalDevice.getFormatProperties(m_format).optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Failed to generate mipmaps! Unsupported linear filtering!");
    }
    singleTimeCommand(device, commandPool, graphicsQueue, [&](const vk::CommandBuffer& commandBuffer) {
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