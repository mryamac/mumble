#include "USBSerialCom.h"
#include <QFile>
#include <fstream>
#include <iostream>
#include <qstandardpaths.h>
#include <sstream>
#include <thread>
#include <unordered_map>

std::atomic< bool > USBSerialCom::shouldStop;
std::thread USBSerialCom::myThread;
std::string USBSerialCom::devPath          = "";
unsigned int USBSerialCom::baudRate        = 0;
mumble_channelid_t USBSerialCom::channelID = 0;
const char *USBSerialCom::userName;
mumble_userid_t USBSerialCom::callUserID       = 0;
mumble_channelid_t USBSerialCom::callChannelID = 0;
LibSerial::SerialStream USBSerialCom::serialStream;

void USBSerialCom::load() {
	std::string settingsPath = findSettingsLocation(false);

	std::ifstream stream(settingsPath);

	// Check if the file is open
	if (!stream.is_open()) {
		throw std::string("Could not open settings file" + settingsPath);
	}

	// Container to store key-value pairs
	std::unordered_map< std::string, std::string > keyValuePairs;

	// Read and parse each line from the file
	std::string line;
	while (std::getline(stream, line)) {
		std::istringstream iss(line);
		std::string key, value;
		if (std::getline(iss, key, '=') && std::getline(iss, value)) {
			// Trim leading and trailing whitespaces from key and value
			key.erase(0, key.find_first_not_of(" \t\n\r\f\v"));
			key.erase(key.find_last_not_of(" \t\n\r\f\v") + 1);
			value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
			value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);

			// Insert key-value pair into the map
			keyValuePairs[key] = value;
		}
	}

	devPath   = keyValuePairs["devPath"];
	baudRate  = std::stoul(keyValuePairs["baudRate"].c_str());
	channelID = std::stoul(keyValuePairs["channelID"].c_str());

	stream.close();
}

std::string USBSerialCom::findSettingsLocation(bool legacy) {
	QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	QFile settingsFile(QString::fromLatin1("%1/%2").arg(path).arg(QLatin1String("Mumble/settings.ini")));

	return settingsFile.fileName().toStdString();
}

void USBSerialCom::startPooling() {
	// Start pooling the serial port
	shouldStop.store(false);
	serialStream.Open(devPath.c_str());
	LibSerial::BaudRate baudRateEnum = LibSerial::BaudRate::BAUD_9600;
	switch (baudRate) {
		case 9600:
			baudRateEnum = LibSerial::BaudRate::BAUD_9600;
			break;
		case 19200:
			baudRateEnum = LibSerial::BaudRate::BAUD_19200;
			break;
		case 38400:
			baudRateEnum = LibSerial::BaudRate::BAUD_38400;
			break;
		case 57600:
			baudRateEnum = LibSerial::BaudRate::BAUD_57600;
			break;
		case 115200:
			baudRateEnum = LibSerial::BaudRate::BAUD_115200;
			break;
		default:
			// Do something else
			break;
	}
	serialStream.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
	myThread = std::thread(loop);
}

void USBSerialCom::stopPooling() {
	// Stop pooling the serial port
	shouldStop.store(true);
	if (myThread.joinable()) {
		myThread.join();
	}
	serialStream.Close();
}

int USBSerialCom::sendMessageToUser(const char *userName, const char *message) {
	int result;
	// Send a message to the user
	result = sendDataToUser(userName, message);
	if (result == -1)
		return -1;
	return 0;
}

int USBSerialCom::callToUser(const char *user) {
	int result;
	int nb_tries = 5;
	mumble_channelid_t currentChannel;
	std::string channelName;
	mumble_channelid_t callChannelID;
	std::string message;

	// Check if the user is already in a call
	if (callUserID != 0) {
		response("ALREADY IN A CALL");
		return -1;
	}

	// Set the user to the call channel
	if (userName == nullptr) {
		result = getCurrentUserName(&userName);
		if (result == -1)
			return -1;
	}
	currentChannel = getCurrentChannelID();
	if (currentChannel == -1)
		return -1;
	channelName = "Call-" + std::string(userName) + "-" + std::string(user);
	// result      = createTempChannel(channelName.c_str());
	if (result == -1)
		return -1;
	while (getCurrentChannelID() == currentChannel && nb_tries > 0) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		nb_tries--;
	}
	if (currentChannel == getCurrentChannelID())
		return -1;

	// Get the call channel ID
	callChannelID = getCurrentChannelID();
	if (callChannelID == -1)
		return -1;

	// Send a message to the user
	message = "CALL " + std::to_string(callChannelID);
	result  = sendDataToUser(user, message.c_str());
	if (result == -1)
		return -1;

	callUserID = result;
	startListeningToChannel({ channelID });

	return 0;
}

