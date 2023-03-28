#include <iostream>
#include <filesystem>
#include <thread>
#include <memory>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

using namespace std;

struct Entry {
	std::unique_ptr<std::thread> main_thread;
	lua_State* L = nullptr;

	void start(const char* arg);
	void wait();
	static int l_set_timer(lua_State* L);
};

int main(int argc, char** argv)
{
	Entry e[3];
	for (int i = 0; i < 3; ++i) {
		e[i].start(argv[0]);
	}
	for (int i = 0; i < 3; ++i) {
		e[i].wait();
	}
	return 0;
}

void Entry::start(const char* arg)
{
	auto p = std::tr2::sys::path(arg);
	auto script = p.parent_path().string() + "/test.lua";

	L = luaL_newstate();
	luaL_openlibs(L);
	lua_pushlightuserdata(L, this);
	lua_setglobal(L, "Entry");
	lua_register(L, "set_timer", l_set_timer);

	if (luaL_loadfile(L, script.c_str())) {
		cout << "load err:" << lua_tostring(L, -1) << endl;
		lua_close(L);
		L = nullptr;
		return;
	}

	if (lua_pcall(L, 0, 0, 0)) {
		cout << "pcall err:" << lua_tostring(L, -1) << endl;
		lua_close(L);
		L = nullptr;
		return;
	}
}

void Entry::wait()
{
	if (main_thread) {
		main_thread->join();
	}
}

int Entry::l_set_timer(lua_State* L)
{
	lua_getglobal(L, "Entry");
	Entry* e = (Entry*)lua_touserdata(L, -1);
	if (e->main_thread || !e->L) {
		return 0;
	}
	std::string f = lua_tostring(L, 1);
	int diff = lua_tonumber(L, 2);
	int count = lua_tonumber(L, 3);
	e->main_thread = std::make_unique<std::thread>([e,f,diff,count](){
		// 保证l_set_timer 退出了
		std::this_thread::sleep_for(std::chrono::seconds(5));
		for (int i = 0; i < count; ++i)
		{
			lua_settop(e->L, 0);
			lua_getglobal(e->L, f.c_str());
			lua_pcall(e->L, 0, 0, 0);
			std::this_thread::sleep_for(std::chrono::seconds(diff));
		}
	});
}
