# truerandom.h

Single-header C library for hardware-based true random number generation  
leveraging RDRAND (x86/x64) and RNDR (ARM64).

## Usage

```c
#define TRUERANDOM_IMPLEMENTATION
#include "truerandom.h"

int main(void) {
    if (!truernd_is_supported()) {
        return 1;
    }

    uint32_t val32;
    uint64_t val64;
    
    truernd_get32(&val32);  // Returns 0 on success, -1 on error
    truernd_get64(&val64);
    
    uint8_t buffer[256];
    truernd_fill(buffer, sizeof(buffer));  // Automatic retry on failure
    
    return 0;
}
```

## API

**Check Support**
```c
int truernd_is_supported(void);  // Returns 1 if supported, 0 otherwise
```

**Generate Random Numbers**
```c
uint32_t truernd_gen32(void);     // Returns value (0 on failure)
uint64_t truernd_gen64(void);     // Returns value (0 on failure)

int truernd_get32(uint32_t *out); // Returns 0 on success, -1 on error
int truernd_get64(uint64_t *out); // Returns 0 on success, -1 on error

int truernd_fill(void *buf, size_t len);  // Fill buffer, auto-retry
```

## Configuration

```c
#define TRUERND_MAX_RETRIES 10  // Set before including header
#define TRUERANDOM_IMPLEMENTATION
#include "truerandom.h"
```

## Platform Support

- x86/x64: RDRAND instruction (Intel Ivy Bridge+, AMD Zen+)
- ARM64: RNDR instruction (ARMv8.5-A+)
- 32-bit ARM: Not supported

## License

GPL-3.0
