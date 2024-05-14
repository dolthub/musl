#include "pthread_impl.h"

void **___mimalloc_default_heap_location(void)
{
	return &__pthread_self()->mi_default_heap;
}

weak_alias(___mimalloc_default_heap_location, __mimalloc_default_heap_location);
