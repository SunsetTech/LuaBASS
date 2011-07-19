// LuaBASS.cpp : Defines the exported functions for the DLL application.
//
/*
Copyright (c) 2011 Brachyonic

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "stdafx.h"
#include <stdio.h>
#include "bass.h"
extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
}

#define PROJECT_TABLENAME "LuaBASS"
#ifdef WIN32
#define LUA_API __declspec(dllexport)
#else
#define LUA_API
#endif
#define M_FIELDSET(FIELDSET_LUASTATE,FIELDSET_TYPE,FIELDSET_INDEX,FIELDSET_KEY,FIELDSET_VALUE) lua_push##FIELDSET_TYPE##(FIELDSET_LUASTATE,FIELDSET_VALUE); lua_setfield(FIELDSET_LUASTATE,FIELDSET_INDEX - 1,FIELDSET_KEY);
#define M_TABLESET(TABLESET_LUASTATE,TABLESET_KTYPE,TABLESET_VTYPE,TABLESET_INDEX,TABLESET_KEY,TABLESET_VALUE) lua_push##TABLESET_KTYPE##(L,TABLESET_KEY); lua_push##TABLESET_VTYPE##(L,TABLESET_VALUE); lua_settable(L,TABLESET_INDEX - 2);
#define M_EXPORT(EXPORT_NAME) {#EXPORT_NAME,EXPORT_NAME},
#define M_ENUM(ENUM_LUASTATE, ENUM_INDEX, ENUM_NAME) M_FIELDSET(ENUM_LUASTATE, integer, ENUM_INDEX, #ENUM_NAME, ENUM_NAME) M_TABLESET(ENUM_LUASTATE,integer,string,ENUM_INDEX,ENUM_NAME,#ENUM_NAME)
#define M_RETURN(RETURN_LUASTATE, RETURN_NUM) lua_pushinteger( RETURN_LUASTATE , BASS_ErrorGetCode() ); \
	return RETURN_NUM + 1;

typedef struct {
	HSYNC handle;
	DWORD channel;
	DWORD data;
} SyncEventData;

extern "C" {
	int LUA_API luaopen_LuaBASS (lua_State *L);
}

HANDLE SyncMutex;
SyncEventData EventDataList[1024];
int NextEmptyEDLIndex = 0;

static int GetDeviceCount(lua_State *L) {
	int a, count=0;
	BASS_DEVICEINFO Info;
	for (a=0; BASS_GetDeviceInfo(a, &Info); a++)
		if (Info.flags&BASS_DEVICE_ENABLED)
			count++;
	lua_pushinteger(L,count);
	return 1;
}
static int Flags(lua_State *L) {
	int NumFlags = lua_gettop(L);
	int FlagAccum = 0;
	printf("%d\n",NumFlags);
	for (int i = 1; i <= NumFlags; i++) {
		FlagAccum = FlagAccum | lua_tointeger(L,i);
		printf("%d %d\n",FlagAccum,lua_tointeger(L,i));
	}
	lua_pushinteger(L,FlagAccum);
	return 1;
}
static int GetSyncEventList(lua_State *L) {
	DWORD WaitResult = WaitForSingleObject(SyncMutex,INFINITE);
	lua_newtable(L); //return table
	for (int i = 0; i < NextEmptyEDLIndex; i++)
	{
		SyncEventData EventData = EventDataList[i];
		lua_pushinteger(L,i);
		lua_newtable(L); //EventData table
		M_FIELDSET(L,integer,-1,"handle",EventData.handle)
		M_FIELDSET(L,integer,-1,"channel",EventData.channel)
		M_FIELDSET(L,integer,-1,"data",EventData.data)
		lua_settable(L,-3);
	}
	NextEmptyEDLIndex = 0;
	ReleaseMutex(SyncMutex);
	return 1;
}
void CALLBACK SyncDispatcher(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	DWORD WaitResult = WaitForSingleObject(SyncMutex,INFINITE);
	SyncEventData EventData = {handle,channel,data};
	EventDataList[NextEmptyEDLIndex] = EventData;
	NextEmptyEDLIndex++;
	ReleaseMutex(SyncMutex);
	/*
	lua_State *L = (lua_State*) user;
	lua_getfield(L,LUA_GLOBALSINDEX,"package");
	lua_getfield(L,-1,"loaded");
	lua_remove(L,-2);
	lua_getfield(L,-1,PROJECT_TABLENAME);
	lua_remove(L,-2);
	lua_getfield(L,-1,"SyncCallback");
	lua_remove(L,-2);
	if (lua_isfunction(L,-1)) {
		lua_pushinteger(L,handle);
		lua_pushinteger(L,channel);
		lua_pushinteger(L,data);
		lua_call(L,3,0);
	}*/
}
static int Init(lua_State *L) {
	lua_pushboolean(L,BASS_Init(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3),0,0));
	M_RETURN(L,1)
}

