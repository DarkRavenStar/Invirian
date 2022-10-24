#pragma once
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <string>


enum class ImageFormat
{
	None = 0,
	RGBA,
	RGBA32F
};

class Texture
{
public:

	int m_width = -1;
	int m_height = -1;
	int nrChannels = 1;
	//unsigned int m_textureID;
	float scale = 1;

	enum class TextureSourceType
	{
		VECTOR,
		RASTER
	};

	Texture(const char* path, float inScale = 1);
	Texture(const char* path, TextureSourceType type, float inScale = 1, bool flipVertically = true);
	Texture(std::string& path, TextureSourceType type, float inScale = 1, bool flipVertically = true);
	~Texture();

	void SetData(const void* data);

	VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

private:
	void LoadRasterImage(const char* path, float inScale = 1, bool flipVertically = true);
	void LoadVectorImage(const char* path, float inScale = 1);

	void AllocateMemory(uint64_t size);

	VkImage m_Image = nullptr;
	VkImageView m_ImageView = nullptr;
	VkDeviceMemory m_Memory = nullptr;
	VkSampler m_Sampler = nullptr;

	ImageFormat m_Format = ImageFormat::None;

	VkBuffer m_StagingBuffer = nullptr;
	VkDeviceMemory m_StagingBufferMemory = nullptr;

	size_t m_AlignedSize = 0;

	VkDescriptorSet m_DescriptorSet = nullptr;
};
