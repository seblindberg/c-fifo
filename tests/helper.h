#ifndef HELPER_H
#define HELPER_H 1

/* Includes ----------------------------------------------------------------- */

#include <compiler.h>
#include <fifo.h>


/* Constants -------------------------------------+-------------------------- */

#define HELPER__BUFFER_SIZE                         8
#define HELPER__BUFFER_SIZE_GROW                    16
#define HELPER__BUFFER_SIZE_SHRINK                  4


/* Data Types --------------------------------------------------------------- */




/* Global Variables --------------------------------------------------------- */




/* Public Functions --------------------------------------------------------- */

fifo_t *
  helper__setup_fifo(void);
  
bool_t
  helper__is_equal(uint8_t const *a, uint8_t const *b, size_t len);
  
bool_t
  helper__contains(fifo_t *fifo, uint8_t const *ref, size_t len);
  
void
  helper__inspect(fifo_t *fifo);


/* Macros ----------------------------------------+--------+----------------- */




/* Inline Function Definitions ---------------------------------------------- */



#endif /* HELPER_H */