static int GetDeviceInfo(lua_State *L) {
	DWORD Device = lua_tointeger(L,1);
	BASS_DEVICEINFO Info;
	BOOL Success = BASS_GetDeviceInfo(Device, &Info);
	lua_pushboolean(L,Success);
	if (Success) {
		lua_newtable(L);
		M_FIELDSET(L,string,-2,"name",Info.name)
		M_FIELDSET(L,string,-2,"driver",Info.driver)
		M_FIELDSET(L,integer,-2,"flags",Info.flags)
	}
	else {
		lua_pushnil(L);
	}
	M_RETURN(L,2)
}

static int Free(lua_State *L) {
	lua_pushboolean(L,BASS_Free());
	M_RETURN(L,1)
}

static int GetDevice(lua_State *L) {
	lua_pushinteger(L,BASS_GetDevice());
	M_RETURN(L,1)
}

static int SetDevice(lua_State *L) {
	lua_pushboolean(L,BASS_SetDevice(lua_tonumber(L,1)));
	M_RETURN(L,1)
}

static int GetInfo(lua_State *L) {
	BASS_INFO Info;
	BOOL Success = BASS_GetInfo(&Info);
	lua_pushboolean(L,Success);
	if (Success) {
		lua_newtable(L);
		M_FIELDSET(L,integer,-1,"flags",Info.flags)
		M_FIELDSET(L,integer,-1,"hwsize",Info.hwsize)
		M_FIELDSET(L,integer,-1,"hwfree",Info.hwfree)
		M_FIELDSET(L,integer,-1,"freesam",Info.freesam)
		M_FIELDSET(L,integer,-1,"free3d",Info.free3d)
		M_FIELDSET(L,integer,-1,"minrate",Info.minrate)
		M_FIELDSET(L,integer,-1,"maxrate",Info.maxrate)
		M_FIELDSET(L,boolean,-1,"eax",Info.eax)
		M_FIELDSET(L,integer,-1,"minbuf",Info.minbuf)
		M_FIELDSET(L,integer,-1,"dsver",Info.dsver)
		M_FIELDSET(L,integer,-1,"latency",Info.latency)
		M_FIELDSET(L,integer,-1,"initflags",Info.initflags)
		M_FIELDSET(L,integer,-1,"speakers",Info.speakers)
		M_FIELDSET(L,integer,-1,"freq",Info.freq)
	}
	else {
		lua_pushnil(L);
	}
	M_RETURN(L,2)
}

static int GetVersion(lua_State *L) {
	lua_pushinteger(L,BASS_GetVersion());
	M_RETURN(L,1)
}

static int GetVolume(lua_State *L) {
	lua_pushnumber(L,BASS_GetVolume());
	M_RETURN(L,1)
}

static int Pause(lua_State *L) {
	lua_pushboolean(L,BASS_Pause());
	M_RETURN(L,1)
}

static int SetVolume(lua_State *L) {
	lua_pushboolean(L,BASS_SetVolume(lua_tonumber(L,1)));
	M_RETURN(L,1)
}

static int Start(lua_State *L) {
	lua_pushboolean(L,BASS_Start());
	M_RETURN(L,1)
}

