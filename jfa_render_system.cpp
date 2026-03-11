#include "jfa_render_system.h"
#include "lot_model.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <vector>

namespace lot {

// ─────────────────────────────────────────────────────────────────────────────
// Push constant structs (must match shader layouts)
// ─────────────────────────────────────────────────────────────────────────────
struct MaskPushConstant {
    glm::mat4 modelMatrix;
};

struct JfaComputePush {
    glm::ivec2 imageSize;
    int        stepSize;
};

struct OutlinePush {
    glm::vec2 resolution;
    float     outlineThickness;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<char> readSpirvFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("JFA: cannot open shader: " + path);
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), size);
    return buf;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
JFARenderSystem::JFARenderSystem(LotDevice& device,
                                 VkRenderPass mainRenderPass,
                                 VkExtent2D extent,
                                 int frameCount,
                                 VkDescriptorSetLayout globalSetLayout)
    : lotDevice{device}, extent{extent}
{
    // Number of JFA iterations = ceil(log2(max(w, h)))
    int maxDim = static_cast<int>(std::max(extent.width, extent.height));
    numJfaSteps = static_cast<int>(std::ceil(std::log2(static_cast<float>(maxDim))));
    // After numJfaSteps ping-pong steps (starting from jfa[0]),
    // result lives in jfa[numJfaSteps % 2]
    finalJfaIdx = numJfaSteps % 2;

    createImages(frameCount);
    createSampler();
    createSelectionMaskRenderPass();
    createSelectionMaskFramebuffers(frameCount);
    createComputeDescriptorLayout();
    createOutlineDescriptorLayout();
    createDescriptorPool(frameCount);
    allocateDescriptorSets(frameCount);
    createComputePipeline();
    createSelectionMaskPipeline(globalSetLayout);
    createOutlinePipeline(mainRenderPass);
}

JFARenderSystem::~JFARenderSystem() {
    auto dev = lotDevice.device();
    vkDeviceWaitIdle(dev);

    // Pipelines
    maskPipeline.reset();
    outlinePipeline.reset();
    if (jfaComputePipeline)    vkDestroyPipeline(dev, jfaComputePipeline, nullptr);
    if (jfaComputeLayout)      vkDestroyPipelineLayout(dev, jfaComputeLayout, nullptr);
    if (maskPipelineLayout)    vkDestroyPipelineLayout(dev, maskPipelineLayout, nullptr);
    if (outlinePipelineLayout) vkDestroyPipelineLayout(dev, outlinePipelineLayout, nullptr);

    // Descriptor resources
    if (computePool)           vkDestroyDescriptorPool(dev, computePool, nullptr);
    if (computeDescLayout)     vkDestroyDescriptorSetLayout(dev, computeDescLayout, nullptr);
    if (outlineDescLayout)     vkDestroyDescriptorSetLayout(dev, outlineDescLayout, nullptr);

    // Sampler
    if (sampler)               vkDestroySampler(dev, sampler, nullptr);

    // Renderpass
    if (selectionMaskRenderPass) vkDestroyRenderPass(dev, selectionMaskRenderPass, nullptr);

    destroyImages();
}

