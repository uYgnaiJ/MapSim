#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "canvas.hpp"

class Renderer {
    VkInstance instance{}; VkSurfaceKHR surface{}; VkPhysicalDevice gpu{};
    VkDevice device{}; VkQueue queue{}; uint32_t family{};
    VkCommandPool pool{}; VkCommandBuffer command{};
    VkFence fence{}; VkSemaphore acquired{};
    VkSwapchainKHR swap{}; VkExtent2D extent{}; VkFormat format{};
    std::vector<VkImageView> views; std::vector<VkFramebuffer> frames;
    std::vector<VkSemaphore> presented;
    VkRenderPass pass{}; VkPipeline pipeline{}; VkPipelineLayout layout{};
    VkDescriptorSetLayout descriptorLayout{}; VkDescriptorPool descriptorPool{};
    VkDescriptorSet descriptor{}; VkSampler sampler{};
    VkImage atlas{}; VkImageView atlasView{}; VkDeviceMemory atlasMemory{};
    VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
    VkImage msaa{}; VkImageView msaaView{}; VkDeviceMemory msaaMemory{};
    struct Buffer {VkBuffer id{}; VkDeviceMemory memory{}; void* mapped{}; VkDeviceSize size{};};
    Buffer vertex,staging; bool uploaded=false;
    uint32_t memoryType(uint32_t mask,VkMemoryPropertyFlags props);
    void buffer(Buffer& b,VkDeviceSize bytes,VkBufferUsageFlags use);
    void release(Buffer& b);
    void image(VkImage& id,VkDeviceMemory& memory,VkImageView& view,VkFormat fmt,VkExtent2D size,VkImageUsageFlags use,VkSampleCountFlagBits count);
    void createPipeline();
    void resize(unsigned w,unsigned h);
    void clearSwap();
public:
    void init(HWND window);
    void draw(Canvas& c,unsigned w,unsigned h,float logicalW,float logicalH);
    ~Renderer();
};