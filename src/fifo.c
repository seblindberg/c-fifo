#include <fifo.h>

/* Notes:
 * The write index points to the next position that can be written to. The read
 * index points to the first position that can be read from.
 *
 * When the buffer is full the read and write indecies will be equal, and the
 * lowest bit of the mask will be cleared.
 */

/* Macros ------------------------------------------------------------------- */

#define FIFO__MARK_AS_FULL(mask)                            \
  mask &= (~0x01)

#define FIFO__ADVANCE_CURSOR(cursor, mask)                  \
  cursor = (cursor + 1) & mask
  
#define FIFO__REGRESS_CURSOR(cursor, mask)                  \
  cursor = (cursor - 1) & mask

#define FIFO__IS_ZERO_SIZE(fifo)                            \
  (fifo->mask == 0)

/* Private Functions -------------------------------------------------------- */

static inline bool_t
  buffer_includes_edge(fifo_t const *fifo);

static void
  grow_buffer(fifo_t *fifo, uint8_t mask);
  
static fifo__result_t
  shrink_buffer(fifo_t *fifo, uint8_t mask);
  
static uint8_t
  size_to_mask(size_t size) PURE;


/* Global Variables --------------------------------------------------------- */




/* Function Definitions ----------------------------------------------------- */

/* Initialize a new fifo object.
 *
 * Calling fifo__ctor(fifo, NULL, 0) will initialize a zero size fifo that will
 * always read as both empty and full. If instead a buffer handle was provided
 * the fifo can be resized using fifo__resize.
 */
void fifo__ctor(fifo_t *fifo, void *buffer, size_t size)
{
  assert(size <= FIFO__SIZE_MAX);
  assert(size >= FIFO__SIZE_MIN || size == 0);
  
  WRITE_CONST(fifo->buffer, uint8_t*, buffer);
  /* Support 0 size buffers */
  if (size == 0) {
    fifo->mask = 0;
  } else {
    fifo->mask = size_to_mask(size);
    //fifo->mask = size - 1;
    /* Size must be a power of 2 */
    //assert((~fifo->mask & size) == size);
  }
  
  fifo->read   = 0;
  fifo->write  = 0;
}


/* Resize
 *
 * Change the size of the fifo buffer. Note that the underlying memory area must
 * be big enough to contain the new size.
 */
fifo__result_t
fifo__resize(fifo_t *fifo, size_t new_size)
{
  size_t current_size = fifo->mask;
  uint8_t new_mask;
    
  if (current_size & 0x0001) {
    current_size += 1;
  } else if (current_size > 0) {
    current_size += 2;
  }
  
  if (new_size == current_size) {
    return FIFO__OK;
  }
  
  /* Handle zero size fifos */
  if (new_size == 0) {
    if (fifo__is_empty(fifo)) {
      fifo->read  = 0;
      fifo->write = 0;
      fifo->mask  = 0;
      
      return FIFO__OK;
    } else {
      return FIFO__FULL;
    }
  }
  
  if (new_size < FIFO__SIZE_MIN || new_size > FIFO__SIZE_MAX) {
    return FIFO__INVALID_SIZE;
  }
  
  /* At this point the buffer must be set. */
  assert(fifo->buffer != NULL);

  new_mask = size_to_mask(new_size); //new_size - 1;
  
  /* The new size must be a power of 2 */
  // if ((~new_mask & new_size) != new_size) {
  //   return FIFO__INVALID_SIZE;
  // }

  if (new_size < current_size) {
    return shrink_buffer(fifo, new_mask);
  } else {
    grow_buffer(fifo, new_mask);
  }
  
  return FIFO__OK;
}


/* Flush
 *
 * Empty the fifo and reset it to its pristine state.
 */
void
fifo__flush(fifo_t *fifo)
{
  if (FIFO__IS_ZERO_SIZE(fifo)) {
    return;
  }
  
  fifo->read  = 0;
  fifo->write = 0;
  fifo->mask |= 0x01;
}

/* Is Empty
 *
 * Returns non-zero if the fifo is empty.
 */
bool_t
fifo__is_empty(fifo_t const *fifo)
{
  return (fifo->read == fifo->write && !fifo__is_full(fifo))
         || FIFO__IS_ZERO_SIZE(fifo);
}


/* Size
 *
 * Returns the total size of the buffer in bytes.
 */
size_t
fifo__size(fifo_t const *fifo)
{
  if (FIFO__IS_ZERO_SIZE(fifo)) {
    return 0;
  }
  
  return (fifo->mask | 0x01) + 1;
}


/* Used
 *
 * Returns the number of bytes currently used.
 */
