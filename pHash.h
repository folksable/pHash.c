#ifndef PHASH_H
#define PHASH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Version information
#define PHASH_VERSION_MAJOR 1
#define PHASH_VERSION_MINOR 0
#define PHASH_VERSION_PATCH 0

// Error codes
typedef enum {
    PHASH_OK = 0,
    PHASH_ERR_NULL_POINTER,
    PHASH_ERR_INVALID_ARGUMENT,
    PHASH_ERR_MEMORY_ALLOCATION,
    PHASH_ERR_UNSUPPORTED_OPERATION,
    PHASH_ERR_DOMAIN
} PhashError;

typedef enum {
    COLORSPACE_LUMINOSITY,  // 0.299R + 0.587G + 0.114B
    COLORSPACE_AVERAGE,     // (R + G + B) / 3
    COLORSPACE_REC601,      // ITU-R BT.601 (SDTV)
    COLORSPACE_REC709,      // ITU-R BT.709 (HDTV)
    COLORSPACE_REC2100      // ITU-R BT.2100 (HDR)
} ColorSpaceConversion;

typedef enum {
    DCT_METHOD_AUTO,      // Automatically choose best method
    DCT_METHOD_NAIVE,     // Basic implementation
    DCT_METHOD_LOEFFLER,  // Loeffler algorithm (8x8 only)
    DCT_METHOD_LOOKUP,    // Lookup table-based
    DCT_METHOD_AAN        // Arai-Agui-Nakajima (8/16/32/64 sizes)
} DCTMethod;

// Configuration parameters
typedef struct {
    int dct_size;          // Must be power of 2 between 8 and 64
    int hash_size;         // Must be <= dct_size (typical 8-32)
    bool use_high_precision; // Use double precision for calculations
    bool enable_simd;      // Allow SIMD optimizations when available
    ColorSpaceConversion colorspace;
    DCTMethod dct_method;
} PhashConfig;

// Image representation
typedef struct {
    const unsigned char* data; // Pixel data in RGB format
    int width;
    int height;
    int channels;         // 3 for RGB, 4 for RGBA
    bool owns_memory;     // If true, data will be freed on destruction
} PhashImage;

// Core functions
PhashError phash_compute(const PhashImage* image, 
                        const PhashConfig* config,
                        uint64_t* out_hash);

PhashError phash_compare(uint64_t hash_a, 
                        uint64_t hash_b,
                        int* out_distance);

// Utility functions
PhashError phash_image_create(const unsigned char* data,
                             int width, int height, int channels,
                             bool copy_data, PhashImage** out_image);

void phash_image_destroy(PhashImage* image);

const char* phash_error_string(PhashError error);

// Configuration management
PhashError phash_config_validate(const PhashConfig* config);
PhashConfig phash_config_default(void);

// Library initialization/cleanup
PhashError phash_initialize(void);
void phash_terminate(void);

#ifdef __cplusplus
}
#endif

#endif /* PHASH_H */