// ─────────────────────────────────────────────────────────────────────────────
// Image creation
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createImages(int frameCount) {
    frames.resize(frameCount);

    for (auto& f : frames) {
        // --- Selection mask: R8G8B8A8_UNORM, COLOR_ATTACHMENT + SAMPLED ---
        {
            VkImageCreateInfo info{};
            info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType     = VK_IMAGE_TYPE_2D;
            info.format        = VK_FORMAT_R8G8B8A8_UNORM;
            info.extent        = {extent.width, extent.height, 1};
            info.mipLevels     = 1;
            info.arrayLayers   = 1;
            info.samples       = VK_SAMPLE_COUNT_1_BIT;
            info.tiling        = VK_IMAGE_TILING_OPTIMAL;
            info.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT;
            info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            lotDevice.createImageWithInfo(info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          f.maskImage, f.maskMemory);

            VkImageViewCreateInfo view{};
            view.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.image    = f.maskImage;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format   = VK_FORMAT_R8G8B8A8_UNORM;
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            if (vkCreateImageView(lotDevice.device(), &view, nullptr, &f.maskView) != VK_SUCCESS)
                throw std::runtime_error("JFA: failed to create mask image view");
        }

        // --- Depth for selection mask renderpass: D32_SFLOAT ---
        {
            VkImageCreateInfo info{};
            info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType     = VK_IMAGE_TYPE_2D;
            info.format        = VK_FORMAT_D32_SFLOAT;
            info.extent        = {extent.width, extent.height, 1};
            info.mipLevels     = 1;
            info.arrayLayers   = 1;
            info.samples       = VK_SAMPLE_COUNT_1_BIT;
            info.tiling        = VK_IMAGE_TILING_OPTIMAL;
            info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            lotDevice.createImageWithInfo(info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          f.depthImage, f.depthMemory);

            VkImageViewCreateInfo view{};
            view.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.image    = f.depthImage;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format   = VK_FORMAT_D32_SFLOAT;
            view.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
            if (vkCreateImageView(lotDevice.device(), &view, nullptr, &f.depthView) != VK_SUCCESS)
                throw std::runtime_error("JFA: failed to create depth image view");
        }

        // --- JFA ping-pong A/B: R32G32_SFLOAT, STORAGE + SAMPLED ---
        for (int i = 0; i < 2; i++) {
            VkImageCreateInfo info{};
            info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType     = VK_IMAGE_TYPE_2D;
            info.format        = VK_FORMAT_R32G32_SFLOAT;
            info.extent        = {extent.width, extent.height, 1};
            info.mipLevels     = 1;
            info.arrayLayers   = 1;
            info.samples       = VK_SAMPLE_COUNT_1_BIT;
            info.tiling        = VK_IMAGE_TILING_OPTIMAL;
            info.usage         = VK_IMAGE_USAGE_STORAGE_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT;
            info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            lotDevice.createImageWithInfo(info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          f.jfaImage[i], f.jfaMemory[i]);

            VkImageViewCreateInfo view{};
            view.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.image    = f.jfaImage[i];
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format   = VK_FORMAT_R32G32_SFLOAT;
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            if (vkCreateImageView(lotDevice.device(), &view, nullptr, &f.jfaView[i]) != VK_SUCCESS)
                throw std::runtime_error("JFA: failed to create jfa image view");
        }
    }
}

