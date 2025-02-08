# pHash.c: Perceptual Hash Library for Image Comparison

The library presented here is a modern, flexible implementation of perceptual image hashing with several key improvements over existing solutions.

## Table of Contents

- [pHash.c: Perceptual Hash Library for Image Comparison](#phashc-perceptual-hash-library-for-image-comparison)
  - [Table of Contents](#table-of-contents)
  - [Core Architecture](#core-architecture)
  - [Advantages Over Existing Solutions](#advantages-over-existing-solutions)
  - [Performance Considerations](#performance-considerations)
  - [Build](#build)
  - [Usage](#usage)

## Core Architecture

The library implements perceptual hashing through DCT (Discrete Cosine Transform) with several unique features:

1. **Flexible DCT Sizes**: Unlike traditional implementations that are fixed to 8x8 or 32x32 matrices, this library supports configurable DCT sizes from 8x8 up to 64x64 through the `dct_size` parameter.

2. **Multiple Color Space Conversions**: The library offers five different color space conversion methods:
    - Standard luminosity (0.299R + 0.587G + 0.114B)
    - Simple averaging
    - Professional standards: Rec.601, Rec.709, and Rec.2100 for HDR content

3. **Optimized DCT Implementations**: The library provides multiple DCT computation methods:
    - Loeffler algorithm for 8x8 (fastest for small sizes)
    - AAN (Arai-Agui-Nakajima) for power-of-2 sizes
    - Lookup-table based approach
    - Auto-selection based on input size

## Advantages Over Existing Solutions

1. **Precision Control**: The `use_high_precision` option allows switching between float and double precision, trading speed for accuracy when needed.

2. **SIMD Support**: Built-in SIMD optimization support through the `enable_simd` flag, which can be toggled for performance testing.

3. **Memory Safety**: Clear ownership semantics with `owns_memory` flag in the image structure, preventing memory leaks.

4. **Error Handling**: Comprehensive error reporting system with detailed error codes and string descriptions.

## Performance Considerations

The library is designed with performance in mind, offering:
- Multiple DCT implementation choices
- SIMD optimizations
- Configurable precision levels
- Memory-efficient image handling

For developers looking to implement perceptual hashing in their projects, this library offers a more flexible and maintainable alternative to existing solutions, with modern features while maintaining a clean C89-compatible interface.

## Build 

```bash 
mkdir build && cd build

cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIB=ON \
    -DBUILD_EXECUTABLE=ON \
    -DBUILD_TESTS=ON

make && sudo make install
```

## Usage

```c
#include <stdio.h>
#include <pHash.h>

int main() {
    PhashError err;
    PhashImage *img = NULL;
    uint64_t hash;

    // Initialize library
    if ((err = phash_initialize()) != PHASH_OK) {
        printf("Error: %s\n", phash_error_string(err));
        return 1;
    }

    // Create a simple image (replace with your image loading code)
    const int width = 100, height = 100;
    unsigned char* data = malloc(width * height * 3);
    // Fill image data here...

    // Create image object
    if ((err = phash_image_create(data, width, height, 3, 1, &img)) != PHASH_OK) {
        printf("Error: %s\n", phash_error_string(err));
        free(data);
        return 1;
    }

    // Compute hash
    if ((err = phash_compute(img, NULL, &hash)) != PHASH_OK) {
        printf("Error: %s\n", phash_error_string(err));
    } else {
        printf("Hash: %016lx\n", hash);
    }

    // Cleanup
    free(data);
    phash_image_destroy(img);
    phash_terminate();
    return 0;
}
```

