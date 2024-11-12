module;

export module thyme.platform.vulkan:renderer;
import :graphic_pipeline;
import :utils;

import thyme.core.renderer;

export namespace Thyme::Vulkan {

struct VulkanRendererOptions {};

class VulkanRenderer final: public Renderer {
public:
    VulkanRenderer(const Device& device, const FrameDataList& frameDataList)
        : m_device{ device }, m_frameDataList{ frameDataList } {}

    void draw() const override {
        for (const auto& pipeline : m_pipelines) {
            // pipeline.draw();
        }
    }

private:
    std::vector<GraphicPipeline> m_pipelines;

    const Device& m_device;
    const FrameDataList& m_frameDataList;

    /*auto getNextFrameIndex = [frameIndex = 0, maxFrames]() mutable {
        const auto currentFrameIndex = frameIndex;
        frameIndex = (frameIndex + 1) % maxFrames;
        return currentFrameIndex;
    };*/
};
}// namespace Thyme::Vulkan