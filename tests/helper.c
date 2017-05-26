#include "helper.h"

/* Macros ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––– */



/* Private Functions –––––––––––––––––––––––––––––––––––––––––––––––––––––––– */




/* Global Variables ––––––––––––––––––––––––––––––––––––––––––––––––––––––––– */

static uint8_t helper__buffer[HELPER__BUFFER_SIZE_GROW];


/* Function Definitions ––––––––––––––––––––––––––––––––––––––––––––––––––––– */

fifo_t *helper__setup_fifo(void)
{
  static fifo_t fifo;

  fifo__ctor(&fifo, helper__buffer, HELPER__BUFFER_SIZE);

  memset(helper__buffer, 0, sizeof(helper__buffer));

  return &fifo;
}

/* Returns a non-zero value if the two buffers a and b are equal for len bytes.
 */
bool_t helper__is_equal(uint8_t const *a, uint8_t const *b, size_t len)
{
  do {
    if (*(a ++) != *(b ++)) {
      return 0;
    }
  } while (--len);

  return 1;
}

/* Returns a non-zero value if the fifo contains (exactly) the data in ref.
 */
bool_t helper__contains(fifo_t *fifo, uint8_t const *ref, size_t len)
{
  uint8_t read[len];

  fifo__read(fifo, read, len);

  return fifo__is_empty(fifo) &&
         helper__is_equal(read, ref, len);
}

/* Inspect the contents and state of a fifo.
 */
void helper__inspect(fifo_t *fifo)
{
  size_t i;
  size_t size = fifo__size(fifo);
  uint_fast8_t read  = fifo->read;
  uint_fast8_t write = fifo->write;

  for (i = 0; i < size; i ++) {
    printf("%02X ", fifo->buffer[i]);
  }

  puts("");

  for (i = 0; i < size; i ++) {
    if (i == read && i == write) {
      printf("RW ");
    } else if (i == read) {
      printf(" R ");
    } else if (i == write) {
      printf(" W ");
    } else {
      printf("   ");
    }
  }

  puts("");
}