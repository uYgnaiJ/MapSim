#include "vulkan.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

static void ok(VkResult r, const char *action)
{
    if (r != VK_SUCCESS)
        throw std::runtime_error(std::string(action) + " (Vulkan " + std::to_string(r) + ")");
}
#define VK_CALL(call) ok(call, #call)
uint32_t Renderer::memoryType(uint32_t mask, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties p;
    vkGetPhysicalDeviceMemoryProperties(gpu, &p);
    for (unsigned i = 0; i < p.memoryTypeCount; i++)
        if ((mask & (1u << i)) && (p.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    throw std::runtime_error("No suitable Vulkan memory type");
}
void Renderer::buffer(Buffer &b, VkDeviceSize bytes, VkBufferUsageFlags use)
{
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = bytes;
    info.usage = use;
    VK_CALL(vkCreateBuffer(device, &info, nullptr, &b.id));
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, b.id, &req);
    VkMemoryAllocateInfo mem{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mem.allocationSize = req.size;
    mem.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CALL(vkAllocateMemory(device, &mem, nullptr, &b.memory));
    VK_CALL(vkBindBufferMemory(device, b.id, b.memory, 0));
    VK_CALL(vkMapMemory(device, b.memory, 0, bytes, 0, &b.mapped));
    b.size = bytes;
}
void Renderer::release(Buffer &b)
{
    if (b.mapped)
        vkUnmapMemory(device, b.memory);
    vkDestroyBuffer(device, b.id, nullptr);
    vkFreeMemory(device, b.memory, nullptr);
    b = {};
}
void Renderer::image(VkImage &id, VkDeviceMemory &mem, VkImageView &view, VkFormat fmt, VkExtent2D size, VkImageUsageFlags use, VkSampleCountFlagBits count)
{
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = fmt;
    ci.extent = {size.width, size.height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = count;
    ci.usage = use;
    VK_CALL(vkCreateImage(device, &ci, nullptr, &id));
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, id, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CALL(vkAllocateMemory(device, &ai, nullptr, &mem));
    VK_CALL(vkBindImageMemory(device, id, mem, 0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = id;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = fmt;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VK_CALL(vkCreateImageView(device, &vi, nullptr, &view));
}
void Renderer::init(HWND win)
{
    const char *extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Field Atlas";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ii{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ii.pApplicationInfo = &app;
    ii.enabledExtensionCount = 2;
    ii.ppEnabledExtensionNames = extensions;
    VK_CALL(vkCreateInstance(&ii, nullptr, &instance));
    VkWin32SurfaceCreateInfoKHR si{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    si.hwnd = win;
    si.hinstance = GetModuleHandle(nullptr);
    VK_CALL(vkCreateWin32SurfaceKHR(instance, &si, nullptr, &surface));
    uint32_t n = 0;
    VK_CALL(vkEnumeratePhysicalDevices(instance, &n, nullptr));
    std::vector<VkPhysicalDevice> devices(n);
    if (n)
        VK_CALL(vkEnumeratePhysicalDevices(instance, &n, devices.data()));
    for (auto d : devices)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &count, nullptr);
        std::vector<VkQueueFamilyProperties> props(count);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &count, props.data());
        for (uint32_t i = 0; i < count; i++)
        {
            VkBool32 present = 0;
            VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &present));
            if (present && (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                gpu = d;
                family = i;
                break;
            }
        }
        if (gpu)
            break;
    }
    if (!gpu)
        throw std::runtime_error("A Vulkan graphics and presentation queue is required");
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(gpu, &props);
    if (props.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT)
        samples = VK_SAMPLE_COUNT_4_BIT;
    float priority = 1;
    VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qi.queueFamilyIndex = family;
    qi.queueCount = 1;
    qi.pQueuePriorities = &priority;
    const char *ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    di.queueCreateInfoCount = 1;
    di.pQueueCreateInfos = &qi;
    di.enabledExtensionCount = 1;
    di.ppEnabledExtensionNames = &ext;
    VK_CALL(vkCreateDevice(gpu, &di, nullptr, &device));
    vkGetDeviceQueue(device, family, 0, &queue);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = family;
    pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CALL(vkCreateCommandPool(device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ci{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ci.commandPool = pool;
    ci.commandBufferCount = 1;
    VK_CALL(vkAllocateCommandBuffers(device, &ci, &command));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CALL(vkCreateFence(device, &fi, nullptr, &fence));
    VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CALL(vkCreateSemaphore(device, &sem, nullptr, &acquired));
    VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dl.bindingCount = 1;
    dl.pBindings = &binding;
    VK_CALL(vkCreateDescriptorSetLayout(device, &dl, nullptr, &descriptorLayout));
    VkPushConstantRange pc{VK_SHADER_STAGE_VERTEX_BIT, 0, 8};
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &descriptorLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pc;
    VK_CALL(vkCreatePipelineLayout(device, &pli, nullptr, &layout));
    VkDescriptorPoolSize ps{binding.descriptorType, 1};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dp.maxSets = 1;
    dp.poolSizeCount = 1;
    dp.pPoolSizes = &ps;
    VK_CALL(vkCreateDescriptorPool(device, &dp, nullptr, &descriptorPool));
    VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    da.descriptorPool = descriptorPool;
    da.descriptorSetCount = 1;
    da.pSetLayouts = &descriptorLayout;
    VK_CALL(vkAllocateDescriptorSets(device, &da, &descriptor));
    VkSamplerCreateInfo samp{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samp.magFilter = samp.minFilter = VK_FILTER_LINEAR;
    samp.addressModeU = samp.addressModeV = samp.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VK_CALL(vkCreateSampler(device, &samp, nullptr, &sampler));
    image(atlas, atlasMemory, atlasView, VK_FORMAT_B8G8R8A8_UNORM, {Atlas::side, Atlas::side}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SAMPLE_COUNT_1_BIT);
    buffer(staging, Atlas::side * Atlas::side * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VkDescriptorImageInfo source{sampler, atlasView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = descriptor;
    write.descriptorType = binding.descriptorType;
    write.descriptorCount = 1;
    write.pImageInfo = &source;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}
void Renderer::createPipeline()
{
    const uint32_t vs[] =
#include "vert.inc"
        ;
    const uint32_t fs[] =
#include "frag.inc"
        ;
    VkShaderModule modules[2]{};
    VkPipelineShaderStageCreateInfo stages[2]{};
    for (int i = 0; i < 2; i++)
    {
        VkShaderModuleCreateInfo mi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        mi.codeSize = i ? sizeof(fs) : sizeof(vs);
        mi.pCode = i ? fs : vs;
        VK_CALL(vkCreateShaderModule(device, &mi, nullptr, &modules[i]));
        stages[i] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[i].stage = i ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;
        stages[i].module = modules[i];
        stages[i].pName = "main";
    }
    VkVertexInputBindingDescription bind{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attr[] = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 0, VK_FORMAT_R32G32_SFLOAT, 8}, {2, 0, VK_FORMAT_R8G8B8A8_UNORM, 16}};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bind;
    vi.vertexAttributeDescriptionCount = 3;
    vi.pVertexAttributeDescriptions = attr;
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.lineWidth = 1;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = samples;
    VkPipelineColorBlendAttachmentState blend{};
    blend.blendEnable = 1;
    blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend.colorWriteMask = 15;
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &blend;
    VkDynamicState ds[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2;
    dy.pDynamicStates = ds;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vi;
    info.pInputAssemblyState = &ia;
    info.pViewportState = &vp;
    info.pRasterizationState = &rs;
    info.pMultisampleState = &ms;
    info.pColorBlendState = &cb;
    info.pDynamicState = &dy;
    info.layout = layout;
    info.renderPass = pass;
    auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
    for (auto m : modules)
        vkDestroyShaderModule(device, m, nullptr);
    ok(result, "Create graphics pipeline");
}
void Renderer::clearSwap()
{
    for (auto f : frames)
        vkDestroyFramebuffer(device, f, nullptr);
    for (auto v : views)
        vkDestroyImageView(device, v, nullptr);
    for (auto s : presented)
        vkDestroySemaphore(device, s, nullptr);
    frames.clear();
    views.clear();
    presented.clear();
    vkDestroyImageView(device, msaaView, nullptr);
    vkDestroyImage(device, msaa, nullptr);
    vkFreeMemory(device, msaaMemory, nullptr);
    msaaView = {};
    msaa = {};
    msaaMemory = {};
    vkDestroyPipeline(device, pipeline, nullptr);
    pipeline = {};
    vkDestroyRenderPass(device, pass, nullptr);
    pass = {};
    vkDestroySwapchainKHR(device, swap, nullptr);
    swap = {};
    extent = {};
}
void Renderer::resize(unsigned w, unsigned h)
{
    VK_CALL(vkDeviceWaitIdle(device));
    clearSwap();
    VkSurfaceCapabilitiesKHR caps;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps));
    uint32_t n;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &n, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(n);
    if (!n)
        throw std::runtime_error("No Vulkan surface formats");
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &n, formats.data()));
    auto chosen = formats.front();
    for (auto f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            chosen = f;
            break;
        }
    format = chosen.format == VK_FORMAT_UNDEFINED ? VK_FORMAT_B8G8R8A8_UNORM : chosen.format;
    extent = caps.currentExtent.width != UINT32_MAX ? caps.currentExtent : VkExtent2D{std::clamp(w, caps.minImageExtent.width, caps.maxImageExtent.width), std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height)};
    if (!extent.width || !extent.height)
        return;
    VkSwapchainCreateInfoKHR sw{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sw.surface = surface;
    sw.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount)
        sw.minImageCount = std::min(sw.minImageCount, caps.maxImageCount);
    sw.imageFormat = format;
    sw.imageColorSpace = chosen.colorSpace;
    sw.imageExtent = extent;
    sw.imageArrayLayers = 1;
    sw.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sw.preTransform = caps.currentTransform;
    sw.compositeAlpha = VkCompositeAlphaFlagBitsKHR(caps.supportedCompositeAlpha & (-caps.supportedCompositeAlpha));
    sw.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sw.clipped = VK_TRUE;
    VK_CALL(vkCreateSwapchainKHR(device, &sw, nullptr, &swap));
    VkAttachmentDescription at[2]{};
    at[0].format = format;
    at[0].samples = samples;
    at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    at[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    bool multi = samples != VK_SAMPLE_COUNT_1_BIT;
    at[0].finalLayout = multi ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    at[1] = at[0];
    at[1].samples = VK_SAMPLE_COUNT_1_BIT;
    at[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    at[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, resolve{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &color;
    if (multi)
        sub.pResolveAttachments = &resolve;
    VkSubpassDependency dep{VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = multi ? 2 : 1;
    rp.pAttachments = at;
    rp.subpassCount = 1;
    rp.pSubpasses = &sub;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;
    VK_CALL(vkCreateRenderPass(device, &rp, nullptr, &pass));
    createPipeline();
    if (multi)
        image(msaa, msaaMemory, msaaView, format, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, samples);
    VK_CALL(vkGetSwapchainImagesKHR(device, swap, &n, nullptr));
    std::vector<VkImage> images(n);
    VK_CALL(vkGetSwapchainImagesKHR(device, swap, &n, images.data()));
    views.resize(n);
    frames.resize(n);
    presented.resize(n);
    for (unsigned i = 0; i < n; i++)
    {
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = images[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = format;
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CALL(vkCreateImageView(device, &vi, nullptr, &views[i]));
        VkImageView attachments[] = {msaaView, views[i]};
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = pass;
        fb.attachmentCount = multi ? 2 : 1;
        fb.pAttachments = attachments + (multi ? 0 : 1);
        fb.width = extent.width;
        fb.height = extent.height;
        fb.layers = 1;
        VK_CALL(vkCreateFramebuffer(device, &fb, nullptr, &frames[i]));
        VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CALL(vkCreateSemaphore(device, &sem, nullptr, &presented[i]));
    }
}
void Renderer::draw(Canvas &c, unsigned w, unsigned h, float lw, float lh)
{
    if (!w || !h)
        return;
    if (extent.width != w || extent.height != h || !swap)
        resize(w, h);
    if (!swap)
        return;
    VK_CALL(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    uint32_t index{};
    auto acq = vkAcquireNextImageKHR(device, swap, UINT64_MAX, acquired, VK_NULL_HANDLE, &index);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR)
    {
        extent = {};
        return;
    }
    if (acq != VK_SUBOPTIMAL_KHR)
        ok(acq, "Acquire image");
    auto bytes = c.vertices.size() * sizeof(Vertex);
    if (vertex.size < bytes)
    {
        release(vertex);
        buffer(vertex, bytes + 65536, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }
    memcpy(vertex.mapped, c.vertices.data(), bytes);
    VK_CALL(vkResetCommandBuffer(command, 0));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CALL(vkBeginCommandBuffer(command, &begin));
    if (c.atlas.dirty)
    {
        GdiFlush();
        memcpy(staging.mapped, c.atlas.pixels, staging.size);
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.image = atlas;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.oldLayout = uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcAccessMask = uploaded ? VK_ACCESS_SHADER_READ_BIT : 0;
        b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command, uploaded ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {Atlas::side, Atlas::side, 1};
        vkCmdCopyBufferToImage(command, staging.id, atlas, b.newLayout, 1, &copy);
        b.oldLayout = b.newLayout;
        b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        c.atlas.dirty = false;
        uploaded = true;
    }
    VkClearValue clear{};
    clear.color.float32[3] = 1;
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = pass;
    rp.framebuffer = frames[index];
    rp.renderArea.extent = extent;
    rp.clearValueCount = 1;
    rp.pClearValues = &clear;
    vkCmdBeginRenderPass(command, &rp, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vp{0, 0, float(extent.width), float(extent.height), 0, 1};
    vkCmdSetViewport(command, 0, 1, &vp);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptor, 0, nullptr);
    float size[] = {lw, lh};
    vkCmdPushConstants(command, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8, size);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command, 0, 1, &vertex.id, &offset);
    for (auto &b : c.batches)
    {
        if (!b.count)
            continue;
        auto x = [&](float v)
        { return std::clamp(int(std::round(v * extent.width / lw)), 0, int(extent.width)); };
        auto y = [&](float v)
        { return std::clamp(int(std::round(v * extent.height / lh)), 0, int(extent.height)); };
        int l = x(b.clip.x), t = y(b.clip.y), r = x(b.clip.x + b.clip.w), bottom = y(b.clip.y + b.clip.h);
        if (r <= l || bottom <= t)
            continue;
        VkRect2D sc{{l, t}, {uint32_t(r - l), uint32_t(bottom - t)}};
        vkCmdSetScissor(command, 0, 1, &sc);
        vkCmdDraw(command, b.count, 1, b.first, 0);
    }
    vkCmdEndRenderPass(command);
    VK_CALL(vkEndCommandBuffer(command));
    VK_CALL(vkResetFences(device, 1, &fence));
    VkPipelineStageFlags wait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquired;
    submit.pWaitDstStageMask = &wait;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &presented[index];
    VK_CALL(vkQueueSubmit(queue, 1, &submit, fence));
    VkPresentInfoKHR p{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    p.waitSemaphoreCount = 1;
    p.pWaitSemaphores = &presented[index];
    p.swapchainCount = 1;
    p.pSwapchains = &swap;
    p.pImageIndices = &index;
    auto result = vkQueuePresentKHR(queue, &p);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        extent = {};
    else
        ok(result, "Present frame");
}
Renderer::~Renderer()
{
    if (device)
    {
        vkDeviceWaitIdle(device);
        clearSwap();
        release(vertex);
        release(staging);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
        vkDestroyImageView(device, atlasView, nullptr);
        vkDestroyImage(device, atlas, nullptr);
        vkFreeMemory(device, atlasMemory, nullptr);
        vkDestroySampler(device, sampler, nullptr);
        vkDestroyFence(device, fence, nullptr);
        vkDestroySemaphore(device, acquired, nullptr);
        vkDestroyCommandPool(device, pool, nullptr);
        vkDestroyDevice(device, nullptr);
    }
    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance)
        vkDestroyInstance(instance, nullptr);
}