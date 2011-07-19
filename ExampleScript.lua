require("LuaBASS")
require("socket")
LuaBASS.Init(1,44100,0)
local Handle = LuaBASS.StreamCreateFile(
	"C:/Music/SortMe/Tobacco - Maniac Meat (2010) [V0]/02 Fresh Hex (Feat. Beck).mp3",
	0,
	0,
	LuaBASS.Flags(
		LuaBASS.STREAM.BASS_STREAM_PRESCAN,
		LuaBASS.SAMPLE.BASS_SAMPLE_LOOP
	)
)
	
local PlayCount = 0
local ByteLength = LuaBASS.ChannelGetLength(Handle,LuaBASS.POS.BASS_POS_BYTE)
local Seconds = math.floor(LuaBASS.ChannelBytes2Seconds(Handle,ByteLength))
local SyncHandle = LuaBASS.ChannelSetSync(Handle,2,0)
local LastPos = 0
LuaBASS.ChannelPlay(Handle,false)
print("Playing")
repeat
	socket.sleep(1)
	local Events = LuaBASS.GetSyncEventList()
	for Key,Event in pairs(Events) do
		print(Key,Event.handle,Event.channel,Event.Data)
		if (Event.handle == SyncHandle) then
			PlayCount = PlayCount + 1
			print("Plays:", PlayCount)
		end
	end
	local CurPos = math.floor(LuaBASS.ChannelBytes2Seconds(Handle,LuaBASS.ChannelGetPosition(Handle,LuaBASS.POS.BASS_POS_BYTE)))
	if (CurPos ~= LastPos) then
		print(os.date("%M:%S",CurPos) .."/".. os.date("%M:%S",Seconds))
	end
	LastPos = CurPos
until PlayCount == 2
LuaBASS.Free()