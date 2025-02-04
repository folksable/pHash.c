#include "pHash.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <image1_path> <image2_path>\n", argv[0]);
        return 1;
    }

    PhashError err;
    PhashImage *img1 = NULL, *img2 = NULL;
    uint64_t hash1, hash2;
    int distance;

    // Initialize library
    if ((err = phash_initialize()) != PHASH_OK) {
        printf("Initialization failed: %s\n", phash_error_string(err));
        return 1;
    }

    // Load images using stb_image
    int width1, height1, channels1;
    int width2, height2, channels2;
    unsigned char *image_data1 = stbi_load(argv[1], &width1, &height1, &channels1, 3);
    unsigned char *image_data2 = stbi_load(argv[2], &width2, &height2, &channels2, 3);

    if (!image_data1 || !image_data2) {
        printf("Failed to load images\n");
        if (image_data1) stbi_image_free(image_data1);
        if (image_data2) stbi_image_free(image_data2);
        return 1;
    }

    // Create image objects
    if ((err = phash_image_create(image_data1, width1, height1, 3, 1, &img1)) != PHASH_OK) {
        printf("Image creation failed: %s\n", phash_error_string(err));
        stbi_image_free(image_data1);
        stbi_image_free(image_data2);
        return 1;
    }

    if ((err = phash_image_create(image_data2, width2, height2, 3, 1, &img2)) != PHASH_OK) {
        printf("Image creation failed: %s\n", phash_error_string(err));
        phash_image_destroy(img1);
        stbi_image_free(image_data1);
        stbi_image_free(image_data2);
        return 1;
    }

    // Free the image data as it's no longer needed
    stbi_image_free(image_data1);
    stbi_image_free(image_data2);

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
        printf("Hash A: %016llx\n", hash1);
        printf("Hash B: %016llx\n", hash2);
        printf("Hashes are %s\n", distance <= 5 ? "similar" : "different");
    }

    // Cleanup
    phash_image_destroy(img1);
    phash_image_destroy(img2);
    phash_terminate();
    
    return 0;
}