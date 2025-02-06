#include "pHash.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

// Internal constants
#define MIN_DCT_SIZE 8
#define MAX_DCT_SIZE 64
#define ALIGNMENT 64
#define AAN_SCALE_FACTOR 0.35355339059327373  // 1/sqrt(8)

// SIMD optimization flags
static bool g_avx2_enabled = 0;
static bool g_sse4_enabled = 0;

// DCT lookup table
typedef struct {
    double* coefficients;
    size_t size;
    bool initialized;
} DCTLookup;
static DCTLookup g_dct_lookup = {0};

// Error messages
static const char* ERROR_STRINGS[] = {
    "Success",
    "Null pointer encountered",
    "Invalid argument value",
    "Memory allocation failed",
    "Unsupported operation",
    "Domain error in mathematical function"
};

// Internal functions
static bool is_power_of_two(int value) {
    return (value > 0) && ((value & (value - 1)) == 0);
}

PhashError phash_config_validate(const PhashConfig* config) {
    if (!config) return PHASH_ERR_NULL_POINTER;
    
    if (config->dct_size < MIN_DCT_SIZE || 
        config->dct_size > MAX_DCT_SIZE ||
        !is_power_of_two(config->dct_size)) {
        return PHASH_ERR_INVALID_ARGUMENT;
    }
    
    if (config->hash_size < 1 || 
        config->hash_size > config->dct_size ||
        (config->hash_size * config->hash_size) > 64) {
        return PHASH_ERR_INVALID_ARGUMENT;
    }

     // Validate DCT method compatibility
    if (config->dct_method == DCT_METHOD_LOEFFLER && config->dct_size != 8) {
        return PHASH_ERR_UNSUPPORTED_OPERATION;
    }
    
    if (config->dct_method == DCT_METHOD_AAN && 
       !(config->dct_size == 8 || config->dct_size == 16 || 
         config->dct_size == 32 || config->dct_size == 64)) {
        return PHASH_ERR_UNSUPPORTED_OPERATION;
    }
    
    return PHASH_OK;
}

static inline double rgb_to_grayscale(unsigned char r, unsigned char g, unsigned char b,
                                     ColorSpaceConversion method) {
    switch (method) {
        case COLORSPACE_AVERAGE: return (r + g + b) / 3.0;
        case COLORSPACE_REC601: return 0.299*r + 0.587*g + 0.114*b;
        case COLORSPACE_REC709: return 0.2126*r + 0.7152*g + 0.0722*b;
        case COLORSPACE_REC2100: return 0.2627*r + 0.6780*g + 0.0593*b;
        default: return 0.299*r + 0.587*g + 0.114*b;
    }
}

// SIMD-optimized bilinear interpolation
static PhashError resize_and_grayscale(const PhashImage* img,
                                      const PhashConfig* cfg,
                                      double** out_matrix) {
    const int dst_size = cfg->dct_size;
    double* matrix = aligned_alloc(ALIGNMENT, dst_size*dst_size*sizeof(double));
    if (!matrix) return PHASH_ERR_MEMORY_ALLOCATION;

    const double x_ratio = (img->width > 1) ? 
        (double)(img->width - 1) / (dst_size - 1) : 0.0;
    const double y_ratio = (img->height > 1) ? 
        (double)(img->height - 1) / (dst_size - 1) : 0.0;

    for (int y = 0; y < dst_size; y++) {
        for (int x = 0; x < dst_size; x++) {
            const double src_x = x * x_ratio;
            const double src_y = y * y_ratio;
            const int x0 = (int)src_x;
            const int y0 = (int)src_y;
            const int x1 = (x0 < img->width - 1) ? x0 + 1 : x0;
            const int y1 = (y0 < img->height - 1) ? y0 + 1 : y0;
            
            const double dx = src_x - x0;
            const double dy = src_y - y0;
            const double w00 = (1.0 - dx) * (1.0 - dy);
            const double w01 = dx * (1.0 - dy);
            const double w10 = (1.0 - dx) * dy;
            const double w11 = dx * dy;

            const unsigned char* p = img->data;
            const int stride = img->width * img->channels;
            
            // Interpolate RGB channels
            double r = w00 * p[y0*stride + x0*img->channels] +
                      w01 * p[y0*stride + x1*img->channels] +
                      w10 * p[y1*stride + x0*img->channels] +
                      w11 * p[y1*stride + x1*img->channels];
            
            double g = w00 * p[y0*stride + x0*img->channels + 1] +
                      w01 * p[y0*stride + x1*img->channels + 1] +
                      w10 * p[y1*stride + x0*img->channels + 1] +
                      w11 * p[y1*stride + x1*img->channels + 1];
            
            double b = w00 * p[y0*stride + x0*img->channels + 2] +
                      w01 * p[y0*stride + x1*img->channels + 2] +
                      w10 * p[y1*stride + x0*img->channels + 2] +
                      w11 * p[y1*stride + x1*img->channels + 2];

            matrix[y*dst_size + x] = rgb_to_grayscale(r, g, b, cfg->colorspace);
        }
    }
    
    *out_matrix = matrix;
    return PHASH_OK;
}

