// SPDX-FileCopyrightText: 2023 C. J. Howard
// SPDX-License-Identifier: MIT

#include "siafu.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iterator>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
        constexpr double coordinate_epsilon = 1e-6;

        struct density_sample
        {
                double x;
                double y;
                double z;
                f32 density;
        };

        bool is_blank_or_comment(const std::string& line)
        {
                const auto first_non_space = std::find_if_not
                (
                        line.begin(),
                        line.end(),
                        [](unsigned char ch) { return std::isspace(ch); }
                );

                return first_non_space == line.end() || *first_non_space == '#';
        }

        template <class Getter>
        std::vector<double> extract_axis_values(const std::vector<density_sample>& samples, Getter getter)
        {
                std::vector<double> axis_values;
                axis_values.reserve(samples.size());

                for (const auto& sample : samples)
                {
                        axis_values.push_back(getter(sample));
                }

                std::sort(axis_values.begin(), axis_values.end());
                const auto unique_end = std::unique
                (
                        axis_values.begin(),
                        axis_values.end(),
                        [](double a, double b)
                        {
                                return std::abs(a - b) <= coordinate_epsilon;
                        }
                );
                axis_values.erase(unique_end, axis_values.end());

                return axis_values;
        }

        u32 find_axis_index(double value, const std::vector<double>& axis_values)
        {
                const auto lower = std::lower_bound
                (
                        axis_values.begin(),
                        axis_values.end(),
                        value - coordinate_epsilon
                );

                for (auto it = lower; it != axis_values.end(); ++it)
                {
                        if (std::abs(*it - value) <= coordinate_epsilon)
                        {
                                return static_cast<u32>(std::distance(axis_values.begin(), it));
                        }

                        if (*it > value + coordinate_epsilon)
                        {
                                break;
                        }
                }

                throw std::runtime_error("density file contains coordinates that do not lie on a regular grid");
        }
}

std::unique_ptr<std::byte[]> load_volume(const fs::path& path, u32& width, u32& height, u32& depth, u32& bits_per_voxel)
{
        std::ifstream file(path);
        if (!file.is_open())
        {
                throw std::runtime_error("failed to open file");
        }

        std::vector<density_sample> samples;
        std::string line;
        while (std::getline(file, line))
        {
                if (is_blank_or_comment(line))
                {
                        continue;
                }

                std::istringstream stream(line);
                double x, y, z, density;
                if (!(stream >> x >> y >> z >> density))
                {
                        throw std::runtime_error("failed to parse density file");
                }

                samples.push_back({x, y, z, static_cast<f32>(density)});
        }

        if (samples.empty())
        {
                throw std::runtime_error("density file is empty");
        }

        const auto axis_x = extract_axis_values(samples, [](const density_sample& sample) { return sample.x; });
        const auto axis_y = extract_axis_values(samples, [](const density_sample& sample) { return sample.y; });
        const auto axis_z = extract_axis_values(samples, [](const density_sample& sample) { return sample.z; });

        width = static_cast<u32>(axis_x.size());
        height = static_cast<u32>(axis_y.size());
        depth = static_cast<u32>(axis_z.size());

        if (!width || !height || !depth)
        {
                throw std::runtime_error("density file has invalid dimensions");
        }

        const std::size_t total_voxels = static_cast<std::size_t>(width) * height * depth;
        if (samples.size() != total_voxels)
        {
                throw std::runtime_error("density file does not describe a complete grid");
        }

        bits_per_voxel = 32;

        auto voxels = std::make_unique<std::byte[]>(total_voxels * sizeof(f32));
        auto* density_values = reinterpret_cast<f32*>(voxels.get());
        std::fill(density_values, density_values + total_voxels, 0.0f);

        std::vector<bool> assigned(total_voxels, false);
        for (const auto& sample : samples)
        {
                const auto x_index = find_axis_index(sample.x, axis_x);
                const auto y_index = find_axis_index(sample.y, axis_y);
                const auto z_index = find_axis_index(sample.z, axis_z);

                const std::size_t index = x_index + static_cast<std::size_t>(width) * (y_index + static_cast<std::size_t>(height) * z_index);

                if (assigned[index])
                {
                        throw std::runtime_error("density file contains duplicate coordinates");
                }

                assigned[index] = true;
                density_values[index] = sample.density;
        }

        if (!std::ranges::all_of(assigned, [](bool value) { return value; }))
        {
                throw std::runtime_error("density file is missing samples for some grid positions");
        }

        return voxels;
}
