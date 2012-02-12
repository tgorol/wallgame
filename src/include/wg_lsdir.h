#ifndef WG_LSDIR_H
#define WG_LSDIR_H


/** 
* @brief Directory instance
*/
typedef struct wg_dirent{
    wg_char *d_name;      /*!< name of a file in directory */
    List_head list;       /*!< linked list head            */
}wg_dirent;

WG_PUBLIC wg_status
wg_lsdir(const wg_char *path, const wg_char *prefix, List_head *head);

WG_PUBLIC wg_status
wg_lsdir_cleanup(List_head *head);

#endif