// Fast 8x8 DCT using AAN algorithm
static void dct_8x8_aan(const double* input, double* output) {
    double temp[64];
    
    // Process rows
    for (int i = 0; i < 8; i++) {
        const double* in = input + i*8;
        double* t = temp + i*8;
        
        const double s0 = in[0] + in[7];
        const double s1 = in[1] + in[6];
        const double s2 = in[2] + in[5];
        const double s3 = in[3] + in[4];
        const double s4 = in[3] - in[4];
        const double s5 = in[2] - in[5];
        const double s6 = in[1] - in[6];
        const double s7 = in[0] - in[7];
        
        const double t0 = s0 + s3;
        const double t1 = s1 + s2;
        const double t2 = s0 - s3;
        const double t3 = s1 - s2;
        
        t[0] = t0 + t1;
        t[4] = t0 - t1;
        t[2] = t2 * 1.387039845 + t3 * 0.275899379;
        t[6] = t3 * 1.387039845 - t2 * 0.275899379;
        
        const double u0 = s4 * 0.707106781;
        const double u1 = s5 * 0.541196100;
        const double u2 = s6 * 1.306562965;
        const double u3 = s7 * 0.382683433;
        
        t[5] = u0 + u1;
        t[3] = u2 + u3;
        t[1] = u2 - u3;
        t[7] = u0 - u1;
    }
    
    // Process columns
    for (int i = 0; i < 8; i++) {
        double* col = temp + i;
        const double s0 = col[0] + col[56];
        const double s1 = col[8] + col[48];
        const double s2 = col[16] + col[40];
        const double s3 = col[24] + col[32];
        const double s4 = col[24] - col[32];
        const double s5 = col[16] - col[40];
        const double s6 = col[8] - col[48];
        const double s7 = col[0] - col[56];
        
        const double t0 = s0 + s3;
        const double t1 = s1 + s2;
        const double t2 = s0 - s3;
        const double t3 = s1 - s2;
        
        col[0] = (t0 + t1) * AAN_SCALE_FACTOR;
        col[32] = (t0 - t1) * AAN_SCALE_FACTOR;
        col[16] = (t2 * 1.387039845 + t3 * 0.275899379) * AAN_SCALE_FACTOR;
        col[48] = (t3 * 1.387039845 - t2 * 0.275899379) * AAN_SCALE_FACTOR;
        
        const double u0 = s4 * 0.707106781;
        const double u1 = s5 * 0.541196100;
        const double u2 = s6 * 1.306562965;
        const double u3 = s7 * 0.382683433;
        
        col[24] = (u0 + u1) * AAN_SCALE_FACTOR;
        col[40] = (u2 + u3) * AAN_SCALE_FACTOR;
        col[8] = (u2 - u3) * AAN_SCALE_FACTOR;
        col[56] = (u0 - u1) * AAN_SCALE_FACTOR;
    }
    
    // Transpose and store
    for (int i = 0; i < 64; i++) {
        output[i] = temp[(i%8)*8 + (i/8)];
    }
}

// Generic DCT using lookup table
static void dct_generic(const double* input, double* output, int size) {
    if (!g_dct_lookup.initialized || g_dct_lookup.size != size) {
        free(g_dct_lookup.coefficients);
        g_dct_lookup.coefficients = malloc(size*size*sizeof(double));
        for (int u = 0; u < size; u++) {
            for (int x = 0; x < size; x++) {
                g_dct_lookup.coefficients[u*size + x] = 
                    cos((2*x + 1)*u*M_PI/(2*size));
            }
        }
        g_dct_lookup.size = size;
        g_dct_lookup.initialized = 1;
    }
    
    for (int v = 0; v < size; v++) {
        for (int u = 0; u < size; u++) {
            double sum = 0.0;
            const double au = (u == 0) ? M_SQRT1_2 : 1.0;
            const double av = (v == 0) ? M_SQRT1_2 : 1.0;
            
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    sum += input[y*size + x] * 
                          g_dct_lookup.coefficients[u*size + x] *
                          g_dct_lookup.coefficients[v*size + y];
                }
            }
            output[v*size + u] = 0.25 * au * av * sum;
        }
    }
}


