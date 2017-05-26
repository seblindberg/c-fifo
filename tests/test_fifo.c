#include <compiler.h>
#include <fifo.h>

#include "helper.h"


void test__create(void)
{
  fifo_t *fifo = helper__setup_fifo();
  
  assert(fifo__size(fifo) == HELPER__BUFFER_SIZE);
  assert(fifo__used(fifo) == 0);
  assert(fifo__available(fifo) == HELPER__BUFFER_SIZE);
}

void test__write(void)
{
  fifo_t *fifo = helper__setup_fifo();
  uint8_t to_write[] = { 1, 2, 3, 4, 5 };
  size_t ret;
  
  /* Test writing a buffer that fits the fifo */
  ret = fifo__write(fifo, to_write, sizeof(to_write));
  
  assert(ret == sizeof(to_write));
  assert(fifo__used(fifo) == sizeof(to_write));
  assert(fifo__available(fifo)
            == HELPER__BUFFER_SIZE - sizeof(to_write));
  
  /* Test writing a buffer that fills the fifo */
  ret = fifo__write(fifo, to_write, sizeof(to_write));
  
  assert(ret == HELPER__BUFFER_SIZE - sizeof(to_write));
  assert(fifo__is_full(fifo));
  assert(fifo__available(fifo) == 0);
}

void test__read(void)
{
  fifo_t *fifo = helper__setup_fifo();
  uint8_t write[] = { 1, 2, 3, 4, 5 };
  uint8_t read[HELPER__BUFFER_SIZE];
  size_t  ret;
  
  fifo__write(fifo, write, sizeof(write));
  
  ret = fifo__read(fifo, read, sizeof(read));
  
  assert(ret == sizeof(write));
  assert(fifo__is_empty(fifo));
  assert(helper__is_equal(write, read, sizeof(write)));
  
  fifo__write(fifo, write, sizeof(write));
  fifo__write(fifo, write, sizeof(write));
  
  ret = fifo__read(fifo, read, sizeof(read));
  
  assert(ret == HELPER__BUFFER_SIZE);
}

void test__grow_buffer(void)
{
  fifo_t *fifo = helper__setup_fifo();
  uint8_t write[] = { 1, 2, 3, 4, 5 };
  uint8_t read[HELPER__BUFFER_SIZE_GROW];
  uint_fast8_t res;
  
  fifo__write(fifo, write, sizeof(write));
  
  res = fifo__resize(fifo, HELPER__BUFFER_SIZE_GROW);
  assert(res == FIFO__OK);
  
  assert(fifo__size(fifo) == HELPER__BUFFER_SIZE_GROW);
  
  fifo__read(fifo, read, sizeof(read));
  assert(helper__is_equal(write, read, sizeof(write)));
  
  /* Restore the fifo */
  fifo = helper__setup_fifo();
  
  fifo__write(fifo, write, sizeof(write));
  fifo__read(fifo, read, sizeof(read));
  fifo__write(fifo, write, sizeof(write));
  
  res = fifo__resize(fifo, HELPER__BUFFER_SIZE_GROW);
  assert(res == FIFO__OK);
  
  res = fifo__read(fifo, read, sizeof(read));
  assert(res == sizeof(write));
  
  assert(helper__is_equal(write, read, sizeof(write)));
}

