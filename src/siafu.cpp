// SPDX-FileCopyrightText: 2023 C. J. Howard
// SPDX-License-Identifier: MIT

#include "siafu.hpp"
#include "config.hpp"
#include <fstream>
#include <iostream>
#include <format>
#include <stdexcept>
#include <string_view>

int main(int argc, char* argv[])
{
	// Parse options
	if (argc > 1)
	{
		std::string_view option(argv[1]);
		
		if (option == "--version")
		{
			std::cout << siafu_version_string << std::endl;
			return 0;
		}
		else if (option == "--help")
		{
			std::cout << siafu_help_string << std::endl;
			return 0;
		}
	}
	
	// Incorrect usage
	if (argc != 4)
	{
		std::cerr << siafu_help_string << std::endl;
		return 1;
	}
	
	// Parse isolevel parameter
	char* endptr;
	f32 isolevel = std::strtof(argv[2], &endptr);
	if (*endptr != '\0')
	{
		// NaN
		std::cerr << siafu_help_string << std::endl;
		return 1;
	}
	
	// Load volume
	u32 volume_w, volume_h, volume_d, bits_per_voxel;
	std::unique_ptr<std::byte[]> voxels;
	try
	{
		voxels = load_volume(argv[1], volume_w, volume_h, volume_d, bits_per_voxel);
	}
	catch (const std::exception& e)
	{
		std::cerr << std::format("failed to load volume: {}\n", e.what());
		return 1;
	}
	std::cout << std::format("loaded volume ({}x{}x{}@{}bpv)\n", volume_w, volume_h, volume_d, bits_per_voxel);
	
	// Select sampling function
	std::function<f32(u32, u32, u32)> sample;
        if (bits_per_voxel == 8)
        {
                sample = [=, vu8 = reinterpret_cast<const u8*>(voxels.get())](u32 x, u32 y, u32 z) -> f32
                {
                        return vu8[x + volume_w * (y + volume_h * z)];
                };
        }
        else if (bits_per_voxel == 16)
        {
                sample = [=, vu16 = reinterpret_cast<const u16*>(voxels.get())](u32 x, u32 y, u32 z) -> f32
                {
                        return vu16[x + volume_w * (y + volume_h * z)];
                };
        }
        else if (bits_per_voxel == 32)
        {
                sample = [=, vf32 = reinterpret_cast<const f32*>(voxels.get())](u32 x, u32 y, u32 z) -> f32
                {
                        return vf32[x + volume_w * (y + volume_h * z)];
                };
        }
        else
        {
                std::cerr << "unsupported voxel size" << std::endl;
                return 1;
        }
	
	// Extract isosurface
	std::vector<vertex> vertices;
	std::vector<triangle> triangles;
	try
	{
		polygonize(isolevel, sample, volume_w, volume_h, volume_d, vertices, triangles);
	}
	catch (const std::exception& e)
	{
		std::cerr << std::format("failed to extract isosurface: {}\n", e.what());
		return 1;
	}
	std::cout << std::format("extracted isosurface ({} triangles, {} vertices)\n", triangles.size(), vertices.size());
	
	// Save isosurface
	fs::path file_path(argv[3]);
	try
	{
		std::ofstream file(file_path, std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open output file");
		}
		
		// Output isosurface
		if (file_path.extension() == ".obj")
		{
			write_obj(file, vertices, triangles);
		}
		else if (file_path.extension() == ".stl")
		{
			write_stl(file, vertices, triangles);
		}
		else
		{
			write_ply(file, vertices, triangles);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << std::format("failed to save isosurface: {}\n", e.what());
		return 1;
	}
	std::cout << std::format("saved isosurface to {}\n", file_path.string());
	
	return 0;
}