void JFARenderSystem::destroyImages() {
    auto dev = lotDevice.device();
    for (auto& f : frames) {
        // Framebuffer
        if (f.maskFramebuffer) vkDestroyFramebuffer(dev, f.maskFramebuffer, nullptr);
        // Mask
        if (f.maskView)   vkDestroyImageView(dev, f.maskView, nullptr);
        if (f.maskImage)  vkDestroyImage(dev, f.maskImage, nullptr);
        if (f.maskMemory) vkFreeMemory(dev, f.maskMemory, nullptr);
        // Depth
        if (f.depthView)   vkDestroyImageView(dev, f.depthView, nullptr);
        if (f.depthImage)  vkDestroyImage(dev, f.depthImage, nullptr);
        if (f.depthMemory) vkFreeMemory(dev, f.depthMemory, nullptr);
        // JFA
        for (int i = 0; i < 2; i++) {
            if (f.jfaView[i])   vkDestroyImageView(dev, f.jfaView[i], nullptr);
            if (f.jfaImage[i])  vkDestroyImage(dev, f.jfaImage[i], nullptr);
            if (f.jfaMemory[i]) vkFreeMemory(dev, f.jfaMemory[i], nullptr);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Selection mask renderpass
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createSelectionMaskRenderPass() {
    // Attachment 0: color (R8G8B8A8_UNORM)
    VkAttachmentDescription colorAttach{};
    colorAttach.format         = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttach.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttach.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Attachment 1: depth
    VkAttachmentDescription depthAttach{};
    depthAttach.format         = VK_FORMAT_D32_SFLOAT;
    depthAttach.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttach.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttach.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    // Dependency: ensure color write is visible to compute read (SAMPLED)
    VkSubpassDependency dep{};
    dep.srcSubpass    = 0;
    dep.dstSubpass    = VK_SUBPASS_EXTERNAL;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttach, depthAttach};
    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments    = attachments.data();
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dep;

    if (vkCreateRenderPass(lotDevice.device(), &rpInfo, nullptr, &selectionMaskRenderPass) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create selection mask renderpass");
}

void JFARenderSystem::createSelectionMaskFramebuffers(int frameCount) {
    for (int i = 0; i < frameCount; i++) {
        std::array<VkImageView, 2> attachments = {frames[i].maskView, frames[i].depthView};
        VkFramebufferCreateInfo fb{};
        fb.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass      = selectionMaskRenderPass;
        fb.attachmentCount = static_cast<uint32_t>(attachments.size());
        fb.pAttachments    = attachments.data();
        fb.width           = extent.width;
        fb.height          = extent.height;
        fb.layers          = 1;
        if (vkCreateFramebuffer(lotDevice.device(), &fb, nullptr, &frames[i].maskFramebuffer) != VK_SUCCESS)
            throw std::runtime_error("JFA: failed to create mask framebuffer");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Sampler
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createSampler() {
    VkSamplerCreateInfo info{};
    info.sType      = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter  = VK_FILTER_NEAREST;
    info.minFilter  = VK_FILTER_NEAREST;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.maxLod       = 0.0f;
    if (vkCreateSampler(lotDevice.device(), &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create sampler");
}

// ─────────────────────────────────────────────────────────────────────────────
// Descriptor layouts
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createComputeDescriptorLayout() {
    // binding 0: COMBINED_IMAGE_SAMPLER (input)
    // binding 1: STORAGE_IMAGE (output)
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
    bindings[0].binding         = 0;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding         = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings    = bindings.data();
    if (vkCreateDescriptorSetLayout(lotDevice.device(), &info, nullptr, &computeDescLayout) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create compute desc layout");
}

void JFARenderSystem::createOutlineDescriptorLayout() {
    // binding 0: COMBINED_IMAGE_SAMPLER (JFA result)
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings    = &binding;
    if (vkCreateDescriptorSetLayout(lotDevice.device(), &info, nullptr, &outlineDescLayout) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create outline desc layout");
}

// ─────────────────────────────────────────────────────────────────────────────
// Descriptor pool + sets
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createDescriptorPool(int frameCount) {
    // Per frame: 3 compute sets (initSet, atobSet, btoaSet) + 1 outline set
    // Each compute set: 1 sampler + 1 storage image
    // Outline set: 1 sampler
    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[0].descriptorCount = static_cast<uint32_t>(frameCount * 4); // 3 compute + 1 outline
    sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    sizes[1].descriptorCount = static_cast<uint32_t>(frameCount * 3); // 3 compute output

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes    = sizes.data();
    info.maxSets       = static_cast<uint32_t>(frameCount * 4);

    if (vkCreateDescriptorPool(lotDevice.device(), &info, nullptr, &computePool) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create descriptor pool");
}

void JFARenderSystem::allocateDescriptorSets(int frameCount) {
    for (int i = 0; i < frameCount; i++) {
        auto& f = frames[i];

        // Allocate 3 compute sets + 1 outline set
        std::array<VkDescriptorSetLayout, 4> layouts = {
            computeDescLayout, computeDescLayout, computeDescLayout, outlineDescLayout
        };
        std::array<VkDescriptorSet, 4> sets;
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = computePool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts        = layouts.data();
        if (vkAllocateDescriptorSets(lotDevice.device(), &allocInfo, sets.data()) != VK_SUCCESS)
            throw std::runtime_error("JFA: failed to allocate descriptor sets");
        f.initSet   = sets[0];
        f.atobSet   = sets[1];
        f.btoaSet   = sets[2];
        f.outlineSet= sets[3];

        // Helper to write compute set: binding0=sampler input, binding1=storage output
        auto writeComputeSet = [&](VkDescriptorSet set,
                                   VkImageView inputView, VkImageLayout inputLayout,
                                   VkImageView outputView) {
            VkDescriptorImageInfo inInfo{};
            inInfo.sampler     = sampler;
            inInfo.imageView   = inputView;
            inInfo.imageLayout = inputLayout;

            VkDescriptorImageInfo outInfo{};
            outInfo.sampler     = VK_NULL_HANDLE;
            outInfo.imageView   = outputView;
            outInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet          = set;
            writes[0].dstBinding      = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].pImageInfo      = &inInfo;

            writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet          = set;
            writes[1].dstBinding      = 1;
            writes[1].descriptorCount = 1;
            writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writes[1].pImageInfo      = &outInfo;

            vkUpdateDescriptorSets(lotDevice.device(), static_cast<uint32_t>(writes.size()),
                                   writes.data(), 0, nullptr);
        };

        // initSet:  mask(SAMPLED) → jfa[0](GENERAL)
        writeComputeSet(f.initSet,
                        f.maskView,   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        f.jfaView[0]);

        // atobSet:  jfa[0](SAMPLED) → jfa[1](GENERAL)
        writeComputeSet(f.atobSet,
                        f.jfaView[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        f.jfaView[1]);

        // btoaSet:  jfa[1](SAMPLED) → jfa[0](GENERAL)
        writeComputeSet(f.btoaSet,
                        f.jfaView[1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        f.jfaView[0]);

        // outlineSet: reads the final JFA result
        VkDescriptorImageInfo resultInfo{};
        resultInfo.sampler     = sampler;
        resultInfo.imageView   = f.jfaView[finalJfaIdx];
        resultInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet outlineWrite{};
        outlineWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        outlineWrite.dstSet          = f.outlineSet;
        outlineWrite.dstBinding      = 0;
        outlineWrite.descriptorCount = 1;
        outlineWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        outlineWrite.pImageInfo      = &resultInfo;
        vkUpdateDescriptorSets(lotDevice.device(), 1, &outlineWrite, 0, nullptr);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Compute pipeline (JFA)
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createComputePipeline() {
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(JfaComputePush);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &computeDescLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pcRange;
    if (vkCreatePipelineLayout(lotDevice.device(), &layoutInfo, nullptr, &jfaComputeLayout) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create compute pipeline layout");

    auto code = readSpirvFile("shaders/jfa.comp.spv");
    VkShaderModuleCreateInfo smInfo{};
    smInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smInfo.codeSize = code.size();
    smInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(lotDevice.device(), &smInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create compute shader module");

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName  = "main";

    VkComputePipelineCreateInfo pipeInfo{};
    pipeInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeInfo.stage  = stageInfo;
    pipeInfo.layout = jfaComputeLayout;
    if (vkCreateComputePipelines(lotDevice.device(), VK_NULL_HANDLE, 1, &pipeInfo,
                                 nullptr, &jfaComputePipeline) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create compute pipeline");

    vkDestroyShaderModule(lotDevice.device(), shaderModule, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Selection mask graphics pipeline
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createSelectionMaskPipeline(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(MaskPushConstant);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &globalSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pcRange;
    if (vkCreatePipelineLayout(lotDevice.device(), &layoutInfo, nullptr, &maskPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create mask pipeline layout");

    PipelineConfigInfo config{};
    LotPipeline::defaultPipelineConfigInfo(config);
    config.renderPass      = selectionMaskRenderPass;
    config.pipelineLayout  = maskPipelineLayout;
    // No stencil needed for mask pass
    config.depthStencilInfo.stencilTestEnable = VK_FALSE;
    // Keep depth test enabled (only visible selected pixels)
    config.depthStencilInfo.depthTestEnable  = VK_TRUE;
    config.depthStencilInfo.depthWriteEnable = VK_TRUE;
    config.depthStencilInfo.depthCompareOp   = VK_COMPARE_OP_LESS;

    maskPipeline = std::make_unique<LotPipeline>(
        lotDevice,
        "shaders/selection_mask.vert.spv",
        "shaders/selection_mask.frag.spv",
        config);
}

// ─────────────────────────────────────────────────────────────────────────────
// Outline (fullscreen) graphics pipeline
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::createOutlinePipeline(VkRenderPass mainRenderPass) {
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(OutlinePush);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &outlineDescLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pcRange;
    if (vkCreatePipelineLayout(lotDevice.device(), &layoutInfo, nullptr, &outlinePipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("JFA: failed to create outline pipeline layout");

    PipelineConfigInfo config{};
    LotPipeline::defaultPipelineConfigInfo(config);
    config.renderPass     = mainRenderPass;
    config.pipelineLayout = outlinePipelineLayout;

    // No vertex input (fullscreen triangle from gl_VertexIndex)
    config.bindingDescription  = {};
    config.attributeDescription = {};

    // No depth test/write (draw on top of everything)
    config.depthStencilInfo.depthTestEnable   = VK_FALSE;
    config.depthStencilInfo.depthWriteEnable  = VK_FALSE;
    config.depthStencilInfo.stencilTestEnable = VK_FALSE;

    // Alpha blending for outline (fragments not matching outline use discard in shader)
    LotPipeline::enableAlphaBlending(config);

    outlinePipeline = std::make_unique<LotPipeline>(
        lotDevice,
        "shaders/fullscreen.vert.spv",
        "shaders/jfa_outline.frag.spv",
        config);
}

// ─────────────────────────────────────────────────────────────────────────────
// Image layout barrier helper
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::imageBarrier(VkCommandBuffer cmd, VkImage image,
                                   VkImageAspectFlags aspect,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = {aspect, 0, 1, 0, 1};
    barrier.srcAccessMask       = srcAccess;
    barrier.dstAccessMask       = dstAccess;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: renderSelectionMask
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::renderSelectionMask(VkCommandBuffer cmd, int frameIndex,
                                          FrameInfo& frameInfo) {
    auto& f = frames[frameIndex];

    // Begin selection mask renderpass
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass        = selectionMaskRenderPass;
    rpBegin.framebuffer       = f.maskFramebuffer;
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = extent;
    rpBegin.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    rpBegin.pClearValues      = clearValues.data();
    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport/scissor
    VkViewport viewport{0.f, 0.f, (float)extent.width, (float)extent.height, 0.f, 1.f};
    VkRect2D   scissor{{0,0}, extent};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    maskPipeline->bind(cmd);

    // Bind global descriptor set (projection/view)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             maskPipelineLayout, 0, 1,
                             &frameInfo.globalDescriptorSet, 0, nullptr);

    // Draw only selected objects
    for (auto& kv : frameInfo.Objects) {
        auto& obj = kv.second;
        if (!obj.isSelected || !obj.model) continue;

        MaskPushConstant push{};
        push.modelMatrix = obj.transform.mat4();
        vkCmdPushConstants(cmd, maskPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(MaskPushConstant), &push);
        obj.model->bind(cmd);
        obj.model->draw(cmd);
    }

    vkCmdEndRenderPass(cmd);
    // Note: renderpass finalLayout transitions mask to SHADER_READ_ONLY_OPTIMAL automatically
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: runJFA
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::runJFA(VkCommandBuffer cmd, int frameIndex) {
    auto& f = frames[frameIndex];

    // Transition both JFA images to GENERAL (storage write) for the first dispatch
    imageBarrier(cmd, f.jfaImage[0],
                 VK_IMAGE_ASPECT_COLOR_BIT,
                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                 0, VK_ACCESS_SHADER_WRITE_BIT,
                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    imageBarrier(cmd, f.jfaImage[1],
                 VK_IMAGE_ASPECT_COLOR_BIT,
                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                 0, VK_ACCESS_SHADER_WRITE_BIT,
                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, jfaComputePipeline);

    uint32_t groupX = (extent.width  + 7) / 8;
    uint32_t groupY = (extent.height + 7) / 8;

    // ── Init step (stepSize=0): selection mask → jfa[0] ──
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                jfaComputeLayout, 0, 1, &f.initSet, 0, nullptr);
        JfaComputePush push{};
        push.imageSize = glm::ivec2(extent.width, extent.height);
        push.stepSize  = 0;
        vkCmdPushConstants(cmd, jfaComputeLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(JfaComputePush), &push);
        vkCmdDispatch(cmd, groupX, groupY, 1);
    }

    // ── JFA steps: ping-pong between jfa[0] and jfa[1] ──
    // Start: jfa[0] has init result, transition to SHADER_READ_ONLY for sampling
    imageBarrier(cmd, f.jfaImage[0],
                 VK_IMAGE_ASPECT_COLOR_BIT,
                 VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                 VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    // step sizes: N/2, N/4, ..., 1
    int maxDim = static_cast<int>(std::max(extent.width, extent.height));
    int step   = maxDim / 2;
    int srcIdx = 0; // current source ping-pong index
    while (step >= 1) {
        int dstIdx = 1 - srcIdx;

        // dstIdx must be in GENERAL for writing
        // (first iteration: jfa[1] is already GENERAL from the transition above)
        // (subsequent: dst was last written and left as SHADER_READ_ONLY, need GENERAL)
        if (step < maxDim / 2) {
            imageBarrier(cmd, f.jfaImage[dstIdx],
                         VK_IMAGE_ASPECT_COLOR_BIT,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                         VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }

        VkDescriptorSet set = (srcIdx == 0) ? f.atobSet : f.btoaSet;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                jfaComputeLayout, 0, 1, &set, 0, nullptr);
        JfaComputePush push{};
        push.imageSize = glm::ivec2(extent.width, extent.height);
        push.stepSize  = step;
        vkCmdPushConstants(cmd, jfaComputeLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(JfaComputePush), &push);
        vkCmdDispatch(cmd, groupX, groupY, 1);

        // Transition dst (just written) to SHADER_READ_ONLY for next read
        imageBarrier(cmd, f.jfaImage[dstIdx],
                     VK_IMAGE_ASPECT_COLOR_BIT,
                     VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        srcIdx = dstIdx;
        step /= 2;
    }
    // Final result is in jfa[finalJfaIdx], already in SHADER_READ_ONLY_OPTIMAL
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: renderOutline
// ─────────────────────────────────────────────────────────────────────────────
void JFARenderSystem::renderOutline(VkCommandBuffer cmd, int frameIndex) {
    auto& f = frames[frameIndex];

    outlinePipeline->bind(cmd);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             outlinePipelineLayout, 0, 1,
                             &f.outlineSet, 0, nullptr);

    OutlinePush push{};
    push.resolution       = glm::vec2(extent.width, extent.height);
    push.outlineThickness = 3.0f; // pixels
    vkCmdPushConstants(cmd, outlinePipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(OutlinePush), &push);

    vkCmdDraw(cmd, 3, 1, 0, 0); // fullscreen triangle, no vertex buffer
}

} // namespace lot
