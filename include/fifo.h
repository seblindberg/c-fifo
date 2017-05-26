/* Fifo
 *
 * The buffer size must be a power of 2, with the maximum size being 2^8 = 256.
 *
 * The smallest buffer size the fifo can support is 4 bytes. This is a result of
 * how the size is stored internally and needed to correctly handle zero size
 * buffers.
 */

#ifndef FIFO_H
#define FIFO_H 1

/* Includes ----------------------------------------------------------------- */

#include <compiler.h>


#define FIFO__SIZE_MAX                            256
#define FIFO__SIZE_MIN                            4


/* Data Types --------------------------------------------------------------- */

/* Main FIFO data type.
 * Size: 2+3 = 5 bytes.
 */
typedef struct fifo {
  uint8_t * const buffer;
  uint8_t volatile mask;
  uint8_t volatile read;
  uint8_t volatile write;
} fifo_t;

typedef enum {
  FIFO__OK = 0,
  FIFO__EMPTY,
  FIFO__FULL,
  FIFO__INVALID_SIZE,
} fifo__result_t;


/* Macros ------------------------------------------------------------------- */




/* Global Variables --------------------------------------------------------- */




/* Public Functions --------------------------------------------------------- */

void
  fifo__ctor(fifo_t *fifo, void *buffer, size_t size);
  
fifo__result_t
  fifo__resize(fifo_t *fifo, size_t new_size);

void
  fifo__flush(fifo_t *fifo);

bool_t
  fifo__is_full(fifo_t const *fifo);
  
bool_t
  fifo__is_empty(fifo_t const *fifo);
  
size_t
  fifo__size(fifo_t const *fifo);
  
size_t
  fifo__used(fifo_t const *fifo);
  
size_t
  fifo__available(fifo_t const *fifo);
  
size_t
  fifo__write(fifo_t *fifo, void const *src, size_t len);
  
bool_t
  fifo__write_force(fifo_t *fifo, void *src, size_t len);

size_t
  fifo__read(fifo_t *fifo, void *dest, size_t size);

#endif /* FIFO_H */