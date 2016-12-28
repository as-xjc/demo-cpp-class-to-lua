#include <iostream>
#include <lua.hpp>

#define REG_CLASS(class, name) {#name, class::name}

template <typename T> T* ToUserDataPtr(lua_State* L, int index)
{
    void** p = reinterpret_cast<void**>(lua_touserdata(L, index));
    if (p == nullptr) return nullptr;

    T* t = reinterpret_cast<T*>(*p);

    return t;
}

class Pet
{
public:
    Pet():_age(10) {}
    int GetAge() { return _age;}
    void SetAge(int age) { _age = age;}

private:
    int _age;
};

class LuaPet
{
public:
    static int GetAge(lua_State* L) {
        Pet* pet = ToUserDataPtr<Pet>(L, 1);
        lua_pushnumber(L, pet->GetAge());
        return 1;
    }
    static int SetAge(lua_State* L) {
        Pet* pet = ToUserDataPtr<Pet>(L, 1);
        lua_Number age = lua_tonumber(L, 2);
        pet->SetAge(age);
        return 0;
    }
};

luaL_Reg PetLib[] = {
        REG_CLASS(LuaPet, GetAge),
        REG_CLASS(LuaPet, SetAge),
        {0,0}
};

static void set(lua_State *L, int table_index, const char *key)
{
    lua_pushstring(L, key);
    lua_insert(L, -2);  // swap value and key
    lua_settable(L, table_index);
}

/* 创建完class的metatable后，lua层加入的东西
 * _G[className] = {
 * "new" = 略,
 * __metatable = {},
 * }
 *
 * className(metatable) = {
 * __metatable = _G[className],
 * __index = _G[className],
 * __tostring = 略，
 * __gc = 略,
 * __class = 略,
 * }
 */
void createClassMeta(lua_State*L, const char* className, luaL_Reg libs[])
{
    int index = lua_gettop(L);

    lua_newtable(L);
    int tbi = lua_gettop(L);
    for (int i = 0; libs[i].name != 0; i++) {
        luaL_Reg reg = libs[i];
        lua_pushstring(L, reg.name);
        lua_pushcfunction(L, reg.func);
        lua_settable(L, tbi);
    }

    lua_pushvalue(L, tbi);
    lua_setglobal(L, className);

    luaL_newmetatable(L, className);
    int metai = lua_gettop(L);

    lua_pushvalue(L, tbi);
    set(L, metai, "__metatable");
    lua_pushvalue(L, tbi);
    set(L, metai, "__index");

    lua_pop(L, lua_gettop(L)-index);
}

static int report(lua_State *L, int status)
{
    if (status) {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL) msg = "(error with no message)";
        fprintf(stderr, "ERROR: %s/n", msg);
        lua_pop(L, 1);
    }
    return status;
}

void Lua_Pet(lua_State* L, Pet* pet)
{
    void** p = reinterpret_cast<void**>(lua_newuserdata(L, sizeof(void*)));
    *p = reinterpret_cast<void*>(pet);
    luaL_getmetatable(L, "Pet");
    lua_setmetatable(L, -2);
}

int main(int argc, char* argv[]) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    createClassMeta(L, "Pet", PetLib);

    lua_pushnumber(L, 50);
    lua_setglobal(L, "lua_engine");

    report(L, luaL_dofile(L, "run.lua"));
    lua_getglobal(L, "run");
    Lua_Pet(L, new Pet());
    Lua_Pet(L, new Pet());
    report(L, lua_pcall(L, 2, 0, 0));

    lua_close(L);
    return 0;
}