static int Stop(lua_State *L) {
	lua_pushboolean(L,BASS_Stop());
	M_RETURN(L,1)
}

static int Update(lua_State *L) {
	lua_pushboolean(L,BASS_Update(lua_tointeger(L,1)));
	M_RETURN(L,1)
}

static int GetCPU(lua_State *L) {
	lua_pushnumber(L,BASS_GetCPU());
	M_RETURN(L,1)
}

static int StreamCreateFile(lua_State *L) {
	HSTREAM Handle = BASS_StreamCreateFile(false,lua_tostring(L,1),lua_tointeger(L,2),lua_tointeger(L,3),lua_tointeger(L,4));
	lua_pushinteger(L,Handle);
	M_RETURN(L,1)
}

static int ChannelPlay(lua_State *L) {
	lua_pushboolean(L,BASS_ChannelPlay(lua_tointeger(L,1),lua_toboolean(L,2)));
	M_RETURN(L,1)
}

static int ChannelStop(lua_State *L) {
	lua_pushboolean(L,BASS_ChannelStop(lua_tointeger(L,1)));
	M_RETURN(L,1)
}

static int ChannelPause(lua_State *L) {
	lua_pushboolean(L,BASS_ChannelPause(lua_tointeger(L,1)));
	M_RETURN(L,1)
}

static int ChannelSetSync(lua_State *L) {
	lua_pushinteger(L,BASS_ChannelSetSync(lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3), &SyncDispatcher,NULL));
	M_RETURN(L,1)
}

static int ChannelGetPosition(lua_State *L) {
	lua_pushinteger(L,BASS_ChannelGetPosition(lua_tointeger(L,1),lua_tointeger(L,2)));
	M_RETURN(L,1)
}

static int ChannelGetLength(lua_State *L) {
	lua_pushinteger(L,BASS_ChannelGetLength(lua_tointeger(L,1),lua_tointeger(L,2)));
	M_RETURN(L,1)
}

static int ChannelBytes2Seconds(lua_State *L) {
	lua_pushnumber(L,BASS_ChannelBytes2Seconds(lua_tointeger(L,1),lua_tointeger(L,2)));
	M_RETURN(L,1)
}