size_t
fifo__used(fifo_t const *fifo)
{
  uint8_t mask = fifo->mask;
  size_t  used;
    
  if (mask == 0) { // FIFO__IS_ZERO_SIZE
    return 0;
  }
  
  /* If full */
  if ((mask & 0x01) == 0) {
    return mask + 2;
  }
  
  used = fifo->write - fifo->read;
  
  /* If empty */
  if (used == 0) {
    return 0;
  }
  
  return (used & mask);
}


/* Available
 *
 * Returns the number of free bytes in the buffer.
 */
size_t
fifo__available(fifo_t const *fifo)
{
  uint8_t mask = fifo->mask;
  size_t  available;
    
  /* If full */
  if ((mask & 0x01) == 0) {
    return 0;
  }
  
  available = fifo->read - fifo->write;
  
  /* If empty */
  if (available == 0) {
    return mask + 1;
  }
  
  return (available & mask);
}

/* Write
 *
 * Writes the given src buffer to the fifo.
 * The write cursor points to the position in the buffer that should be written
 * to next.
 */
size_t
fifo__write(fifo_t *fifo, void const *src, size_t len)
{
  uint8_t to_write;
  uint8_t cursor;
  uint8_t cursor_limit;
  uint8_t mask;
  
  uint8_t const *src_buffer = (uint8_t const *) src;
  
  assert(fifo != NULL);
  assert(src != NULL);
  assert(len > 0);
    
  if (fifo__is_full(fifo)) {
    return 0;
  }
  
  cursor       = fifo->write;
  cursor_limit = fifo->read;
  mask         = fifo->mask;
  
  if (len > FIFO__SIZE_MAX) {
    len = FIFO__SIZE_MAX;
  }
  
  to_write = len;
  
  for (;;) {
    fifo->buffer[cursor] = *(src_buffer++);
    FIFO__ADVANCE_CURSOR(cursor, mask);
    
    if (cursor == cursor_limit) {
      len -= (to_write - 1);
      
      FIFO__MARK_AS_FULL(mask);
      fifo->mask = mask;
      
      break;
    }
    
    if (--to_write == 0) {
      break;
    }
  }
  
  /* Update write position */
  fifo->write = cursor;
  
  /* Because we are counting from 0 */
  return len;
}


/* Force Write
 *
 * Not yet implemented.
 */
bool_t
fifo__write_force(fifo_t *fifo, void const *src, size_t len)
{
  assert(0);
  return 0;
}


/* Read
 *
 * Read a number of bytes from the buffer.
 * Returns the number of bytes that where successfully read.
 */
size_t
fifo__read(fifo_t *fifo, void *dest, size_t len)
{
  uint8_t  to_read;
  uint8_t  cursor;
  uint8_t  cursor_limit;
  uint8_t  mask;
  
  uint8_t *dest_buffer = (uint8_t *) dest;
  
  assert(fifo != NULL);
  assert(dest != NULL);
  assert(len > 0);
  
  cursor       = fifo->read;
  cursor_limit = fifo->write;
  mask         = fifo->mask;
  
  /* If not full */
  if (mask & 0x01) {
    /* Empty */
    if (cursor == cursor_limit) {
      return 0;
    }
  } else {
    if (mask == 0) { // FIFO__IS_ZERO_SIZE
      return 0;
    }
    
    mask |= 0x01;
    fifo->mask = mask;
  }
  
  /* Predict what the read size will be */
  if (len > FIFO__SIZE_MAX) {
    len = FIFO__SIZE_MAX;
  }
  
  to_read = len;
    
  for (;;) {
    /* Read at least one */
    *(dest_buffer++) = fifo->buffer[cursor];
    FIFO__ADVANCE_CURSOR(cursor, mask);
    
    if (cursor == cursor_limit) {
      len -= (to_read - 1);
      break;
    }
    
    if (--to_read == 0) {
      break;
    }
  }
  
  fifo->read = cursor;
  
  return len;
}


/* Private Function Definitions --------------------------------------------- */


/* Buffer Includes Edges [private]
 *
 * Check if the data currently held by the fifo wraps around the outer edges of
 * the buffer.
 */
bool_t
buffer_includes_edge(fifo_t const *fifo)
{
  uint8_t read_pos  = fifo->read;
  uint8_t write_pos = fifo->write;
  
  if (write_pos < read_pos) {
    return 1;
  } else if (write_pos == read_pos) {
    return (~fifo->mask & 0x01);
  } else {
    return 0;
  }
}


/* Grow Buffer [private]
 *
 * Increase the size of the fifo.
 */
