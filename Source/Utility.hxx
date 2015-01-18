#pragma once

#define TARA_LENGTH_OF(ARRAY) \
  (sizeof (ARRAY) / sizeof *(ARRAY))

#define TARA_OFFSET_OF(TYPE, FIELD) \
  ((unsigned char *)&((TYPE *)0)->FIELD - (unsigned char *)0)

#define TARA_CONTAINER_OF(ADDRESS, TYPE, FIELD) \
  ((TYPE *)((unsigned char *)(ADDRESS) - TARA_OFFSET_OF(TYPE, FIELD)))
