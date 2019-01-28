#include <iostream>
#include <lua.hpp>
#include <map>
#include <string>
#include <model.h>

#include <string>
#include <direct.h> //_mkdir������ͷ�ļ�
#include <io.h>     //_access������ͷ�ļ�

using namespace std;

void CreateDir(string str1)
{
	int m = 0, n;
	string str2;

	INT i = static_cast<INT>(str1.rfind("\\"));
	if (i != std::string::npos)
	{
		str1 = str1.substr(0, i);
	}


	str2 = str1.substr(0, 2);
	str1 = str1.substr(3, str1.size());

	while (m >= 0)
	{
		m = str1.find('\\');

		str2 += '\\' + str1.substr(0, m);
		n = _access(str2.c_str(), 0); //�жϸ�Ŀ¼�Ƿ����
		if (n == -1)
		{
			_mkdir(str2.c_str());     //����Ŀ¼
		}

		str1 = str1.substr(m + 1, str1.size());
	}
}


std::map<int, MODEL*> model_addr_map;

int open_model(lua_State* L)
{
	if (!lua_isstring(L, 1))
	{
		lua_pushnil(L);
		return 1;
	}
		
	std::string FileName = lua_tostring(L, 1);
	BUFFER Buffer;

	if (!FileLoader.Load(FileName, Buffer))
	{
		lua_pushnil(L);
		return 1;
	}
	MODEL* model = new MODEL;
	if (!ResourceLoader.LoadModel(*model, FileName, Buffer))
	{
		delete model;
		lua_pushnil(L);
		return 1;
	}
	int id = (int)model;
	model_addr_map[id] = model;
	
	lua_pushinteger(L, id);
	return 1;
}

int close_model(lua_State* L)
{
	if (!lua_isinteger(L, 1))
		return 0;
	int id = lua_tointeger(L, 1);
	auto it = model_addr_map.find(id);
	if (it == model_addr_map.end())
		return 0;
	delete(it->second);
	model_addr_map.erase(it);

	return 0;
}

int save_model(lua_State* L)
{
	if (!lua_isinteger(L, 1))
		return 0;
	if (!lua_isstring(L, 2))
		return 0;

	int id = lua_tointeger(L, 1);
	const char* path = lua_tostring(L, 2);

	auto it = model_addr_map.find(id);
	if (it == model_addr_map.end())
		return 0;
	string out_path = Common.GetProgramDirectory() + "\\model\\" + path;
	CreateDir(out_path);

	MODEL* model = it->second;
	BUFFER Buffer;

	if (!ResourceLoader.SaveModel(*model, out_path, Buffer))
	{
		printf("save error %s\n", path);
		return 0;
	}
	if (!FileLoader.SaveToFile(out_path, Buffer))
	{
		printf("save write error %s\n", path);
		return 0;
	}
	
	return 0;
}

int load_model_animate(lua_State* L)
{
	if (!lua_isinteger(L, 1))
	{
		lua_pushnil(L);
		return 1;
	}
		
	int id = lua_tointeger(L, 1);
	auto it = model_addr_map.find(id);
	if (it == model_addr_map.end())
	{
		lua_pushnil(L);
		return 1;
	}
	MODEL* model = it->second;
	auto& sequences = model->Data().SequenceContainer;
	char buffer[0x100];
	lua_newtable(L);
	for (int i = 0; i < sequences.GetTotalSize(); i++)
	{
		auto& data = sequences[i]->Data();

		lua_newtable(L);
		lua_pushstring(L, data.Name.c_str());
		lua_setfield(L, -2, "name");

		lua_pushinteger(L, data.Interval.x);
		lua_setfield(L, -2, "start");

		lua_pushinteger(L, data.Interval.y);
		lua_setfield(L, -2, "end");

		lua_rawseti(L, -2, i);
	}
	return 1;
}



int open(lua_State* L)
{
	lua_newtable(L);
	{
		lua_newtable(L);
		{

		}
		lua_setmetatable(L, -2);

		{
			luaL_Reg func[] = {
				{ "open_model", open_model },
				{ "close_model", close_model },
				{ "load_animate", load_model_animate },
				{ "save_model", save_model },
				{ NULL, NULL },
			};
			luaL_setfuncs(L, func, 0);
		}


	}
	return 1;
}

bool InitEditor()
{
	if (!Register.FindWarcraftDirectory())
		return FALSE;
	Graphics.Setup();

	BOOL Result = FALSE;
	std::string FileName;
	std::string ErrorMessage;
	std::string WarcraftDirectory;

	WarcraftDirectory = Register.GetWarcraftDirectory();

	while (TRUE)
	{
		FileName = WarcraftDirectory + "\\" + PATH_MPQ_WAR3;
		if (!MpqWar3.Open(FileName)) break;

		FileName = WarcraftDirectory + "\\" + PATH_MPQ_WAR3X;
		if (!MpqWar3x.Open(FileName)) break;

		FileName = WarcraftDirectory + "\\" + PATH_MPQ_WAR3X_LOCAL;
		if (!MpqWar3xLocal.Open(FileName)) break;

		FileName = WarcraftDirectory + "\\" + PATH_MPQ_WAR3_PATCH;
		if (!MpqWar3Patch.Open(FileName)) break;

		Result = TRUE;
		break;
	}

	if (!Result)
	{
		ErrorMessage = "Unable to open \"" + FileName + "\"!\n\n";
		ErrorMessage += "Make sure that Warcraft 3 is installed and that the registry key\n";
		ErrorMessage += "\"HKEY_CURRENT_USER\\Blizzard Entertainment\\Warcraft III\\InstallPath\" exists!";
		Error.SetMessage(ErrorMessage);
		return FALSE;
	}

	ResourceLoader.RegisterAllLoaders();
	

}
int main()
{
	
	if (!InitEditor())
	{
		printf("�༭����ʼ��ʧ��\n");
		system("pause");
		return 0;
	}
	

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	lua_getfield(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
	lua_pushcclosure(L, open, 0);
	lua_setfield(L, -2, "model.core");
	lua_pop(L, 1);

	lua_getglobal(L, "require");
	lua_pushstring(L, "script.main");
	lua_pcall(L, 1, 0,0);

	system("pause");
	return 0;
}