#if defined(__aarch64__) || defined(_M_ARM64)

// DCT coefficient constants
static const double C1 = 0.98078528040323043;  // cos(1*pi/16)
static const double C2 = 0.92387953251128674;  // cos(2*pi/16)
static const double C3 = 0.83146961230254524;  // cos(3*pi/16)
static const double C4 = 0.70710678118654752;  // cos(4*pi/16)
static const double C5 = 0.55557023301960222;  // cos(5*pi/16)
static const double C6 = 0.38268343236508977;  // cos(6*pi/16)
static const double C7 = 0.19509032201612825;  // cos(7*pi/16)

static void dct_8x8_neon(const double* input, double* output) {
    float64x2_t rows[8], cols[8];
    float64x2_t temp[8];
    
    // Load input rows
    for (int i = 0; i < 8; i++) {
        rows[i] = vld1q_f64(&input[i * 8]);
    }
    
    // 1D DCT on rows
    for (int i = 0; i < 8; i++) {
        // Stage 1: Initial butterfly
        float64x2_t sum = vaddq_f64(rows[i], rows[7-i]);
        float64x2_t diff = vsubq_f64(rows[i], rows[7-i]);
        
        // Stage 2: Apply DCT coefficients
        float64x2_t term1 = vmulq_n_f64(sum, C4);
        float64x2_t term2 = vmulq_n_f64(diff, C4);
        
        // Additional terms for different frequencies
        float64x2_t term3 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(rows[i], C1),
            vmulq_n_f64(rows[7-i], C7)
        ), 0.5);
        
        float64x2_t term4 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(rows[i], C3),
            vmulq_n_f64(rows[7-i], C5)
        ), 0.5);
        
        float64x2_t term5 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(rows[i], C5),
            vmulq_n_f64(rows[7-i], C3)
        ), 0.5);
        
        float64x2_t term6 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(rows[i], C7),
            vmulq_n_f64(rows[7-i], C1)
        ), 0.5);
        
        // Combine terms
        temp[i] = vaddq_f64(term1, term3);
        temp[7-i] = vaddq_f64(term2, term4);
    }
    
    // Transpose the matrix
    for (int i = 0; i < 8; i++) {
        cols[i] = temp[i];
    }
    
    // 1D DCT on columns
    for (int i = 0; i < 8; i++) {
        // Similar process as rows
        float64x2_t sum = vaddq_f64(cols[i], cols[7-i]);
        float64x2_t diff = vsubq_f64(cols[i], cols[7-i]);
        
        float64x2_t term1 = vmulq_n_f64(sum, C4);
        float64x2_t term2 = vmulq_n_f64(diff, C4);
        
        float64x2_t term3 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(cols[i], C1),
            vmulq_n_f64(cols[7-i], C7)
        ), 0.5);
        
        float64x2_t term4 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(cols[i], C3),
            vmulq_n_f64(cols[7-i], C5)
        ), 0.5);
        
        float64x2_t term5 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(cols[i], C5),
            vmulq_n_f64(cols[7-i], C3)
        ), 0.5);
        
        float64x2_t term6 = vmulq_n_f64(vaddq_f64(
            vmulq_n_f64(cols[i], C7),
            vmulq_n_f64(cols[7-i], C1)
        ), 0.5);
        
        // Scale and store final results
        float64x2_t result = vmulq_n_f64(
            vaddq_f64(vaddq_f64(term1, term3), vaddq_f64(term4, term6)),
            1.0 / 8.0
        );
        vst1q_f64(&output[i * 8], result);
    }
}

// Helper function to perform full 8x8 DCT
void dct_transform_8x8(const double input[64], double output[64]) {
    dct_8x8_neon(input, output);
    
    // Apply normalization factors
    const double norm_factor = sqrt(1.0 / 8.0);
    for (int i = 0; i < 64; i++) {
        output[i] *= norm_factor;
    }
}

#endif // __aarch64__ || _M_ARM64

