#pragma once

#define TARA_DISALLOW_COPY(TYPE) \
  TYPE(const TYPE &) = delete;   \
  void operator=(const TYPE &) = delete

#define TARA_LENGTH_OF(ARRAY) \
  (sizeof (ARRAY) / sizeof *(ARRAY))

#define TARA_OFFSET_OF(TYPE, FIELD) \
  ((char *)&((TYPE *)0)->FIELD - (char *)0)

#define TARA_CONTAINER_OF(ADDRESS, TYPE, FIELD) \
  ((TYPE *)((char *)(ADDRESS) - TARA_OFFSET_OF(TYPE, FIELD)))
