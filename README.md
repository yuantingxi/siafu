<!--
SPDX-FileCopyrightText: 2023 C. J. Howard
SPDX-License-Identifier: CC0-1.0
-->

# Siafu

[![build](https://github.com/cjhoward/siafu/actions/workflows/build.yml/badge.svg)](https://github.com/cjhoward/siafu/actions/workflows/build.yml)
[![code quality](https://app.codacy.com/project/badge/Grade/23dc62d0303f4d20a8f15ec8d6a1eea2)](https://app.codacy.com/gh/cjhoward/siafu/dashboard)

Siafu is a tiny utility program for extracting isosurfaces from volumetric data. The program loads a 3D volume from a density file (one sample per line with `x`, `y`, `z`, and density values), extracts an isosurface using the marching cubes algorithm[^1], and outputs a model in `.ply`, `.obj`, or `.stl` format. Siafu is written in C++23 with zero dependencies.

## Table of Contents

-   [Install](#install)
-   [Usage](#usage)
-   [Contributing](#contributing)
-   [Authors](#authors)
-   [License](#license)

## Install

```bash
git clone https://github.com/cjhoward/siafu.git && cd siafu
cmake -B build
cmake --build build --config Release --target install
```

## Usage

```bash
usage: siafu [--version] [--help]
             <density_file> <isolevel> <output_file>
```

-   `density_file`: Path to a text file where each non-empty line contains four whitespace-separated numbers: `x`, `y`, `z`, and the corresponding density value.
-   `isolevel`: Threshold value for isosurface extraction.
-   `output_file`: Output file path and format. Supported file formats include `.ply`, `.obj`, and `.stl`. If the output file extension is unrecognized, the `.ply` format will be used.

### Options

-   `--version`: Display the version number.
-   `--help`: Display usage information.

### Examples

Load a volume from `data/ant.txt`, extract an isosurface at isolevel `500`, and save the isosurface as `ant.ply`:

```bash
siafu data/ant.txt 500 ant.ply
```

Load a volume from the density file `C:\beetle\samples.txt`, extract an isosurface at isolevel `123.4`, and save the isosurface as `beetle.obj`:

```bash
siafu C:\beetle\samples.txt 123.4 beetle.obj
```

## Contributing

Contributions are welcome! Feel free to [open an issue](https://github.com/cjhoward/siafu/issues) or [submit a pull request](https://github.com/cjhoward/siafu/pulls).

## Authors

-   [C. J. Howard](https://github.com/cjhoward)

## License

[![REUSE compliance](https://github.com/cjhoward/siafu/actions/workflows/reuse.yml/badge.svg)](https://github.com/cjhoward/siafu/actions/workflows/reuse.yml)

-   Siafu source code is licensed under [MIT](./LICENSES/MIT.txt).
-   Siafu documentation is licensed under [CC0-1.0](./LICENSES/CC0-1.0.txt).

[^1]: Bourke, P. (1994). Polygonising a scalar field. <https://paulbourke.net/geometry/polygonise/>
