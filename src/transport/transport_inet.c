#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <alloca.h>
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
#include <wg_string.h>
#include <wg_trans.h>

/*! \defgroup  in_transport Inet Transport
 *  \ingroup transport
 */

/*! @{ */

/** @brief IP octal validation regexp expression           */
#define IP_DIGIT  "\\(25[0-5]\\|2[0-4][0-9]\\|[01]\\?[0-9][0-9]\\?\\)"

/** @brief Port validation regexp expression 
*
* @todo Fix: This is not fully correct
*/
#define PORT_DIGIT "[0-9]\\{1,5\\}"

/** @brief IP:PORT validation regexp expression
 */
#define INET_ADDR_EXPR \
    ( "\\(" IP_DIGIT "\\." IP_DIGIT "\\." IP_DIGIT "\\." IP_DIGIT "\\)"       \
    ":\\(" PORT_DIGIT "\\)" ) 
         
/** 
* @brief Groups in INET_ADDR_EXPR
*/
enum {
    GROUP_ALL    = 0 ,    /*!< entire string                             */
    GROUP_IP_ALL     ,    /*!< entire IP                                 */
    GROUP_IP_0       ,    /*!< IP fist octet                             */
    GROUP_IP_1       ,    /*!< IP second octet                           */
    GROUP_IP_2       ,    /*!< IP third octet                            */
    GROUP_IP_3       ,    /*!< IP fourth octet                           */
    GROUP_PORT       ,    /*!< entire port                               */
    GROUP_NUM             /*!< number of groups                          */
};

WG_PRIVATE wg_status
parse_address(const wg_char *address, wg_char **ip, wg_char **port);

/**
 * @brief Create a inet transport
 *
 * @param trans    memory to store a transport
 * @param address  address of the un socket
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_inet_new(Wg_transport *trans, const wg_char *address)
{
    wg_char *ip_string = NULL;
    wg_char *port_string = NULL;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(address);

    status = parse_address(address, &ip_string, &port_string);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    memset(trans, '\0', sizeof (Wg_transport));

    trans->transport.domain   = AF_INET;
    trans->transport.type     = SOCK_DGRAM;
    trans->transport.protocol = IPPROTO_UDP;

    trans->sockaddr.in.sin_family      = AF_INET;
    trans->sockaddr.in.sin_port        = htons(atoi(port_string));
    trans->sockaddr.in.sin_addr.s_addr = inet_addr(ip_string);
    trans->sockaddr_size = sizeof (trans->sockaddr.in);

    WG_FREE(port_string);
    WG_FREE(ip_string);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
parse_address(const wg_char *address, wg_char **ip, wg_char **port)
{
    regex_t addr_exp;
    int reg_status = 1;
    wg_status status = WG_FAILURE;
    regmatch_t match_list[GROUP_NUM];
    wg_char *ip_string   = NULL;
    wg_char *port_string = NULL;
    regmatch_t *match_tmp = NULL;
    wg_size len = 0;

    reg_status = regcomp(&addr_exp, INET_ADDR_EXPR, 0);
    if (reg_status != 0){
        return WG_FAILURE;
    }

    reg_status = regexec(&addr_exp, address, GROUP_NUM, match_list, 0);
    if (reg_status != 0){
        regfree(&addr_exp);
        return WG_FAILURE;
    }
    regfree(&addr_exp);

    match_tmp = &match_list[GROUP_IP_ALL];
    len = match_tmp->rm_eo - match_tmp->rm_so;
    status = wg_strndup(&ip_string, address + match_tmp->rm_so, len);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    match_tmp = &match_list[GROUP_PORT];
    len = match_tmp->rm_eo - match_tmp->rm_so;
    status = wg_strndup(&port_string, address + match_tmp->rm_so, len);
    if (WG_SUCCESS != status){
        WG_FREE(ip_string);
        return WG_FAILURE;
    }

    *ip  = ip_string;
    *port = port_string;

    return WG_SUCCESS;
}

/*! @} */
