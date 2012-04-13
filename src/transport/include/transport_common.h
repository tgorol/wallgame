#ifndef TRANSPORT_COMMON_H
#define TRANSPORT_COMMON_H

typedef struct Transport_init{
    wg_char *name;
    wg_status (*init)(Wg_transport *, const wg_char *);
}Transport_init;

WG_PUBLIC wg_status
transport_match_address(const wg_char *address, 
        wg_char **type, wg_char **addr);

WG_PUBLIC wg_status
transport_initialize(const Transport_init *transports, wg_size num,  
        const wg_char *type, const wg_char *address, Wg_transport *trans);

#endif
