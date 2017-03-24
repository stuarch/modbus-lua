#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <modbus.h>

typedef struct {
    lua_State *L;
    modbus_t *modbus;
} ctx_t;

void l_pushtable_number(lua_State *L, int key, long int value){
    lua_pushnumber(L, key);
    lua_pushnumber(L, value);
    lua_settable(L ,-3);
}

static int l_version(lua_State *L)
{
    lua_pushstring(L, "0.0.2");
    return 1;
}

static int l_init(lua_State *L){
    ctx_t *ctx;
    ctx = (ctx_t *)lua_newuserdata(L, sizeof(ctx_t));
    
    luaL_getmetatable(L, "modbus");
    lua_setmetatable(L, -2);

    const char *host = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);

    ctx->modbus = modbus_new_tcp(host, port);
    if(ctx->modbus == NULL){
        fprintf(stderr, "Init Error: %s\n", modbus_strerror(errno));
        return -1;
    }
    ctx->L = L;

    return 1;
}

static int l_connect(lua_State *L){
    ctx_t *ctx=(ctx_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
    
    if(modbus_connect(ctx->modbus)== -1){
        fprintf(stderr, "Connect Failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx->modbus);
        return -1;
    }
    printf("Connect Successed!\n");
    return 1;
}

static int l_close(lua_State *L){
    ctx_t *ctx=(ctx_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
    
    modbus_close(ctx->modbus);
    modbus_free(ctx->modbus);

    lua_newtable(L);
    lua_setmetatable(L, -2);
    printf("Disconnect!\n");
    return 1;
}

static int l_slave(lua_State *L){
    ctx_t *ctx=(ctx_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
    
    int slave = luaL_checkinteger(L, 2);
    modbus_set_slave(ctx->modbus, slave);
    return 1;
}

static int l_read(lua_State *L){
    ctx_t *ctx=(ctx_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");

    int cnt = lua_objlen(L, -1);
    int addr[cnt];
    uint16_t *buf = malloc(sizeof(uint16_t)*cnt);
   
    int i=0;
    lua_pushnil(L);
    while(lua_next(L, -2)){
        lua_pushvalue(L, -2);
        addr[i] = luaL_checknumber(L, -2);
        lua_pop(L, 2);
        i++;
    }

    lua_newtable(L);
    for(i=0; i<cnt; i++){
        if(modbus_read_registers(ctx->modbus, addr[i], 1, buf) == -1){
            fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
            return -1;
        } 
        if(buf==NULL){
            fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
            return -1;
        }
        l_pushtable_number(L, addr[i], (long int)buf);
    }
    free(buf);
    free(addr);
    return 1;
}

static int l_mread(lua_State *L){
    ctx_t *ctx=(ctx_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
   
    int addr = luaL_checkinteger(L, 2);
    int cnt = luaL_checkinteger(L, 3);
    uint16_t *buf = malloc(sizeof(uint16_t));
    if(modbus_read_registers(ctx->modbus, addr, cnt, buf) == -1){
        fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
        return -1;
    } 
    if(buf==NULL){
        fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
        return -1;
    }
    lua_pushinteger(L, (long int)buf);
    return 1;
}

static int l_write(lua_State *L){
    ctx_t *ctx=(ctx_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");

    int addr = luaL_checkint(L, 2);
    int value = luaL_checkint(L, 3);

    if(modbus_write_register(ctx->modbus, addr, value) == -1){
        fprintf(stderr, "Write Error: invalid action\n");
        return -1;
    }

    printf("%x Write Successed!\n", addr);
    return 1;
}

static const struct luaL_Reg modbus_func[] = {
    {"connect", l_connect},
    {"close", l_close},
    {"read", l_read},
    {"mread", l_mread},
    {"write", l_write},
    {"slave", l_slave},
    {NULL, NULL},
};

static const struct luaL_Reg modbus_lib[] = {
    {"version", l_version},
    {"new", l_init},
    {NULL, NULL},
};

int luaopen_modbus(lua_State *L){
    luaL_newmetatable(L, "modbus");
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");
    luaL_register(L, NULL, modbus_func);
    luaL_register(L, "modbus", modbus_lib);
    return 1;
}
