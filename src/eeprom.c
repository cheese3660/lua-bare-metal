#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "eeprom.h"
#include "componentlib.h"

static int eeprom_get(lua_State *L, struct eeprom_data *data, int arguments_start) {
    if (data->contents == NULL)
        lua_pushnil(L);
    else
        lua_pushstring(L, data->contents);
    return 1;
}

static int eeprom_get_data(lua_State *L, struct eeprom_data *data, int arguments_start) {
    if (data->data == NULL)
        lua_pushnil(L);
    else
        lua_pushstring(L, data->data);
    return 1;
}

static int eeprom_set_data(lua_State *L, struct eeprom_data *data, int arguments_start) {
    const char *new_data = luaL_checkstring(L, arguments_start);

    if (data->data != NULL)
        free(data->data);

    data->data = malloc(strlen(new_data) + 1);

    if (data->data == NULL)
        return luaL_error(L, "out of memory");

    strcpy(data->data, new_data);

    return 0;
}

struct eeprom_data *eeprom_init(void) {
    struct eeprom_data *data = malloc(sizeof(struct eeprom_data));
    assert(data != NULL);

    data->contents = NULL;
    data->data = NULL;
    
    struct component *eeprom = new_component("eeprom", new_uuid(), data);
    add_method(eeprom, "get", eeprom_get, METHOD_DIRECT | METHOD_GETTER);
    // set
    // getLabel
    // setLabel
    // getSize
    // getDataSize
    add_method(eeprom, "getData", eeprom_get_data, METHOD_DIRECT | METHOD_SETTER);
    add_method(eeprom, "setData", eeprom_set_data, METHOD_DIRECT | METHOD_SETTER);
    // getChecksum
    // makeReadonly
    add_component(eeprom);

    return data;
}
