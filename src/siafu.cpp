// SPDX-FileCopyrightText: 2023 C. J. Howard
// SPDX-License-Identifier: MIT

#include "siafu.hpp"
#include "config.hpp"
#include <algorithm>
#include <cstring>
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
	
	// Pad the volume with a one-voxel border so the extracted surface is watertight
	const std::size_t bytes_per_voxel = bits_per_voxel >> 3;
	if (bytes_per_voxel == 0)
	{
		std::cerr << "unsupported voxel format\n";
		return 1;
	}
	
	const u32 padded_w = volume_w + 2;
	const u32 padded_h = volume_h + 2;
	const u32 padded_d = volume_d + 2;
	const std::size_t padded_slice_size = static_cast<std::size_t>(padded_w) * padded_h * bytes_per_voxel;
	const std::size_t padded_volume_size = padded_slice_size * padded_d;
	
	auto padded_voxels = std::make_unique<std::byte[]>(padded_volume_size);
	std::fill_n(padded_voxels.get(), padded_volume_size, std::byte{0});
	
	const std::size_t slice_size_bytes = static_cast<std::size_t>(volume_w) * volume_h * bytes_per_voxel;
	for (u32 z = 0; z < volume_d; ++z)
	{
		const auto* src_slice = voxels.get() + slice_size_bytes * z;
		auto* dst_slice = padded_voxels.get() + padded_slice_size * (z + 1);
	
		for (u32 y = 0; y < volume_h; ++y)
		{
			const auto* src_row = src_slice + static_cast<std::size_t>(y) * volume_w * bytes_per_voxel;
			auto* dst_row = dst_slice + (static_cast<std::size_t>(y + 1) * padded_w + 1) * bytes_per_voxel;
			std::memcpy(dst_row, src_row, static_cast<std::size_t>(volume_w) * bytes_per_voxel);
		}
	}
	
	voxels = std::move(padded_voxels);
	volume_w = padded_w;
	volume_h = padded_h;
	volume_d = padded_d;
	
	std::cout << std::format("padded volume to {}x{}x{} voxels\n", volume_w, volume_h, volume_d);
	
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
