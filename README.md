# pHash.c
a perceptual image dct hashing library to compare image similarity

## Usage

```c
#include "pHash.h"
#include <stdio.h>

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

## Build 

```bash 
mkdir build && cd build

# CMake with explicit source path
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# For Apple Silicon
cmake .. \
  -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
  -DCMAKE_OSX_ARCHITECTURES=arm64

make && sudo make install
```