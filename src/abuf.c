#include "abuf.h"

#include <stdlib.h>
#include <string.h>

void
ab_append (struct abuf *ab, const char *str, int len)
{
  char *new = realloc (ab->b, ab->len + len);

  if (new == NULL)
    return;

  memcpy (&new[ab->len], str, len);
  ab->b = new;
  ab->len += len;
}

// desctructor
void
ab_free (struct abuf *ab)
{
  free (ab->b);
}
