#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

#include "include/wg_config.h"

#define CONFIG_MAX_LINE_SIZE 80
#define CONFIG_DELIM "="

typedef struct Config_line{
    wg_char text[CONFIG_MAX_LINE_SIZE + 1];
    List_head list;
}Config_line;

WG_PRIVATE wg_status get_value(Wg_config *config, const wg_char *key,
        Config_line **config_line);

WG_PRIVATE Config_line * create_line(const char *text);

WG_PRIVATE wg_status read_file(Wg_config *config);

WG_PRIVATE void release_line(Config_line *line);


wg_status
wg_config_init(const wg_char *filename, Wg_config *config)
{
    size_t size = 0;
    wg_char *fname = NULL;

    CHECK_FOR_NULL_PARAM(filename);
    CHECK_FOR_NULL_PARAM(config);

    size = strlen(filename);

    fname = WG_CALLOC(size + 1, sizeof (char));
    if (NULL == fname){
        return WG_FAILURE;
    }

    strcpy(fname, filename);

    config->filename = fname;

    list_init(&config->lines);

    read_file(config);

    return WG_SUCCESS;
}

void
wg_config_cleanup(Wg_config *config)
{
    Iterator itr;
    Config_line *line = NULL;

    iterator_list_init(&itr, &config->lines, GET_OFFSET(Config_line, list));

    while ((line = iterator_list_next(&itr)) != NULL){
        WG_FREE(line);
    }

    WG_FREE(config->filename);

    return;
}

wg_status
wg_config_get_value(Wg_config *config, const wg_char *key, wg_char *value,
        wg_size size)
{
    Config_line *line = NULL;
    wg_status status = WG_FAILURE;
    size_t len = 0;

    CHECK_FOR_NULL_PARAM(config);
    CHECK_FOR_NULL_PARAM(value);
    CHECK_FOR_NULL_PARAM(key);

    len = strlen(key);
    status = get_value(config, key, &line);
    if (WG_SUCCESS == status){
        strncpy(value, line->text + len + 1, size);
    }

    return status;
}

wg_status
wg_config_add_value(Wg_config *config, const wg_char *key, wg_char *value)
{
    Config_line *line = NULL;
    char line_text[CONFIG_MAX_LINE_SIZE + 1];
    size_t len = 0;

    CHECK_FOR_NULL_PARAM(config);
    CHECK_FOR_NULL_PARAM(key);
    CHECK_FOR_NULL_PARAM(value);

    if (get_value(config, key, &line) == WG_SUCCESS){
        strncpy(line->text, value, CONFIG_MAX_LINE_SIZE);
    }else{
        len = strlen(key);
        strncpy(line_text, key, CONFIG_MAX_LINE_SIZE);
        strncat(line_text, CONFIG_DELIM, CONFIG_MAX_LINE_SIZE - len);
        ++len;
        strncat(line_text, value, CONFIG_MAX_LINE_SIZE - len);

        line = create_line(line_text);
        list_add(&config->lines, &line->list);
    }

    return WG_SUCCESS;
}

wg_status
wg_config_sync(Wg_config *config)
{
    FILE *config_file = NULL;
    Config_line *line = NULL;
    Iterator itr;

    config_file = fopen(config->filename, "w");
    if (NULL == config_file){
        WG_PRINT("%s:%s\n", config->filename, strerror(errno));
        return WG_FAILURE;
    }

    iterator_list_init(&itr, &config->lines, GET_OFFSET(Config_line, list));

    while ((line = iterator_list_next(&itr)) != NULL){
        fprintf(config_file, "%s\n", line->text);
    }

    fclose(config_file);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
get_value(Wg_config *config, const wg_char *key, Config_line **config_line)
{
    Iterator itr;
    Config_line *line = NULL;
    size_t len = 0;
    wg_boolean found_flag = WG_FALSE;

    CHECK_FOR_NULL_PARAM(config);
    CHECK_FOR_NULL_PARAM(value);
    CHECK_FOR_NULL_PARAM(key);

    len = strlen(key);

    iterator_list_init(&itr, &config->lines, GET_OFFSET(Config_line, list));

    while ((line = iterator_list_next(&itr)) != NULL){
        if (strncmp(key, line->text, len) == 0){
            *config_line = line;
            found_flag = WG_TRUE;
            break;
        }
    }

    return (found_flag == WG_FALSE)? WG_FAILURE : WG_SUCCESS;
}

WG_PRIVATE Config_line *
create_line(const char *text)
{
    Config_line *line = NULL;
    size_t len = 0;

    CHECK_FOR_NULL_PARAM(text);

    line = WG_MALLOC(sizeof (Config_line));
    if (NULL == line){
        return NULL;
    }

    strncpy(line->text, text, CONFIG_MAX_LINE_SIZE);

    len = strlen(line->text);

    if (line->text[len - 1] == '\n'){
        line->text[len - 1] = '\0';
    }

    list_init(&line->list);

    return line;
}

WG_PRIVATE void
release_line(Config_line *line)
{
    WG_FREE(line);

    return;
}

WG_PRIVATE wg_status
read_file(Wg_config *config)
{
    FILE *config_file = NULL;
    Config_line *line = NULL;
    char line_text[CONFIG_MAX_LINE_SIZE + 1];

    CHECK_FOR_NULL_PARAM(config);

    config_file = fopen(config->filename, "r");
    if (NULL == config_file){
        WG_PRINT("%s:%s\n", config->filename, strerror(errno));
        return WG_FAILURE;
    }

    while (fgets(line_text, ELEMNUM(line_text), config_file) != NULL){
        line = create_line(line_text);
        list_add(&config->lines, &line->list);
    }
    if (ferror(config_file)){
        WG_PRINT("%s:%s\n", config->filename, strerror(errno));
        fclose(config_file);
        return WG_FAILURE;
    }

    fclose(config_file);

    return WG_SUCCESS;
}
