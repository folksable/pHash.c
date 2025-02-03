# pHash.c
a perceptual image dct hashing library to compare image similarity

## Build 

```bash 
mkdir build && cd build

# CMake with explicit source path
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# For Apple Silicon
cmake .. \
  -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
  -DCMAKE_OSX_ARCHITECTURES=arm64
```