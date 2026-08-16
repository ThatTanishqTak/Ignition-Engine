#pragma once

#include "Ignition/Renderer/Vertex.h"

#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace Ignition
{
	struct ModelSubmesh
	{
		std::string Name;

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		std::string AlbedoPath;
		int EmbeddedAlbedoIndex = -1;
		glm::vec4 Tint{ 1.0f };
		bool TwoSided = false;
	};

	struct EmbeddedTexture
	{
		std::vector<uint8_t> Data; // Compressed bytes exactly as stored in the model file
	};

	struct ModelData
	{
		std::vector<ModelSubmesh> Submeshes;
		std::vector<EmbeddedTexture> EmbeddedTextures;
	};

	bool LoadModelFile(const std::string& filepath, ModelData& outModel);
}