// SPDX-FileCopyrightText: 2023 C. J. Howard
// SPDX-License-Identifier: MIT

#include "siafu.hpp"
#include <cmath>
#include <cstring>
#include <memory>

namespace
{
	/// Packed X-, Y-, and Z-offsets to the cube vertices (`0bzzzzzzzzyyyyyyyyxxxxxxxx`).
	inline constexpr u32 cube_offsets = 0xf0cc66;
	
	/// Packed indices of the first vertex of each cube edge (0, 1, 3, 0, 4, 5, 7, 4, 0, 1, 2, 3).
	inline constexpr u64 edge_vertices_a = 0x321047540310;
	
	/// Packed indices of the second vertex of each cube edge (1, 2, 2, 3, 5, 6, 6, 7, 4, 5, 6, 7).
	inline constexpr u64 edge_vertices_b = 0x765476653221;
	
	/// Packed directions of each each cube edge (0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2).
	inline constexpr u32 edge_directions = 0xaa4444;
	
	/// Set of edges intersected by an isosurface for each cube configuration.
	static const u16 edge_table[256] =
	{
		0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
		0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
		0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
		0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
		0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c,
		0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
		0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac,
		0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
		0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c,
		0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
		0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc,
		0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
		0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c,
		0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
		0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc,
		0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
		0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
		0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
		0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
		0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
		0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
		0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
		0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460,
		0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
		0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0,
		0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
		0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230,
		0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
		0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190,
		0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
		0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
	};
	