int LUA_API luaopen_LuaBASS (lua_State *L) {
	SyncMutex = CreateMutex(NULL,FALSE,NULL);
	struct luaL_reg driver[] = {
		M_EXPORT(Free)
		M_EXPORT(GetDevice)
		M_EXPORT(GetDeviceInfo)
		M_EXPORT(GetInfo)
		M_EXPORT(GetVersion)
		M_EXPORT(GetVolume)
		M_EXPORT(Init)
		M_EXPORT(Pause)
		M_EXPORT(SetDevice)
		M_EXPORT(SetVolume)
		M_EXPORT(Start)
		M_EXPORT(Stop)
		M_EXPORT(Update)
		M_EXPORT(StreamCreateFile)
		M_EXPORT(ChannelPlay)
		M_EXPORT(ChannelStop)
		M_EXPORT(ChannelPause)
		M_EXPORT(ChannelSetSync)
		M_EXPORT(ChannelGetPosition)
		M_EXPORT(ChannelGetLength)
		M_EXPORT(ChannelBytes2Seconds)
		M_EXPORT(Flags)
		M_EXPORT(GetSyncEventList)
		{NULL, NULL},
	};
	luaL_register (L, "LuaBASS", driver);
	lua_newtable(L); //ERROR
	M_ENUM(L,-1,BASS_OK)
	M_ENUM(L,-1,BASS_ERROR_MEM)
	M_ENUM(L,-1,BASS_ERROR_FILEOPEN)
	M_ENUM(L,-1,BASS_ERROR_DRIVER)
	M_ENUM(L,-1,BASS_ERROR_BUFLOST)
	M_ENUM(L,-1,BASS_ERROR_HANDLE)
	M_ENUM(L,-1,BASS_ERROR_FORMAT)
	M_ENUM(L,-1,BASS_ERROR_POSITION)
	M_ENUM(L,-1,BASS_ERROR_INIT)
	M_ENUM(L,-1,BASS_ERROR_START)
	M_ENUM(L,-1,BASS_ERROR_ALREADY)
	M_ENUM(L,-1,BASS_ERROR_NOCHAN)
	M_ENUM(L,-1,BASS_ERROR_ILLTYPE)
	M_ENUM(L,-1,BASS_ERROR_ILLPARAM)
	M_ENUM(L,-1,BASS_ERROR_NO3D)
	M_ENUM(L,-1,BASS_ERROR_NOEAX)
	M_ENUM(L,-1,BASS_ERROR_DEVICE)
	M_ENUM(L,-1,BASS_ERROR_NOPLAY)
	M_ENUM(L,-1,BASS_ERROR_FREQ)
	M_ENUM(L,-1,BASS_ERROR_NOTFILE)
	M_ENUM(L,-1,BASS_ERROR_NOHW)
	M_ENUM(L,-1,BASS_ERROR_EMPTY)
	M_ENUM(L,-1,BASS_ERROR_NONET)
	M_ENUM(L,-1,BASS_ERROR_CREATE)
	M_ENUM(L,-1,BASS_ERROR_NOFX)
	M_ENUM(L,-1,BASS_ERROR_NOTAVAIL)
	M_ENUM(L,-1,BASS_ERROR_DECODE)
	M_ENUM(L,-1,BASS_ERROR_DX)
	M_ENUM(L,-1,BASS_ERROR_TIMEOUT)
	M_ENUM(L,-1,BASS_ERROR_FILEFORM)
	M_ENUM(L,-1,BASS_ERROR_SPEAKER)
	M_ENUM(L,-1,BASS_ERROR_VERSION)
	M_ENUM(L,-1,BASS_ERROR_CODEC)
	M_ENUM(L,-1,BASS_ERROR_ENDED)
	M_ENUM(L,-1,BASS_ERROR_UNKNOWN)
	lua_setfield(L,-2,"ERROR");
	lua_newtable(L); //DEVICE
	M_ENUM(L,-1,BASS_DEVICE_8BITS)
	M_ENUM(L,-1,BASS_DEVICE_MONO)
	M_ENUM(L,-1,BASS_DEVICE_3D)
	M_ENUM(L,-1,BASS_DEVICE_LATENCY)
	M_ENUM(L,-1,BASS_DEVICE_CPSPEAKERS)
	M_ENUM(L,-1,BASS_DEVICE_SPEAKERS)
	M_ENUM(L,-1,BASS_DEVICE_NOSPEAKER)
	lua_setfield(L,-2,"DEVICE");
	lua_newtable(L); //STREAM
	M_ENUM(L,-1,BASS_STREAM_PRESCAN)
	M_ENUM(L,-1,BASS_STREAM_AUTOFREE)
	M_ENUM(L,-1,BASS_STREAM_DECODE)
	lua_setfield(L,-2,"STREAM");
	lua_newtable(L); //SAMPLE
	M_ENUM(L,-1,BASS_SAMPLE_FLOAT)
	M_ENUM(L,-1,BASS_SAMPLE_MONO)
	M_ENUM(L,-1,BASS_SAMPLE_SOFTWARE)
	M_ENUM(L,-1,BASS_SAMPLE_3D)
	M_ENUM(L,-1,BASS_SAMPLE_LOOP)
	M_ENUM(L,-1,BASS_SAMPLE_FX)
	lua_setfield(L,-2,"SAMPLE");
	lua_newtable(L); //MISC
	M_ENUM(L,-1,BASS_UNICODE)
	lua_setfield(L,-2,"MISC");
	lua_newtable(L); //POS
	M_ENUM(L,-1,BASS_POS_BYTE)
	M_ENUM(L,-1,BASS_POS_MUSIC_ORDER)
	M_ENUM(L,-1,BASS_POS_DECODE)
	lua_setfield(L,-2,"POS");
	return 1;
}