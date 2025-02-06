#include <assert.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "pHash.h"

// Mock image data for testing
static unsigned char test_image_data[] = {
    255, 0, 0,    0, 255, 0,    0, 0, 255,    // RGB pixels
    0, 0, 0,      255, 255, 255, 128, 128, 128 // More pixels
};

void test_initialization() {
    PhashError err = phash_initialize();
    assert(err == PHASH_OK);
    printf("✓ Initialization test passed\n");
}

void test_image_creation() {
    PhashImage* img = NULL;
    PhashError err = phash_image_create(test_image_data, 3, 2, 3, 1, &img);
    
    assert(err == PHASH_OK);
    assert(img != NULL);
    assert(img->width == 3);
    assert(img->height == 2);
    assert(img->channels == 3);
    assert(img->owns_memory == 1);
    
    phash_image_destroy(img);
    printf("✓ Image creation test passed\n");
}

void test_config_validation() {
    PhashConfig config = phash_config_default();
    
    // Test valid configuration
    PhashError err = phash_config_validate(&config);
    assert(err == PHASH_OK);
    
    // Test invalid configurations
    config.dct_size = 7; // Not power of 2
    err = phash_config_validate(&config);
    assert(err == PHASH_ERR_INVALID_ARGUMENT);
    
    config = phash_config_default();
    config.hash_size = config.dct_size + 1; // Too large
    err = phash_config_validate(&config);
    assert(err == PHASH_ERR_INVALID_ARGUMENT);
    
    printf("✓ Configuration validation test passed\n");
}

void test_hash_computation() {
    PhashImage* img = NULL;
    uint64_t hash;
    PhashConfig config = phash_config_default();
    
    PhashError err = phash_image_create(test_image_data, 3, 2, 3, 1, &img);
    assert(err == PHASH_OK);
    
    err = phash_compute(img, &config, &hash);
    assert(err == PHASH_OK);
    assert(hash != 0); // Hash should not be zero for non-zero image
    
    phash_image_destroy(img);
    printf("✓ Hash computation test passed\n");
}

void test_hash_comparison() {
    uint64_t hash1 = 0x1234567890ABCDEF;
    uint64_t hash2 = 0x1234567890ABCDEF;
    uint64_t hash3 = 0xFFFFFFFFFFFFFFFF;
    int distance;
    
    PhashError err = phash_compare(hash1, hash2, &distance);
    assert(err == PHASH_OK);
    assert(distance == 0); // Identical hashes
    
    err = phash_compare(hash1, hash3, &distance);
    assert(err == PHASH_OK);
    assert(distance > 0); // Different hashes
    
    printf("✓ Hash comparison test passed\n");
}

void test_error_handling() {
    assert(strcmp(phash_error_string(PHASH_OK), "Success") == 0);
    assert(phash_error_string(PHASH_ERR_NULL_POINTER) != NULL);
    printf("✓ Error handling test passed\n");
}

int main() {
    printf("Running pHash library tests...\n\n");
    
    test_initialization();
    test_image_creation();
    test_config_validation();
    test_hash_computation();
    test_hash_comparison();
    test_error_handling();
    
    phash_terminate();
    printf("\nAll tests passed successfully!\n");
    return 0;
}