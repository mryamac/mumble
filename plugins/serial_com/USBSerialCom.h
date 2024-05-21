#ifndef MUMBLE_MUMBLE_PLUGIN_SERIAL_COM_SETTINGS_H_
#define MUMBLE_MUMBLE_PLUGIN_SERIAL_COM_SETTINGS_H_

#include <SerialStream.h>
#include <atomic> // For std::atomic<bool>
#include <string>
#include <thread>
#include <vector>

typedef int32_t mumble_channelid_t;
typedef uint32_t mumble_userid_t;

mumble_channelid_t getChannelID(const char *channelID);
int setUserToChannel(mumble_channelid_t channelID);
int linkChannels(std::vector< mumble_channelid_t > channels);
int unlinkChannels(std::vector< mumble_channelid_t > channels);
int startListeningToChannel(std::vector< mumble_channelid_t > channels);
int stopListeningToChannel(std::vector< mumble_channelid_t > channels);
// int createTempChannel(const char *channelName);
int sendDataToUser(const char *userName, const char *message);
int sendDataToUser(mumble_userid_t user, const char *message);
int getCurrentUserName(const char **userName);
mumble_channelid_t getCurrentChannelID();

class USBSerialCom {
private:
	static LibSerial::SerialStream serialStream;
	static std::atomic< bool > shouldStop;
	static std::thread myThread;
	static std::string devPath;
	static unsigned int baudRate;
	static mumble_channelid_t channelID;
	const static char *userName;
	static mumble_userid_t callUserID;
	static mumble_channelid_t callChannelID;
	static std::string findSettingsLocation(bool legacy);
	static void loop();

public:
	USBSerialCom(/* args */){};
	~USBSerialCom(){};

	static void load();
	static void startPooling();
	static void stopPooling();
	static void response(std::string message);

	static mumble_userid_t getCallUser();
	static void setCall(mumble_userid_t user, mumble_channelid_t channelID);

	static void clearCallState();
	static int callToUser(const char *userName);
	static int sendMessageToUser(const char *userName, const char *message);
	static void endCall();
	static int acceptCall();
	static int rejectCall();
};


#endif // MUMBLE_MUMBLE_PLUGIN_SERIAL_COM_SETTINGS_H_