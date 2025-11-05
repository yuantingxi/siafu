// SPDX-FileCopyrightText: 2023 C. J. Howard
// SPDX-License-Identifier: MIT

#ifndef SIAFU_HPP
#define SIAFU_HPP

#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <vector>

namespace fs = std::filesystem;

/// Sized types. @{
using i8 = std::int8_t;
using u8 = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
/// @}

/// Swaps the byte order of a floating-point value.
/// @{
[[nodiscard]] inline constexpr f32 byteswapf(f32 x) noexcept
{
	return std::bit_cast<f32>(std::byteswap(std::bit_cast<u32>(x)));
}
[[nodiscard]] inline constexpr f64 byteswapf(f64 x) noexcept
{
	return std::bit_cast<f64>(std::byteswap(std::bit_cast<u64>(x)));
}
/// @}

/// 3D vector.
template <class T>
struct vec3
{
	T x, y, z;
};

/// Size vector types @{
using f32vec3 = vec3<f32>;
using u32vec3 = vec3<u32>;
/// @}

/// Isosurface vertex.
struct vertex
{
	/// Vertex position.
	f32vec3 p;
	
	/// Vertex normal.
	f32vec3 n;
};

/// Isosurface triangle.
struct triangle
{
	/// Indices of triangle vertices.
	u32 a, b, c;
};

/**
 * Extracts an isosurface from a scalar field.
 *
 * @param[in] isolevel Isosurface threshold value.
 * @param[in] sample Scalar field sampling function.
 * @param[in] width X-axis sampling resolution.
 * @param[in] height Y-axis sampling resolution.
 * @param[in] depth Z-axis sampling resolution.
 * @param[out] vertices Isosurface vertex list.
 * @param[out] triangles Isosurface triangle list.
 *
 * @see Bourke, P. (1994). Polygonising a scalar field.
 */
void polygonize
(
	f32 isolevel,
	const std::function<f32(u32, u32, u32)>& sample,
	u32 width,
	u32 height,
	u32 depth,
	std::vector<vertex>& vertices,
	std::vector<triangle>& triangles
);

/**
 * Loads a 3D volume from a density file.
 *
 * @param[in] path Path to the density file.
 * @param[out] width Volume width, in voxels.
 * @param[out] height Volume height, in voxels.
 * @param[out] depth Volume depth, in voxels.
 * @param[out] bits_per_voxel Voxel size, in bits.
 *
 * @return Voxel data.
 */
[[nodiscard]] std::unique_ptr<std::byte[]> load_volume
(
	const fs::path& path,
	u32& width,
	u32& height,
	u32& depth,
	u32& bits_per_voxel
);

/**
 * Writes a model to a file.
 *
 * @param[out] file Output file.
 * @param[in] vertices Vertex list.
 * @param[in] triangles Triangle list.
 */
/// @{
void write_obj(std::ostream& file, std::span<const vertex> vertices, std::span<const triangle> triangles);
void write_ply(std::ostream& file, std::span<const vertex> vertices, std::span<const triangle> triangles);
void write_stl(std::ostream& file, std::span<const vertex> vertices, std::span<const triangle> triangles);
/// @}

#endif // SIAFU_HPP
