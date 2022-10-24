#include "Texture.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include "../AstranEditorUI.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


//Teena - need to do this since nanosvg is fucked. use /FORCE:MULTIPLE in linker commandline section
//#ifndef ALLOWSVG
//#define ALLOWSVG
//#ifdef ALLOWSVG
//#define NANOSVG_ALL_COLOR_KEYWORDS	// Include full list of color keywords.
//#define NANOSVG_IMPLEMENTATION		// Expands implementation
//#include "nanosvg.h"
//#define NANOSVGRAST_IMPLEMENTATION
//#include "nanosvgrast.h"
//#endif // ALLOWSVG
//#endif


namespace Utils {

	static uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
	{
		VkPhysicalDeviceMemoryProperties prop;
		vkGetPhysicalDeviceMemoryProperties(AstranEditorUI::GetPhysicalDevice(), &prop);
		for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
		{
			if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
				return i;
		}

		return 0xffffffff;
	}

	static uint32_t BytesPerChannel(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGBA:    return 4;
		case ImageFormat::RGBA32F: return 32;
		}
		return 0;
	}

	static VkFormat WalnutFormatToVulkanFormat(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGBA:    return VK_FORMAT_R8G8B8A8_UNORM;
		case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		return (VkFormat)0;
	}

}



Texture::Texture(const char * path, float inScale) : Texture(path, TextureSourceType::RASTER, inScale, true)
{
	//Texture(path, TextureSourceType::RASTER, inScale, true);
}

Texture::Texture(const char * path, TextureSourceType type, float inScale, bool flipVertically)
{
	scale = inScale;
	if (type == TextureSourceType::RASTER)
	{
		LoadRasterImage(path, inScale, flipVertically);
	}
	else if (type == TextureSourceType::VECTOR)
	{
		LoadVectorImage(path, inScale);
	}
}

Texture::Texture(std::string& path, TextureSourceType type, float inScale, bool flipVertically)
	: Texture(path.c_str(), type, inScale, flipVertically)
{
}

Texture::~Texture()
{
	VkDevice device = AstranEditorUI::GetDevice();

	vkDestroySampler(device, m_Sampler, nullptr);
	vkDestroyImageView(device, m_ImageView, nullptr);
	vkDestroyImage(device, m_Image, nullptr);
	vkFreeMemory(device, m_Memory, nullptr);
	vkDestroyBuffer(device, m_StagingBuffer, nullptr);
	vkFreeMemory(device, m_StagingBufferMemory, nullptr);
}

void Texture::LoadRasterImage(const char * path, float inScale, bool flipVertically)
{
	// load image, create texture and generate mipmaps
	stbi_set_flip_vertically_on_load(flipVertically);
	unsigned char* data = stbi_load(path, &m_width, &m_height, &nrChannels, STBI_rgb_alpha); //0 - Auto or 4 - RGBA

	uint64_t size = m_width * m_height * 4;
	m_Format = ImageFormat::RGBA;

	std::cout << "m_width: " << m_width << "m_height: " + m_height;

	AllocateMemory(size);
	SetData(data);
	stbi_image_free(data);
}

void Texture::LoadVectorImage(const char * path, float inScale)
{
	/*
	NSVGrasterizer *rast = NULL;
	unsigned char* img = NULL;

	NSVGimage* image = nsvgParseFromFile(path, "px", 96.0f);
	if (image == NULL)
	{
		printf("Could not open SVG image.\n");
		goto error;
	}

	m_width = (int)image->width*inScale;
	m_height = (int)image->height*inScale;

	rast = nsvgCreateRasterizer();
	if (rast == NULL)
	{
		printf("Could not init rasterizer.\n");
		goto error;
	}

	img = (unsigned char*)malloc(m_width*m_height * 4);
	if (img == NULL)
	{
		printf("Could not alloc image buffer.\n");
		goto error;
	}

	nsvgRasterize(rast, image, 0, 0, inScale, img, m_width, m_height, m_width * 4);

	GenerateTexture(img);

	free((unsigned char*)img);

error:
	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	*/
}

void Texture::AllocateMemory(uint64_t size)
{
	VkDevice device = AstranEditorUI::GetDevice();

	VkResult err;

	VkFormat vulkanFormat = Utils::WalnutFormatToVulkanFormat(m_Format);

	// Create the Image
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = vulkanFormat;
		info.extent.width = m_width;
		info.extent.height = m_height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		err = vkCreateImage(device, &info, nullptr, &m_Image);
		check_vk_result(err);
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(device, m_Image, &req);
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;
		alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
		err = vkAllocateMemory(device, &alloc_info, nullptr, &m_Memory);
		check_vk_result(err);
		err = vkBindImageMemory(device, m_Image, m_Memory, 0);
		check_vk_result(err);
	}

	// Create the Image View:
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_Image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = vulkanFormat;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		err = vkCreateImageView(device, &info, nullptr, &m_ImageView);
		check_vk_result(err);
	}

	// Create sampler:
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = VK_FILTER_LINEAR;
		info.minFilter = VK_FILTER_LINEAR;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.minLod = -1000;
		info.maxLod = 1000;
		info.maxAnisotropy = 1.0f;
		VkResult err = vkCreateSampler(device, &info, nullptr, &m_Sampler);
		check_vk_result(err);
	}

	// Create the Descriptor Set:
	m_DescriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Texture::SetData(const void* data)
{
	VkDevice device = AstranEditorUI::GetDevice();

	size_t upload_size = m_width * m_height * Utils::BytesPerChannel(m_Format);

	VkResult err;

	if (!m_StagingBuffer)
	{
		// Create the Upload Buffer
		{
			VkBufferCreateInfo buffer_info = {};
			buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.size = upload_size;
			buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			err = vkCreateBuffer(device, &buffer_info, nullptr, &m_StagingBuffer);
			check_vk_result(err);
			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device, m_StagingBuffer, &req);
			m_AlignedSize = req.size;
			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = req.size;
			alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
			err = vkAllocateMemory(device, &alloc_info, nullptr, &m_StagingBufferMemory);
			check_vk_result(err);
			err = vkBindBufferMemory(device, m_StagingBuffer, m_StagingBufferMemory, 0);
			check_vk_result(err);
		}

	}

	// Upload to Buffer
	{
		char* map = NULL;
		err = vkMapMemory(device, m_StagingBufferMemory, 0, m_AlignedSize, 0, (void**)(&map));
		check_vk_result(err);
		memcpy(map, data, upload_size);
		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = m_StagingBufferMemory;
		range[0].size = m_AlignedSize;
		err = vkFlushMappedMemoryRanges(device, 1, range);
		check_vk_result(err);
		vkUnmapMemory(device, m_StagingBufferMemory);
	}


	// Copy to Image
	{
		VkCommandBuffer command_buffer = AstranEditorUI::GetCommandBuffer(true);

		VkImageMemoryBarrier copy_barrier = {};
		copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier.image = m_Image;
		copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_barrier.subresourceRange.levelCount = 1;
		copy_barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copy_barrier);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = m_width;
		region.imageExtent.height = m_height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(command_buffer, m_StagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier use_barrier = {};
		use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier.image = m_Image;
		use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		use_barrier.subresourceRange.levelCount = 1;
		use_barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);

		AstranEditorUI::FlushCommandBuffer(command_buffer);
	}
}