void USBSerialCom::endCall() {
	// Remove the call channel
	if (callChannelID == 0)
		return;

	std::string message = "ENDCALL";
	sendDataToUser(callUserID, message.c_str());
	clearCallState();
}

int USBSerialCom::acceptCall() {
	// Accept the call
	if (callUserID == 0)
		return -1;

	setUserToChannel(callChannelID);
	startListeningToChannel({ channelID });
	sendDataToUser(callUserID, "ACCEPT");
	return 0;
}

int USBSerialCom::rejectCall() {
	sendDataToUser(callUserID, "REJECT");
	clearCallState();
	return 0;
}

void USBSerialCom::clearCallState() {
	callUserID    = 0;
	callChannelID = 0;
	setUserToChannel(channelID);
	stopListeningToChannel({ channelID });
}

void USBSerialCom::loop() {
	// This is the function that will be called in a separate thread
	// It will pool the serial port for new data

	while (!shouldStop.load()) {
		// Read data from the serial port
		std::string data;
		int result;
		std::getline(serialStream, data);
		// std::getline(std::cin, data);

		// Process the read data
		if (!data.empty()) {
			std::istringstream iss(data);
			std::vector< mumble_channelid_t > tokens; // Vector to store each token
			std::string token;
			std::string command;

			iss >> command; // Read the first token as the command

			if (command.empty())
				continue;

			if (strcmp(command.c_str(), "CHANNEL") == 0) {
				iss >> token;
				if (!token.empty())
					setUserToChannel(std::stoi(token));
			} else if (strcmp(command.c_str(), "LINK") == 0) {
				while (iss >> token)
					tokens.push_back(std::stoi(token));
				if (tokens.size() < 1)
					continue;
				linkChannels(tokens);
			} else if (strcmp(command.c_str(), "UNLINK") == 0) {
				while (iss >> token)
					tokens.push_back(std::stoi(token));
				if (tokens.size() < 1)
					continue;
				unlinkChannels(tokens);
			} else if (strcmp(command.c_str(), "LISTEN") == 0) {
				while (iss >> token)
					tokens.push_back(std::stoi(token));
				if (tokens.size() < 1)
					continue;
				startListeningToChannel(tokens);
			} else if (strcmp(command.c_str(), "NLISTEN") == 0) {
				while (iss >> token)
					tokens.push_back(std::stoi(token));
				if (tokens.size() < 1)
					continue;
				stopListeningToChannel(tokens);
			} else if (strcmp(command.c_str(), "CALL") == 0) {
				iss >> token;
				if (token.empty())
					continue;

				result = callToUser(token.c_str());
				if (result == -1) {
					response("CALL FAILED");
					clearCallState();
				}
			} else if (strcmp(command.c_str(), "SEND") == 0) {
				std::string user, message;
				iss >> user;
				iss >> message;
				if (user.empty() || message.empty())
					continue;
				result = sendMessageToUser(user.c_str(), message.c_str());
				if (result == -1) {
					response("SEND FAILED");
				}
			} else if (strcmp(command.c_str(), "ENDCALL") == 0) {
				endCall();
			} else if (strcmp(command.c_str(), "ACCEPT") == 0) {
				acceptCall();
			} else if (strcmp(command.c_str(), "REJECT") == 0) {
				rejectCall();
			} else {
				// Do something else
			}
		}

		// Add a delay to simulate continuous reading 100ms

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	return;
}

void USBSerialCom::response(std::string message) {
	// Send a response to the serial port
	serialStream << message;
	// std::cout << message << std::endl;
}

mumble_userid_t USBSerialCom::getCallUser() {
	return callUserID;
}

void USBSerialCom::setCall(mumble_userid_t user, mumble_channelid_t channelID) {
	// Set the last call
	callUserID    = user;
	callChannelID = channelID;
}