	/// Packed set of triangles for each cube configuration.
	static const u64 triangle_table[256] =
	{
		0xffffffffffffffff,
		0xfffffffffffff380,
		0xfffffffffffff910,
		0xffffffffff189381,
		0xfffffffffffffa21,
		0xffffffffffa21380,
		0xffffffffff920a29,
		0xfffffff89a8a2382,
		0xfffffffffffff2b3,
		0xffffffffff0b82b0,
		0xffffffffffb32091,
		0xfffffffb89b912b1,
		0xffffffffff3ab1a3,
		0xfffffffab8a801a0,
		0xfffffff9ab9b3093,
		0xffffffffffb8aa89,
		0xfffffffffffff874,
		0xffffffffff437034,
		0xffffffffff748910,
		0xfffffff137174914,
		0xffffffffff748a21,
		0xfffffffa21403743,
		0xfffffff748209a29,
		0xffff4973727929a2,
		0xffffffffff2b3748,
		0xfffffff40242b74b,
		0xfffffffb32748109,
		0xffff1292b9b49b74,
		0xfffffff487ab31a3,
		0xffff4b7401b41ab1,
		0xffff30bab9b09874,
		0xfffffffab99b4b74,
		0xfffffffffffff459,
		0xffffffffff380459,
		0xffffffffff051450,
		0xfffffff513538458,
		0xffffffffff459a21,
		0xfffffff594a21803,
		0xfffffff204245a25,
		0xffff8434535235a2,
		0xffffffffffb32459,
		0xfffffff594b802b0,
		0xfffffffb32510450,
		0xffff584b82852512,
		0xfffffff45931ab3a,
		0xffffab81a8180594,
		0xffff30bab5b05045,
		0xfffffffb8aa85845,
		0xffffffffff975879,
		0xfffffff375359039,
		0xfffffff751710870,
		0xffffffffff753351,
		0xfffffff21a759879,
		0xffff37503505921a,
		0xffff25a758528208,
		0xfffffff7533525a2,
		0xfffffff2b3987597,
		0xffffb72029279759,
		0xffff751871810b32,
		0xfffffff51771b12b,
		0xffffb3a31a758859,
		0xf0aba010b7905075,
		0xf07570805a30b0ab,
		0xffffffffff5b75ab,
		0xfffffffffffff56a,
		0xffffffffff6a5380,
		0xffffffffff6a5109,
		0xfffffff6a5891381,
		0xffffffffff162561,
		0xfffffff803621561,
		0xfffffff620609569,
		0xffff823625285895,
		0xffffffffff56ab32,
		0xfffffff56a02b80b,
		0xfffffff6a5b32910,
		0xffffb892b92916a5,
		0xfffffff315356b36,
		0xffff6b51505b0b80,
		0xffff9505606306b3,
		0xfffffff89bb96956,
		0xffffffffff8746a5,
		0xfffffffa56374034,
		0xfffffff7486a5091,
		0xffff49737179156a,
		0xfffffff874156216,
		0xffff743403625521,
		0xffff620560509748,
		0xf962695923497937,
		0xfffffff56a4872b3,
		0xffffb720242746a5,
		0xffff6a5b32874910,
		0xf6a54b7b492b9129,
		0xffff6b51535b3748,
		0xfb404b7b016b5b15,
		0xf74836b630560950,
		0xffff9b7974b96956,
		0xffffffffffa4694a,
		0xfffffff380a946a4,
		0xfffffff04606a10a,
		0xffffa16468618138,
		0xfffffff462421941,
		0xffff462942921803,
		0xffffffffff624420,
		0xfffffff624428238,
		0xfffffff32b46a94a,
		0xffff6a4a94b82280,
		0xffffa164606102b3,
		0xf1b8b12184a16146,
		0xffff36b319639469,
		0xf14641916b0181b8,
		0xfffffff4600636b3,
		0xffffffffff86b846,
		0xfffffffa98a876a7,
		0xffffa76a907a0370,
		0xffff0818717a176a,
		0xfffffff37117a76a,
		0xffff768981861621,
		0xf937390976192962,
		0xfffffff206607087,
		0xffffffffff276237,
		0xffff76898a86ab32,
		0xf7a9a76790b72702,
		0xfb32a767a1871081,
		0xffff17616a71b12b,
		0xf63136b619768698,
		0xffffffffff76b190,
		0xffff06b0b3607087,
		0xfffffffffffff6b7,
		0xfffffffffffffb67,
		0xffffffffff67b803,
		0xffffffffff67b910,
		0xfffffff67b138918,
		0xffffffffff7b621a,
		0xfffffff7b6803a21,
		0xfffffff7b69a2092,
		0xffff89a38a3a27b6,
		0xffffffffff726327,
		0xfffffff026067807,
		0xfffffff910732672,
		0xffff678891681261,
		0xfffffff73171a67a,
		0xffff801781a7167a,
		0xffff7a69a0a70730,
		0xfffffff9a88a7a67,
		0xffffffffff68b486,
		0xfffffff640603b63,
		0xfffffff109648b68,
		0xffff63b139369649,
		0xfffffff1a28b6486,
		0xffff640b60b03a21,
		0xffff9a2920b648b4,
		0xf36463b34923a39a,
		0xfffffff264248328,
		0xffffffffff264240,
		0xffff834642432091,
		0xfffffff642241491,
		0xffff1a6648168318,
		0xfffffff40660a01a,
		0xf39a9303a6834364,
		0xffffffffff4a649a,
		0xffffffffffb67594,
		0xfffffff67b594380,
		0xfffffffb67045105,
		0xffff51345343867b,
		0xfffffffb6721a459,
		0xffff594380a217b6,
		0xffff204a24a45b67,
		0xf67b25a523453843,
		0xfffffff945267327,
		0xffff786260680459,
		0xffff045051673263,
		0xf851584812786826,
		0xffff73167161a459,
		0xf459078701671a61,
		0xfa737a6a305a4a04,
		0xffffa84a458a7a67,
		0xfffffff98b9b6596,
		0xffff590650360b63,
		0xffffb65510b508b0,
		0xfffffff1355363b6,
		0xffff65b8b9b59a21,
		0xfa21965690b603b0,
		0xf52025a50865b58b,
		0xffff35a3a25363b6,
		0xffff283265825985,
		0xfffffff260069659,
		0xf826283865081851,
		0xffffffffff612651,
		0xf698965683a61631,
		0xffff06505960a01a,
		0xffffffffffa65830,
		0xfffffffffffff65a,
		0xffffffffffb57a5b,
		0xfffffff03857ba5b,
		0xfffffff091ba57b5,
		0xffff1381897ba57a,
		0xfffffff15717b21b,
		0xffffb27571721380,
		0xffff7b2209729579,
		0xf289823295b27257,
		0xfffffff573532a52,
		0xffff52a578258028,
		0xffff2a37353a5109,
		0xf25752a278129289,
		0xffffffffff573531,
		0xfffffff571170780,
		0xfffffff735539309,
		0xffffffffff795789,
		0xfffffff8ba8a5485,
		0xffff03bba50b5405,
		0xffff54aba8a48910,
		0xf41314943b54a4ba,
		0xffff8548b2582152,
		0xfb151b2b543b0b40,
		0xf58b8545b2950520,
		0xffffffffff3b2549,
		0xffff483543253a52,
		0xfffffff0244252a5,
		0xf910854583a532a3,
		0xffff2492914252a5,
		0xfffffff153358548,
		0xffffffffff501540,
		0xffff530509358548,
		0xfffffffffffff549,
		0xfffffffba9b947b4,
		0xffffba97b9794380,
		0xffffb470414b1ba1,
		0xf4bab474a1843413,
		0xffff219b294b97b4,
		0xf3801b2b197b9479,
		0xfffffff04224b47b,
		0xffff42343824b47b,
		0xffff947732972a92,
		0xf70207872a4797a9,
		0xfa040a1a472a3a73,
		0xffffffffff4782a1,
		0xfffffff317714194,
		0xffff178180714194,
		0xffffffffff347304,
		0xfffffffffffff784,
		0xffffffffff8ba8a9,
		0xfffffffa9bb93903,
		0xfffffffba88a0a10,
		0xffffffffffa3ba13,
		0xfffffff8b99b1b21,
		0xffff9b2921b93903,
		0xffffffffffb08b20,
		0xfffffffffffffb23,
		0xfffffff98aa82832,
		0xffffffffff2902a9,
		0xffff8a1810a82832,
		0xfffffffffffff2a1,
		0xffffffffff819831,
		0xfffffffffffff190,
		0xfffffffffffff830,
		0xffffffffffffffff
	};
}