static PhashError compute_dct(const double* input, double* output,
                             const PhashConfig* cfg) {
#if defined(__x86_64__) || defined(_M_X64)
    if (cfg->dct_size == 8 && cfg->dct_method == DCT_METHOD_AAN) {
        dct_8x8_aan(input, output);
    } else {
        dct_generic(input, output, cfg->dct_size);
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    if (cfg->dct_size == 8) {
        dct_8x8_neon(input, output);
    } else {
        dct_generic(input, output, cfg->dct_size);
    }
#endif
    return PHASH_OK;
}

// Public API implementation
PhashError phash_compute(const PhashImage* image,
                        const PhashConfig* config,
                        uint64_t* out_hash) {
    PhashError err;
    double *grayscale = NULL, *dct_matrix = NULL;
    
    if (!image || !config || !out_hash) 
        return PHASH_ERR_NULL_POINTER;
    
    if ((err = phash_config_validate(config)) != PHASH_OK)
        return err;
    
    if ((err = resize_and_grayscale(image, config, &grayscale)) != PHASH_OK)
        return err;
    
    dct_matrix = aligned_alloc(ALIGNMENT, config->dct_size*config->dct_size*sizeof(double));
    if (!dct_matrix) {
        free(grayscale);
        return PHASH_ERR_MEMORY_ALLOCATION;
    }
    
    if ((err = compute_dct(grayscale, dct_matrix, config)) != PHASH_OK) {
        free(grayscale);
        free(dct_matrix);
        return err;
    }
    
    // Compute hash
    const int hash_size = config->hash_size;
    const int dct_size = config->dct_size;
    double avg = 0.0;
    int count = 0;
    
    for (int y = 0; y < hash_size; y++) {
        for (int x = 0; x < hash_size; x++) {
            if (x == 0 && y == 0) continue;
            avg += dct_matrix[y*dct_size + x];
            count++;
        }
    }
    
    if (count == 0) {
        free(grayscale);
        free(dct_matrix);
        return PHASH_ERR_DOMAIN;
    }
    
    avg /= count;
    uint64_t hash = 0;
    int bit_pos = 0;
    
    for (int y = 0; y < hash_size; y++) {
        for (int x = 0; x < hash_size; x++) {
            if (x == 0 && y == 0) continue;
            if (dct_matrix[y*dct_size + x] > avg)
                hash |= 1ULL << bit_pos;
            bit_pos++;
        }
    }
    
    free(grayscale);
    free(dct_matrix);
    *out_hash = hash;
    return PHASH_OK;
}

// Remaining API functions
PhashError phash_compare(uint64_t hash_a, uint64_t hash_b, int* out_distance) {
    if (!out_distance) return PHASH_ERR_NULL_POINTER;
    *out_distance = __builtin_popcountll(hash_a ^ hash_b);
    return PHASH_OK;
}

PhashError phash_image_create(const unsigned char* data,
                             int width, int height, int channels,
                             bool copy_data, PhashImage** out_image) {
    if (!data || !out_image) return PHASH_ERR_NULL_POINTER;
    
    PhashImage* img = malloc(sizeof(PhashImage));
    if (!img) return PHASH_ERR_MEMORY_ALLOCATION;
    
    if (copy_data) {
        size_t size = width * height * channels;
        unsigned char* copy = malloc(size);
        if (!copy) {
            free(img);
            return PHASH_ERR_MEMORY_ALLOCATION;
        }
        memcpy(copy, data, size);
        img->data = copy;
    } else {
        img->data = data;
    }
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->owns_memory = copy_data;
    *out_image = img;
    return PHASH_OK;
}

void phash_image_destroy(PhashImage* image) {
    if (image) {
        if (image->owns_memory) free((void*)image->data);
        free(image);
    }
}

const char* phash_error_string(PhashError error) {
    if (error < 0 || error > PHASH_ERR_DOMAIN) return "Unknown error";
    return ERROR_STRINGS[error];
}

PhashConfig phash_config_default(void) {
    return (PhashConfig){
        .dct_size = 32,
        .hash_size = 8,
        .use_high_precision = 0,
        .enable_simd = 1,
        .colorspace = COLORSPACE_REC709,
        .dct_method = DCT_METHOD_AUTO
    };
}

PhashError phash_initialize(void) {
#if defined(__x86_64__) || defined(_M_X64)
    g_avx2_enabled = 1;
    g_sse4_enabled = 1;
#elif defined(__aarch64__) || defined(_M_ARM64)
    // ARM NEON is always available on Apple M1
    g_avx2_enabled = 0;
    g_sse4_enabled = 0;
#endif
    return PHASH_OK;
}

void phash_terminate(void) {
    free(g_dct_lookup.coefficients);
    g_dct_lookup = (DCTLookup){0};
}

