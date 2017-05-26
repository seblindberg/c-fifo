# FIFO

A simple fifo buffer implementation designed primarily for embedded targets. To keep the memory footprint small the buffer size is limited to powers of two.

## Installation

Simply running `make` will build the static library file `libfifo.a` and place it in the `lib` directory. Run `make test` to compile and run the tests found in the `tests` directory. `make clean` will remove all output generated by the build system.

## Usage

```c
#include <fifo.h>

void test_fifo(void) 
{
  /* Allocate the fifo data structure and a buffer. The size
     must be a power of two and in the range [4, 256]. */
  fifo_t  fifo;
  uint8_t buffer[64];
  
  fifo__ctor(&fifo, buffer, 64);
  
  /* Write to the fifo.
     This function returns the number of bytes it managed to
     write. */
  fifo__write(&fifo, "Hello World", 11);
  
  fifo__used(&fifo);      // => 11
  fifo__available(&fifo); // => 53
  
  /* Read from the fifo.
     This function returns the number of bytes that were
     successfully read from the buffer. */
  char dest[11];
  fifo__read(&fifo, dest, 11);
}
```