void polygonize
(
	f32 isolevel,
	const std::function<f32(u32, u32, u32)>& sample,
	u32 width,
	u32 height,
	u32 depth,
	std::vector<vertex>& vertices,
	std::vector<triangle>& triangles
)
{
	const u32vec3 max{std::max(width, 1u) - 1, std::max(height, 1u) - 1, std::max(depth, 1u) - 1};
	const u32 z_stride = width * height;
	
	f32vec3 scale;
	scale.x = 2.0f / std::max(max.x, std::max(max.y, max.z));
	scale.y = scale.x;
	scale.z = scale.x;
	f32vec3 translation = {-1.0f, -1.0f, -1.0f};
	
	// Index offset for cube vertices
	const u32 offsets[8] =
	{
		0,
		1,
		width + 1,
		width,
		z_stride,
		z_stride + 1,
		width * (height + 1) + 1,
		width * (height + 1),
	};
	
	// Allocate a cache to hold the indices of two Z-slices of vertices
	const u32 vertex_cache_capacity = z_stride * 2;
	const u32 vertex_cache_size = vertex_cache_capacity * 3;
	auto vertex_cache = std::make_unique<u32[]>(vertex_cache_size);
	std::memset(vertex_cache.get(), 0xff, vertex_cache_size * sizeof(u32));
	
	// Allocate a cache to hold 4 Z-slices of voxels
	const auto voxel_cache_size = z_stride * 4;
	auto voxel_cache = std::make_unique<f32[]>(voxel_cache_size);
	
	// Fetches a voxel from the voxel cache, given X-, Y-, and Z-coordinates.
	auto get_voxel = [&](u32 x, u32 y, u32 z) -> f32
	{
		return voxel_cache[x + width * (y + height * (z % 4))];
	};
	
	// Calculates a gradient from the voxel cache, given X-, Y-, and Z-coordinates.
	auto get_gradient = [&](u32 x, u32 y, u32 z) -> f32vec3
	{
		return
		{
			get_voxel(std::max(x, 1u) - 1, y, z) - get_voxel(std::min(x + 1, max.x), y, z),
			get_voxel(x, std::max(y, 1u) - 1, z) - get_voxel(x, std::min(y + 1, max.y), z),
			get_voxel(x, y, std::max(z, 1u) - 1) - get_voxel(x, y, std::min(z + 1, max.z))
		};
	};
	
	// Caches voxels in the given Z-slice.
	auto cache_z_slice = [&](u32 z)
	{
		f32* s = voxel_cache.get() + (z % 4) * z_stride;
		for (u32 y = 0; y < height; ++y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				*(s++) = sample(x, y, z);
			}
		}
	};
	
	// Cache voxels in the first two Z-slices
	cache_z_slice(0);
	cache_z_slice(1);
	
	// Init minimum Z-coordinate of cached vertices
	f32 min_cached_vertex_z = -std::numeric_limits<f32>::infinity();
	
	// Loop through the grid
	for (u32 z = 0; z < max.z; ++z)
	{
		// Cache voxels in Z-slice `z + 2`
		if (z + 2 < depth)
		{
			cache_z_slice(z + 2);
		}
		
		// Calculate Z-coordinates of the cube vertices
		u32vec3 cube_vertices[8];
		f32vec3 transformed_cube_vertices[8];
		for (u32 i = 0; i < 8; ++i)
		{
			cube_vertices[i].z = z + ((cube_offsets >> (i + 16)) & 1);
			transformed_cube_vertices[i].z = static_cast<f32>(cube_vertices[i].z) * scale.z + translation.z;
		}
		
		for (u32 y = 0; y < max.y; ++y)
		{
			// Calculate Y-coordinates of the cube vertices
			for (u32 i = 0; i < 8; ++i)
			{
				cube_vertices[i].y = y + ((cube_offsets >> (i + 8)) & 1);
				transformed_cube_vertices[i].y = static_cast<f32>(cube_vertices[i].y) * scale.y + translation.y;
			}
			
			for (u32 x = 0; x < max.x; ++x)
			{
				const auto base_vertex_index = x + width * (y + height * z);
				
				// Determine cube configuration
				u32 cube_config = 0;
				for (u32 i = 0; i < 8; ++i)
				{
					const auto value = voxel_cache[(base_vertex_index + offsets[i]) % voxel_cache_size];
					cube_config |= (value < isolevel) << i;
				}
				
				// Skip cubes not intersected by the isosurface
				const auto edge_case = edge_table[cube_config];
				if (!edge_case)
				{
					continue;
				}
				
				// For each cube edge
				u32 vertex_indices[12];
				for (u32 i = 0; i < 12; ++i)
				{
					// Disregard edges not intersected by the isosurface
					if (!(edge_case & (1 << i)))
					{
						continue;
					}
					
					// Determine indices of cube vertices that form the edge
					const u32 v1 = (edge_vertices_a >> (i << 2)) & 0b111;
					const u32 v2 = (edge_vertices_b >> (i << 2)) & 0b111;
					const auto v1_index = base_vertex_index + offsets[v1];
					const auto v2_index = base_vertex_index + offsets[v2];
					
					// Fetch cached edge vertex with edge key
					const auto edge_key = (v1_index % vertex_cache_capacity) * 3 + ((edge_directions >> (i << 1)) & 0b11);
					auto& cached_vertex = vertex_cache[edge_key];
					
					// Reuse cached edge vertex if not invalid or expired
					if (cached_vertex != ~u32{0} && vertices[cached_vertex].p.z >= min_cached_vertex_z)
					{
						vertex_indices[i] = cached_vertex;
						continue;
					}
					
					// Valid cached edge vertex not found, cache a new edge vertex
					cached_vertex = static_cast<u32>(vertices.size());
					vertex_indices[i] = cached_vertex;
					
					// Calculate X-coordinates of the cube vertices
					cube_vertices[v1].x = x + ((cube_offsets >> v1) & 1);
					cube_vertices[v2].x = x + ((cube_offsets >> v2) & 1);
					transformed_cube_vertices[v1].x = static_cast<f32>(cube_vertices[v1].x) * scale.x + translation.x;
					transformed_cube_vertices[v2].x = static_cast<f32>(cube_vertices[v2].x) * scale.x + translation.x;
					
					// Get transformed edge vertex positions
					const auto& p1 = transformed_cube_vertices[v1];
					const auto& p2 = transformed_cube_vertices[v2];
					
					// Fetch voxel values from voxel cache
					const auto voxel1 = voxel_cache[v1_index % voxel_cache_size];
					const auto voxel2 = voxel_cache[v2_index % voxel_cache_size];
					
					// Calculate interpolation factor between edge endpoints
					const f32 t = std::abs(voxel1 - voxel2) < 1e-6 ? 0.5f : (isolevel - voxel1) / (voxel2 - voxel1);
					
					// Construct new vertex between edge endpoints
					vertex v;
					v.p = {(p2.x - p1.x) * t + p1.x, (p2.y - p1.y) * t + p1.y, (p2.z - p1.z) * t + p1.z};
					
					// Interpolate between isofield gradients at edge endpoints
					const auto g1 = get_gradient(cube_vertices[v1].x, cube_vertices[v1].y, cube_vertices[v1].z);
					const auto g2 = get_gradient(cube_vertices[v2].x, cube_vertices[v2].y, cube_vertices[v2].z);
					const f32vec3 g = {(g2.x - g1.x) * t + g1.x, (g2.y - g1.y) * t + g1.y, (g2.z - g1.z) * t + g1.z};
					
					// Calculate vertex normal from normalized interpolated gradient
					const f32 sqr_gl = g.x * g.x + g.y * g.y + g.z * g.z;
					const f32 inv_gl = (sqr_gl > 1e-6f) ? 1.0f / std::sqrt(sqr_gl) : 0.0f;
					v.n = {g.x * inv_gl, g.y * inv_gl, g.z * inv_gl};
					
					// Add vertex to isosurface vertex list
					vertices.emplace_back(std::move(v));
				}
				
				// Generate triangles
				auto triangulation = triangle_table[cube_config];
				for (int i = 0; (triangulation & 0xf) != 0xf && i < 15; i += 3)
				{
					const auto a = vertex_indices[triangulation & 0xf];
					const auto b = vertex_indices[(triangulation >> 4) & 0xf];
					const auto c = vertex_indices[(triangulation >> 8) & 0xf];
					triangulation >>= 12;
					
					// If triangle is not degenerate
					if (a != b && a != c && b != c)
					{
						triangles.emplace_back(a, b, c);
					}
				}
			}
		}
		
		// Update minimum Z-coordinate of cached vertices
		min_cached_vertex_z = transformed_cube_vertices[7].z;
	}
}
