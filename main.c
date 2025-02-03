#include "pHash.h"
#include <stdio.h>

// Generate a simple gradient test image
static unsigned char* generate_test_image(int width, int height) {
    unsigned char* data = malloc(width * height * 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            data[idx] = (x * 255) / width;       // Red
            data[idx+1] = (y * 255) / height;    // Green
            data[idx+2] = 128;                   // Blue
        }
    }
    return data;
}

int main() {
    PhashError err;
    PhashImage *img1 = NULL, *img2 = NULL;
    uint64_t hash1, hash2;
    int distance;

    // Initialize library
    if ((err = phash_initialize()) != PHASH_OK) {
        printf("Initialization failed: %s\n", phash_error_string(err));
        return 1;
    }

    // Create test images
    const int W = 640, H = 480;
    unsigned char* image_data = generate_test_image(W, H);
    
    // Create image objects
    if ((err = phash_image_create(image_data, W, H, 3, 1, &img1)) != PHASH_OK) {
        printf("Image creation failed: %s\n", phash_error_string(err));
        free(image_data);
        return 1;
    }
    
    // Duplicate image for demonstration
    if ((err = phash_image_create(image_data, W, H, 3, 1, &img2)) != PHASH_OK) {
        printf("Image creation failed: %s\n", phash_error_string(err));
        phash_image_destroy(img1);
        return 1;
    }
    free(image_data);

    // Configure hashing parameters
    PhashConfig config = phash_config_default();
    config.dct_size = 32;
    config.hash_size = 8;
    config.colorspace = COLORSPACE_REC709;
    config.dct_method = DCT_METHOD_AUTO;

    // Compute hashes
    if ((err = phash_compute(img1, &config, &hash1)) != PHASH_OK) {
        printf("Hash computation failed: %s\n", phash_error_string(err));
        phash_image_destroy(img1);
        phash_image_destroy(img2);
        return 1;
    }

    if ((err = phash_compute(img2, &config, &hash2)) != PHASH_OK) {
        printf("Hash computation failed: %s\n", phash_error_string(err));
        phash_image_destroy(img1);
        phash_image_destroy(img2);
        return 1;
    }

    // Compare hashes
    if ((err = phash_compare(hash1, hash2, &distance)) != PHASH_OK) {
        printf("Comparison failed: %s\n", phash_error_string(err));
    } else {
        printf("Hamming distance: %d\n", distance);
        printf("Hash A: %016lx\n", hash1);
        printf("Hash B: %016lx\n", hash2);
        printf("Hashes are %s\n", distance <= 5 ? "similar" : "different");
    }

    // Cleanup
    phash_image_destroy(img1);
    phash_image_destroy(img2);
    phash_terminate();
    
    return 0;
}