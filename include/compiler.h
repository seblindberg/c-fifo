#ifndef COMPILER_H
#define COMPILER_H

/* Includes ----------------------------------------------------------------- */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


/* Data Types --------------------------------------------------------------- */

typedef uint_fast8_t bool_t;


/* Macros ---------------------------------------+---------+----------------- */

#define ENTER_CRITICAL_REGION()
#define LEAVE_CRITICAL_REGION()                   

#define WRITE_CONST(field, type, value)                     \
  *((type *) &(field)) = value


#define PACKED                                    __attribute__((__packed__))
#define UNUSED                                    __attribute__((__unused__))
#define DEPRICATED                                          \
  __attribute__((__deprecated__))
#define NONNULL                                   __attribute__((__nonnull__))
#define NONNULL_ARGS(...)                                   \
  __attribute__((__nonnull__ (##__VA_ARGS__)))

#endif /* COMPILER_H */