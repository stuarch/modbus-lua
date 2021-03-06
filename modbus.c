#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <modbus.h>

void l_pushtable(lua_State *L, int key, void *value, char *vtype){
    lua_pushnumber(L, key);

    if(strcmp(vtype, "number")==0){
        double *buf = value;
        lua_pushnumber(L, *buf);
    }else if(strcmp(vtype, "integer")==0){
        long int *buf = value;
        lua_pushinteger(L, *buf);
    }else if(strcmp(vtype, "boolean")==0){
        int *buf = value;
        lua_pushboolean(L, *buf);
    }else if(strcmp(vtype, "string")==0){
        lua_pushstring(L, (char *)value);
    }else{
        printf("Get NULL value\n");
        lua_pushnil(L);
    }
    lua_settable(L ,-3);
}

static int l_version(lua_State *L)
{
    lua_pushstring(L, "0.0.3");
    return 1;
}

static int l_init(lua_State *L){
    const char *host = lua_tostring(L, 1);
    int port = lua_tointeger(L, 2);
    lua_pop(L, 2);
    
    modbus_t *ctx;
    ctx = (modbus_t *)lua_newuserdata(L, sizeof(ctx));
    
    luaL_getmetatable(L, "modbus");
    lua_setmetatable(L, -2);

    ctx = modbus_new_tcp(host, port);
    if(ctx == NULL){
        fprintf(stderr, "Init Error: %s\n", modbus_strerror(errno));
        return -1;
    }
    return 1;
}

static int l_connect(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
    
    if(modbus_connect(ctx)== -1){
        fprintf(stderr, "Connect Failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
    printf("Connect Successed!\n");
    return 1;
}

static int l_close(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
    
    modbus_close(ctx);
    modbus_free(ctx);

    lua_pop(L, lua_gettop(L));
    printf("Disconnect!\n");
    return 1;
}

static int l_slave(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
    
    int slave = lua_tointeger(L, 2);
    lua_pop(L, 1);
    modbus_set_slave(ctx, slave);
    return 1;
}

static int l_read(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");

    int cnt = lua_objlen(L, -1);
    int addr[cnt];
    uint16_t buf[sizeof(uint16_t)];

    int i=0;
    lua_pushnil(L);
    while(lua_next(L, -2)){
        lua_pushvalue(L, -2);
        addr[i] = lua_tointeger(L, -2);
        lua_pop(L, 2);
        i++;
    }
    lua_pop(L, 1);

    lua_newtable(L);
    for(i=0; i<cnt; i++){
        if(modbus_read_registers(ctx, addr[i], 1, buf) == -1){
            fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
            return -1;
        }
        if(buf==NULL){
            fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
            return -1;
        }
        long int res = (long int)*buf;
        l_pushtable(L, addr[i], &res, "integer");
        //printf("%d,%ld\n", addr[i], (long int)buf);
    }
    return 1;
}

static int l_mread(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");
   
    int addr = lua_tointeger(L, 2);
    int cnt = lua_tointeger(L, 3);
    lua_pop(L, 2);

    uint16_t *buf[cnt*sizeof(uint16_t)];
    if(modbus_read_registers(ctx, addr, cnt, *buf) == -1){
        fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
        return -1;
    } 
    if(buf==NULL){
        fprintf(stderr, "Read Error: %s\n", modbus_strerror(errno));
        return -1;
    }
    lua_pushinteger(L, (long int)*buf);
    return 1;
}

static int l_mwrite(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");

    int addr = lua_tointeger(L, 2);
    int value = lua_tointeger(L, 3);
    lua_pop(L, 2);

    if(modbus_write_register(ctx, addr, value) == -1){
        fprintf(stderr, "Write Error: invalid action\n");
        return -1;
    }

    printf("%x Write Successed!\n", addr);
    return 1;
}

static int l_write(lua_State *L){
    modbus_t *ctx=(modbus_t *)luaL_checkudata(L, 1, "modbus");
    luaL_argcheck(L, ctx != NULL, 1, "Context Error");

    lua_pushvalue(L, -1);
    int cnt=0;
    lua_pushnil(L);
    while(lua_next(L, -2)){
        lua_pushvalue(L, -2);
        lua_pop(L, 2);
        cnt++;
    }
    lua_pop(L, 1);
    
    int addr[cnt];
    int value[cnt];

    int i=0;
    lua_pushnil(L);
    while(lua_next(L, -2)){
        lua_pushvalue(L, -2);
        addr[i] = lua_tointeger(L, -1);
        value[i] = lua_tointeger(L, -2);
        lua_pop(L, 2);
        i++;
    }
    lua_pop(L, 1);

    lua_newtable(L);
    int res = 0;
    for(i=0; i<cnt; i++){
        if(modbus_write_register(ctx, addr[i], value[i]) == -1){
            fprintf(stderr, "%d Write Error:%s\n", addr[i], modbus_strerror(errno));
            res = 0;
        }else{
            res = 1;
        }
        l_pushtable(L, addr[i], &res, "boolean");
    }
    return 1;
}

static const struct luaL_Reg modbus_func[] = {
    {"connect", l_connect},
    {"close", l_close},
    {"read", l_read},
    {"mread", l_mread},
    {"mwrite", l_mwrite},
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
