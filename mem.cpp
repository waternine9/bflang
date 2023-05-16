#include <stdint.h>
#include <stddef.h>
#include "mem.hpp"
#include <stdlib.h>

void kmemcpy(void *Destination_, const void *Source_, size_t N)
{
    uint8_t *Destination = (uint8_t*)Destination_;
    const uint8_t *Source = (uint8_t*)Source_;
    while (N--) {
        *Destination++ = *Source++;
    }
}
void kmemset(void *Destination_, uint8_t Val, size_t N)
{
    uint8_t *Destination = (uint8_t*)Destination_;
    while (N--) {
        *Destination++ = Val;
    }
}

// Each page is 512 bytes
uint8_t Pages[1000000] = { 0 };

uint8_t IsInit = 0;

void *kmalloc(size_t Bytes)
{
    return malloc(Bytes);
}

void kfree(void *Buf)
{
    free(Buf);
}
