#ifndef WG_MEMLEAK_H
#define WG_MEMLEAK_H

WG_PUBLIC void
wg_memleak_start();

WG_PUBLIC void
wg_memleak_stop();

WG_PUBLIC void *
wg_malloc(size_t size, const wg_char *filename, wg_uint line);

WG_PUBLIC void *
wg_calloc(size_t num, size_t size, const wg_char *filename, wg_uint line);

WG_PUBLIC void
wg_free(void *ptr, wg_boolean ff);
#endif
