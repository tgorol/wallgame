#ifndef WG_LSDIR_H
#define WG_LSDIR_H


typedef struct wg_dirent{
    wg_char *d_name;
    List_head list;
}wg_dirent;

WG_PUBLIC wg_status
wg_lsdir(const wg_char *path, const wg_char *prefix, List_head *head);

WG_PUBLIC wg_status
wg_lsdir_cleanup(List_head *head);

#endif