void test__shrink_buffer(void)
{
  fifo_t *fifo;
  uint8_t write[] = { 1, 2, 3, 4, 5 };
  uint8_t read[HELPER__BUFFER_SIZE_GROW];
  
  /* [1 2 3 4 5 . . .] -/-> [1 2 3 4] */
  fifo = helper__setup_fifo();
  
  /* Write 5 bytes to the 8 byte fifo. Shrinking should not
     be possible. */
  fifo__write(fifo, write, 5);
  
  assert(fifo__resize(fifo, HELPER__BUFFER_SIZE_SHRINK) ==
    FIFO__FULL);
  
  /* [1 2 3 . . . . .] ---> [1 2 3 .] */
  fifo = helper__setup_fifo();
  
  /* Write 3 bytes to the beginning of the 8 byte fifo.
     Shrinking should be possible. */
  fifo__write(fifo, write, 3);
  
  
  assert(fifo__resize(fifo, HELPER__BUFFER_SIZE_SHRINK) ==
    FIFO__OK);
  assert(helper__contains(fifo, write, 3));
  
  /* [. . 1 2 3 . . .] ---> [3 . 1 2] */
  fifo = helper__setup_fifo();
  
  fifo__write(fifo, write, 2);
  fifo__read(fifo, read, 2);
  
  fifo__write(fifo, write, 3);
  
  assert(fifo__resize(fifo, HELPER__BUFFER_SIZE_SHRINK) ==
    FIFO__OK);
  assert(helper__contains(fifo, write, 3));
  
  /* [. . . . 1 2 3 .] ---> [1 2 3 .] */
  fifo = helper__setup_fifo();
  
  fifo__write(fifo, write, 4);
  fifo__read(fifo, read, 4);
  
  fifo__write(fifo, write, 3);
  
  assert(fifo__resize(fifo, HELPER__BUFFER_SIZE_SHRINK) ==
    FIFO__OK);
  assert(helper__contains(fifo, write, 3));
  
  /* [3 . . . . . 1 2] ---> [3 . 1 2] */
  fifo = helper__setup_fifo();
  
  fifo__write(fifo, write, 6);
  fifo__read(fifo, read, 6);
  
  fifo__write(fifo, write, 3);
  
  assert(fifo__resize(fifo, HELPER__BUFFER_SIZE_SHRINK) ==
    FIFO__OK);
  assert(helper__contains(fifo, write, 3));
  
  /* [3 4 . . . . 1 2] ---> [3 4 1 2] */
  fifo = helper__setup_fifo();
  
  fifo__write(fifo, write, 6);
  fifo__read(fifo, read, 6);
  
  fifo__write(fifo, write, 4);
  
  assert(fifo__resize(fifo, HELPER__BUFFER_SIZE_SHRINK) ==
    FIFO__OK);
  assert(fifo__is_full(fifo));
  assert(helper__contains(fifo, write, 4));
}

void test__zero_size_fifo(void)
{
  fifo_t fifo;
  uint8_t dest[1];
  fifo__ctor(&fifo, NULL, 0);
  
  assert(fifo__is_full(&fifo));
  assert(fifo__is_empty(&fifo));
  assert(fifo__size(&fifo) == 0);
  assert(fifo__used(&fifo) == 0);
  assert(fifo__available(&fifo) == 0);
  assert(fifo__write(&fifo, "test", 4) == 0);
  assert(fifo__read(&fifo, dest, 1) == 0);
  assert(fifo__resize(&fifo, 0) == FIFO__OK);
  
  fifo__flush(&fifo);
  assert(fifo__size(&fifo) == 0);
  /* This is not permitted since no buffer is provided. */
  //fifo__resize(&fifo, 4);
}

void test__resize_zero_size_fifo(void)
{
  fifo_t fifo;
  uint8_t buffer[FIFO__SIZE_MIN];
  uint8_t dest[FIFO__SIZE_MIN];
  fifo__ctor(&fifo, buffer, 0);
  
  assert(fifo__resize(&fifo, FIFO__SIZE_MIN) == FIFO__OK);
  assert(fifo__size(&fifo) == FIFO__SIZE_MIN);
  assert(fifo__write(&fifo, "test", 4) == 4);
  assert(fifo__read(&fifo, dest, 4) == 4);
  assert(memcmp(dest, "test", 4) == 0);
  
  fifo__write(&fifo, "t", 1);
  assert(fifo__resize(&fifo, 0) == FIFO__FULL);
  
  fifo__flush(&fifo);
  assert(fifo__resize(&fifo, 0) == FIFO__OK);
}

void test__uneven_buffer_size(void)
{
  fifo_t fifo;
  uint8_t buffer[19];
  
  fifo__ctor(&fifo, buffer, sizeof(buffer));
  assert(fifo__size(&fifo) == 16);
  
  fifo__resize(&fifo, 10);
  assert(fifo__size(&fifo) == 8);
}

int main(int argc, char *argv[])
{
  test__create();
  test__write();
  test__read();
  test__grow_buffer();
  test__shrink_buffer();
  test__zero_size_fifo();
  test__resize_zero_size_fifo();
  test__uneven_buffer_size();
  
  puts("fifo passed all tests");
  
  return 0;
}
