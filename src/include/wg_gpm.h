#ifndef WG_GPM_H
#define WG_GPM_H

enum {
    WG_EXIT_SUCCESS = EXIT_SUCCESS, 
    WG_EXIT_ERROR
};

/**
 * @brief Field iterator
 */
typedef struct Field_iterator{
    const Config_section *section;  /*!< field section */
    wg_uint index;                  /*!< actual index  */
    wg_uint last_index;             /*!< last index    */
}Field_iterator;

WG_PUBLIC int
wg_start(int argc, char *argv[]);

WG_PUBLIC wg_status 
gpm_ini_get_server(const Config_section **server);

WG_PUBLIC wg_status
gpm_ini_get_max_num_of_games(wg_uint *num);

WG_PUBLIC wg_status
gpm_ini_get_game_by_name(wg_char *name, const Config_section **game,
        wg_uint *index);

WG_PUBLIC wg_status
gpm_ini_get_game_by_index(wg_uint index, const Config_section **game);

WG_PUBLIC wg_status
gpm_ini_field_iterator_next(Field_iterator *itr, const 
        Config_field **config_field);

WG_PUBLIC wg_status
gpm_ini_get_field_iterator(Field_iterator *itr, const Config_section *section);

wg_status
gpm_ini_get_field_by_id(const Config_section *section, wg_uint id,
        const Config_field **config_field);

#endif
