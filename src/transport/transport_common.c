#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_trans.h>

#include <wg_string.h>
#include "include/transport_common.h"

#define ADDR_EXPR  "^\\(unix\\|inet\\)[: ]\\([^$]\\+\\)$"

enum {
    GROUP_ALL     = 0 ,
    GROUP_TYPE        ,
    GROUP_ADDRESS     ,
    GROUP_NUM
};

wg_status
transport_match_address(const wg_char *address, wg_char **type, wg_char **addr)
{
    regex_t addr_expr;
    int reg_status = 1;
    wg_status status = WG_FAILURE;
    regmatch_t match_list[GROUP_NUM];
    regmatch_t *match_tmp = NULL;
    wg_char *tmp_1 = NULL;
    wg_char *tmp_2 = NULL;
    wg_int len = 0;

    CHECK_FOR_NULL_PARAM(type);
    CHECK_FOR_NULL_PARAM(addr);

    reg_status = regcomp(&addr_expr, ADDR_EXPR, REG_ICASE);
    if (0 != reg_status){
        return WG_FAILURE;
    }

    reg_status = regexec(&addr_expr, address, ELEMNUM(match_list),
            match_list, 0);
    if (0 != reg_status){
        regfree(&addr_expr);
        return WG_FAILURE;
    }
    regfree(&addr_expr);

    match_tmp = &match_list[GROUP_TYPE];
    len = match_tmp->rm_eo - match_tmp->rm_so;
    status = wg_strndup(&tmp_1, address + match_tmp->rm_so, len);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    match_tmp = &match_list[GROUP_ADDRESS];
    len = match_tmp->rm_eo - match_tmp->rm_so;
    status = wg_strndup(&tmp_2, address + match_tmp->rm_so, len);
    if (WG_SUCCESS != status){
        WG_FREE(tmp_1);
        return WG_FAILURE;
    }

    *type = tmp_1;
    *addr = tmp_2;

    return WG_SUCCESS;
}

wg_status
transport_initialize(const Transport_init *transports, wg_size num, 
        const wg_char *type, const wg_char *address, Wg_transport *trans)
{
    wg_int i = 0;
    wg_status status = WG_FAILURE;
    const Transport_init *info = NULL;

    for (i = 0; i < num; ++i){
        info = &transports[i];
        if (strcmp(type, info->name) == 0){
            status = info->init(trans, address);
            if (WG_SUCCESS != status){
                WG_LOG("Could not initialize \"%s\" transport\n", info->name);
            }
            break;
        }
    }

    return status;
}
