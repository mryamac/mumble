#pragma once

#define COMMAND_NAME(name) #name

enum HandlerCommands {
	NONE,
	CHANNEL,
	LINK,
	UNLINK,
	LISTEN,
	UNLISTEN,
	WHISPER,
	UNWHISPER,
	USERNAME,
	SEND,
	MUTE,
	UNMUTE,
};

enum SystemCommands {
	DATA,
	PING,
	PONG,
	SUCCESS,
	FAIL,
};