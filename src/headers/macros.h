#ifndef MACROS
#define MACROS

// feature test macros
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

// Mask to imitate a CTRL key press on keyboard.
#define CTRL_KEY(k) ((k) & 0x1f)

#define TAB_SIZE 4

#define ABUF_INIT {NULL, 0}

#endif