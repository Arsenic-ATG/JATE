#ifndef ABUF_H
#define ABUF_H

struct abuf
{
  char *b;
  int len;
};

// constructor
#define ABUF_INIT                                                             \
  {                                                                           \
    NULL, 0                                                                   \
  }

// desctructor
void ab_free (struct abuf *ab);

void ab_append (struct abuf *ab, const char *str, int len);

#endif