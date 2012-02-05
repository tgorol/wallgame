#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_string.h>

#include <wg_lsdir.h>


WG_PRIVATE wg_dirent* create_dirent(const wg_char *text);

wg_status
wg_lsdir(const wg_char *path, const wg_char *prefix, List_head *head)
{
    wg_status status = WG_SUCCESS;
    DIR *dir = NULL;
    struct dirent *dir_ent = NULL;
    size_t prefix_length = 0;
    wg_dirent *new_dirent = NULL;

    CHECK_FOR_NULL_PARAM(path);
    CHECK_FOR_NULL_PARAM(head);
    CHECK_FOR_NULL_PARAM(list);

    list_init(head);

    prefix_length = strlen(prefix);

    dir = opendir(path);
    if (NULL == dir){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }

    do{
        /* allows error/no_more_entry check */
        errno = 0;
        dir_ent = readdir(dir);
        if (NULL != dir_ent){
            if (strncmp(dir_ent->d_name, prefix, prefix_length) == 0){
                new_dirent = create_dirent(dir_ent->d_name);
                list_add(head, &new_dirent->list);
            }
        }else if(EBADF == errno){
	    WG_LOG("%s\n", strerror(errno));
            status = WG_FAILURE;
        }
    }while(NULL != dir_ent); 

    closedir(dir);

    return status;
}

wg_status
wg_lsdir_cleanup(List_head *head)
{
    Iterator itr;
    wg_dirent *dirent = NULL;

    iterator_list_init(&itr, head, GET_OFFSET(wg_dirent, list));
    
    while((dirent = iterator_list_next(&itr)) != NULL){
        WG_FREE(dirent->d_name);
        WG_FREE(dirent);
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_dirent*
create_dirent(const wg_char *text)
{
    wg_dirent *dirent = NULL;
    wg_char *new_text = NULL;

    wg_strdup(text, &new_text);
    dirent = WG_MALLOC(sizeof (wg_dirent));
    if (NULL == dirent || NULL == new_text){
        WG_FREE(dirent);
        WG_FREE(new_text);
        dirent = NULL;
    }else{
	dirent->d_name = new_text;
    }

    return dirent;
}
