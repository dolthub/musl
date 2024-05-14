#include "pthread_impl.h"

_Bool *___mimalloc_recurse_location(void)
{
	return &__pthread_self()->mi_recurse;
}

weak_alias(___mimalloc_recurse_location, __mimalloc_recurse_location);
