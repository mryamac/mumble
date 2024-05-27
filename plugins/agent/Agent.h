#pragma once
#include <thread>
#include <zmq.hpp>

#include "Commands.h"
#include "Config.h"

#include "mumble.pb.h"

typedef int32_t mumble_channelid_t;
typedef uint32_t mumble_userid_t;
int setUserToChannel(mumble_channelid_t channelID);
int linkChannels(std::vector< mumble_channelid_t > channels);
int unlinkChannels(std::vector< mumble_channelid_t > channels);
int startListeningToChannel(std::vector< mumble_channelid_t > channels);
int stopListeningToChannel(std::vector< mumble_channelid_t > channels);
int whisperToUser(mumble_userid_t user);
int unwhisperToUser(mumble_userid_t user);
int getCurrentUserName(const char **userName);
int requestLocalUserMutestate(bool mute);

class Agent {
private:
	Config defaultConfig;
	bool shutdown;
	std::thread pluginThread;

	std::unique_ptr< zmq::context_t > context;
	std::unique_ptr< zmq::socket_t > socket;

	void loop();
	bool sendMessage(const char *command);
	bool sendMessage(const char *command, const char *arg);

public:
	static Agent *instance;
	Agent(/* args */);
	~Agent();

	void initialize();
	void terminate();
	void startLoop();
	void stopLoop();
};