void
grow_buffer(fifo_t *fifo, uint8_t mask)
{
  if (buffer_includes_edge(fifo)) {
    uint8_t move_from             = 0;
    uint8_t const move_from_limit = fifo->write;
    uint8_t move_to               = fifo__size(fifo);
    
    /* Move everyting between 0 and read pos to after the
       old edge of the buffer */
    for (;;) {
      fifo->buffer[move_to] = fifo->buffer[move_from++];
      FIFO__ADVANCE_CURSOR(move_to, mask);
            
      if (move_from == move_from_limit) {
        break;
      }
    }
    
    fifo->write = move_to;
  }
  
  fifo->mask = mask;
}


/* Shrink Buffer [private]
 *
 * Decrease the size of the fifo.
 *
 * These are the five different cases that need to be handled by the shrink
 * function, shown on a size 8 fifo that is to be halved in size.
 *
 *   0 1 2 3 4 5 6 7    Comment
 * A [ . . ]|W . . .    No moving of data required.
 * B . . [ .|. ] W .    The upper half must be copied.
 * C . . . .|[ . ] W    The entire buffer must be copied.
 * D . ] W .|. . [ .    The lower half must be copied.
 * E . [ . .|. . ] W    The buffer cannot be shrunk.
 */
fifo__result_t
shrink_buffer(fifo_t *fifo, uint8_t mask)
{
  size_t  const new_size = (mask + 1);
  size_t  const used     = fifo__used(fifo);
  uint8_t current_mask   = fifo->mask;
  
  uint8_t first;
  uint8_t write;
  uint8_t last;
  
  uint8_t copied;
  uint8_t move_from;
  uint8_t move_to;
  
  /* Can we even shrink the buffer? */
  if (used > new_size) {
    return FIFO__FULL;
  }
  
  if (used == 0) {
    fifo->read  = 0;
    fifo->write = 0;
    
    goto fifo__shrink_buffer__mask;
  }
  
  /* The first and last index of the current buffer content. */
  first = fifo->read;
  write = fifo->write;
  
  last  = write;
  FIFO__REGRESS_CURSOR(last, current_mask);
  
  //printf("first = %d\nlast = %d\nmask = %d\n", first, last, mask);
  
  /* Both read and write are on the left side of the buffer
     edge so nothing needs to be done. */
  if (first <= last && last <= mask) {
    /* The write pos might be just past the edge. */
    if (last == mask) {
      fifo->write = 0;
      goto fifo__shrink_buffer__check_full;
    }
    
    goto fifo__shrink_buffer__mask;
  }
  
  /* Is the first byte past the edge? */
  if (first > mask) {
    move_from = first;
    
    /* Is the last byte also past the edge? */
    if (last > mask) {
      move_to = 0;
      copied  = used;
      
      fifo->read  = 0;
      fifo->write = copied;
    } else {
      
      move_to = mask - (current_mask - first);
      copied  = mask - move_to + 1;
      
      fifo->read = move_to;
    }
    
  } else {
    move_to   = 0;
    move_from = new_size;
    copied    = write - move_from;
    
    fifo->write = copied;
  }
  
  //printf("move_to = %d\nmove_from = %d\ncopied = %d\n", move_to, move_from, copied);
  
  for (;;) {
    fifo->buffer[move_to] = fifo->buffer[move_from];
    
    if (--copied == 0) {
      break;
    }
    
    ++move_to;
    ++move_from;
  }
  
fifo__shrink_buffer__check_full:
  /* Mark buffer as full */
  if (fifo->write == fifo->read) {
    FIFO__MARK_AS_FULL(mask);
  }
  
fifo__shrink_buffer__mask:
  fifo->mask = mask;
  
  return FIFO__OK;
}

/* Size to Mask [private]
 *
 * The largest size supported is 0x100.
 *
 * TODO: Optimize this for size.
 */
uint8_t
size_to_mask(size_t size)
{
  uint8_t mask;
  
  if (size == 0x100) {
    return 0xFF;
  }
  
  /* First check if the size is already a power of 2. */
  mask = size - 1;
  if ((~mask & size) == size) {
    return mask;
  }
  
  /* Perform a binary search for the highest 1 in size. */
  if (size & 0xF0) {
    if (size & 0xC0) {
      if (size & 0x80) {
        return 0x7F;
      } else {
        return 0x3F;
      }
    } else { /* 0x30 */
      if (size & 0x20) {
        return 0x1F;
      } else {
        return 0x0F;
      }
    }
  } else {
    if (size & 0x0C) {
      if (size & 0x08) {
        return 0x07;
      } else {
        return 0x03;
      }
    }
    /* Sizes this small are not supported */
    //else { /* 0x03 */
    // if (size & 0x02) {
    //   return 0x01;
    // } else {
    //   return 0;
    // }
    //}
    return 0;
  }
}