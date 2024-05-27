// Copyright 2021-2023 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Include the definitions of the plugin functions
// Not that this will also include ../PluginComponents.h
#include "../MumblePlugin.h"
#include "Agent.h"
#include "Logger.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

// These are just some utility functions facilitating writing logs and the like
// The actual implementation of the plugin is further down
std::ostream &pLog() {
	std::cout << "Agent Plugin: ";
	return std::cout;
}

template< typename T > void pluginLog(T log) {
	pLog() << log << std::endl;
}

std::ostream &operator<<(std::ostream &stream, const mumble_version_t version) {
	stream << "v" << version.major << "." << version.minor << "." << version.patch;
	return stream;
}


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////// PLUGIN IMPLEMENTATION ///////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

mumble_api_t mumAPI;
mumble_plugin_id_t ownID;

/*User Defined Variables*/
mumble_userid_t userID;
mumble_connection_t activeConnection;
const char *dataID = "AgentPluginID";

//////////////////////////////////////////////////////////////
//////////////////// OBLIGATORY FUNCTIONS ////////////////////
//////////////////////////////////////////////////////////////
// All of the following function must be implemented in order for Mumble to load the plugin

mumble_error_t mumble_init(uint32_t id) {
	ownID = id;
	logd(LOG_INFO, "Initializing Agent Plugin with ID: %d", id);
	Agent::instance = new Agent();
	Agent::instance->initialize();
	Agent::instance->startLoop();
	logd(LOG_INFO, "Plugin initialized with ID: %d", id);
	// MUMBLE_STATUS_OK is a macro set to the appropriate status flag (ErrorCode)
	// If you need to return any other status have a look at the ErrorCode enum
	// inside PluginComponents.h and use one of its values
	return MUMBLE_STATUS_OK;
}

void mumble_shutdown() {
	Agent::instance->stopLoop();
	Agent::instance->terminate();
	logd(LOG_INFO, "Plugin terminated with ID: %d", ownID);
}

MumbleStringWrapper mumble_getName() {
	static const char *name = "Agent Plugin";

	MumbleStringWrapper wrapper;
	wrapper.data = name;
	wrapper.size = strlen(name);
	// It's a static String and therefore doesn't need releasing
	wrapper.needsReleasing = false;

	return wrapper;
}

mumble_version_t mumble_getAPIVersion() {
	// MUMBLE_PLUGIN_API_VERSION will always contain the API version of the used header file (the one used to build
	// this plugin against). Thus you should always return that here in order to no have to worry about it.
	return MUMBLE_PLUGIN_API_VERSION;
}

void mumble_registerAPIFunctions(void *api) {
	// In this function the plugin is presented with a struct of function pointers that can be used
	// to interact with Mumble. Thus you should store it somewhere safe for later usage.

	// The pointer has to be cast to the respective API struct. You always have to cast to the same API version
	// as this plugin itself is using. Thus if this plugin is compiled using the API version 1.0.x (where x is an
	// arbitrary version) the pointer has to be cast to MumbleAPI_v_1_0_x (where x is a literal "x"). Furthermore the
	// struct HAS TO BE COPIED!!! Storing the pointer is not an option as it will become invalid quickly!

	// **If** you are using the same API version that is specified in the included header file (as you should), you
	// can simply use the MUMBLE_API_CAST to cast the pointer to the correct type and automatically dereferencing it.
	mumAPI = MUMBLE_API_CAST(api);

	pluginLog("Registered Mumble's API functions");
}

void mumble_releaseResource(const void *pointer) {
	std::cerr << "[ERROR]: Unexpected call to mumble_releaseResources" << std::endl;
	std::terminate();
	// This plugin doesn't use resources that are explicitly allocated (only static Strings are used). Therefore
	// we don't have to implement this function.
	//
	// If you allocated resources using malloc(), you're implementation for releasing that would be
	// free(const_cast<void *>(pointer));
	//
	// If however you allocated a resource using the new operator (C++ only), you have figure out the pointer's
	// original type and then invoke
	// delete static_cast<ActualType *>(pointer);

	// Mark as unused
	(void) pointer;
}

void mumble_onServerSynchronized(mumble_connection_t connection) {
	mumble_error_t result = MUMBLE_EC_API_REQUEST_TIMEOUT;
	while (result != MUMBLE_EC_OK) {
		result = mumAPI.getActiveServerConnection(ownID, &activeConnection);
	}

	result = mumAPI.getLocalUserID(ownID, activeConnection, &userID);
	if (result != MUMBLE_STATUS_OK) {
		mumAPI.log(ownID, "Failed to get local user id");
		return;
	}
}
//////////////////////////////////////////////////////////////
///////////////////// OPTIONAL FUNCTIONS /////////////////////
//////////////////////////////////////////////////////////////
// The implementation of below functions is optional. If you don't need them, don't include them in your
// plugin

int setUserToChannel(mumble_channelid_t channelID) {
	mumble_error_t result;

	// Set the user to the given channel
	result = mumAPI.requestUserMove(ownID, activeConnection, userID, channelID, NULL);
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to set user to channel");
		return -1;
	}

	return 0;
}

int linkChannels(std::vector< mumble_channelid_t > channels) {
	mumble_error_t result;
	mumble_channelid_t currentChannel;

	result = mumAPI.getChannelOfUser(ownID, activeConnection, userID, &currentChannel);
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to get channel of user");
		return -1;
	}

	channels.push_back(currentChannel);
	// Link the channel
	result = mumAPI.requestLinkChannels(ownID, activeConnection, channels.data(), channels.size());
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to link channel");
		return -1;
	}

	return 0;
}

int unlinkChannels(std::vector< mumble_channelid_t > channels) {
	mumble_error_t result;
	mumble_channelid_t currentChannel;

	result = mumAPI.getChannelOfUser(ownID, activeConnection, userID, &currentChannel);
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to get channel of user");
		return -1;
	}

	channels.push_back(currentChannel);
	// Unlink the channel
	result = mumAPI.requestUnlinkGivenChannels(ownID, activeConnection, channels.data(), channels.size());
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to unlink channel");
		return -1;
	}

	return 0;
}

int startListeningToChannel(std::vector< mumble_channelid_t > channels) {
	mumble_error_t result;

	// Listen to the channel
	result = mumAPI.requestStartListeningToChannels(ownID, activeConnection, channels.data(), channels.size());
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to listen to channel");
		return -1;
	}

	return 0;
}

int stopListeningToChannel(std::vector< mumble_channelid_t > channels) {
	mumble_error_t result;

	// Stop listening to the channel
	result = mumAPI.requestStopListeningToChannels(ownID, activeConnection, channels.data(), channels.size());
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to stop listening to channel");
		return -1;
	}

	return 0;
}

int whisperToUser(mumble_userid_t user) {
}

int unwhisperToUser(mumble_userid_t user) {
}

int getCurrentUserName(const char **userName) {
	mumble_error_t result;
	result = mumAPI.getUserName(ownID, activeConnection, userID, userName);
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to get user name");
		return -1;
	}

	return 0;
}

int requestLocalUserMutestate(bool mute) {
	mumble_error_t result;
	result = mumAPI.requestLocalUserMute(ownID, mute);
	if (result != MUMBLE_STATUS_OK) {
		logd(LOG_ERR, "Failed to set user mute state");
		return -1;
	}

	return 0;
}
//////////////////////////////////////////////////////////////