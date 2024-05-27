#include "Agent.h"
#include "Logger.h"

Agent *Agent::instance = nullptr;

Agent::Agent(/* args */) {
	this->context = std::make_unique< zmq::context_t >(1);
	this->socket  = std::make_unique< zmq::socket_t >(*(this->context), zmq::socket_type::rep);
}

Agent::~Agent() {
}

void Agent::initialize() {
	// Set Ready state
	this->socket->bind(this->defaultConfig.getMumbleConnectionString());
}

void Agent::startLoop() {
	this->pluginThread = std::thread(&Agent::loop, this);
}

void Agent::stopLoop() {
	this->shutdown = true;
	this->pluginThread.join();

	logd(LOG_INFO, "Agent stopped");
}

void Agent::loop() {
	while (!this->shutdown) {
		zmq::message_t response;
		zmq::recv_result_t recvResult = this->socket->recv(response, zmq::recv_flags::none);

		mumble_message responseMessage;
		responseMessage.ParseFromArray(response.data(), response.size());

		if (responseMessage.command() == COMMAND_NAME(HandlerCommands::CHANNEL)) {
			if (responseMessage.args_size() > 0)
				if (setUserToChannel(std::stoi(responseMessage.args(0))) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::LINK)) {
			if (responseMessage.args_size() > 0) {
				std::vector< mumble_channelid_t > channels;
				for (int i = 0; i < responseMessage.args_size(); i++)
					channels.push_back(std::stoi(responseMessage.args(i)));

				if (linkChannels(channels) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::UNLINK)) {
			if (responseMessage.args_size() > 0) {
				std::vector< mumble_channelid_t > channels;
				for (int i = 0; i < responseMessage.args_size(); i++)
					channels.push_back(std::stoi(responseMessage.args(i)));

				if (unlinkChannels(channels) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::LISTEN)) {
			if (responseMessage.args_size() > 0) {
				std::vector< mumble_channelid_t > channels;
				for (int i = 0; i < responseMessage.args_size(); i++)
					channels.push_back(std::stoi(responseMessage.args(i)));

				if (startListeningToChannel(channels) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::UNLISTEN)) {
			if (responseMessage.args_size() > 0) {
				std::vector< mumble_channelid_t > channels;
				for (int i = 0; i < responseMessage.args_size(); i++)
					channels.push_back(std::stoi(responseMessage.args(i)));

				if (stopListeningToChannel(channels) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::WHISPER)) {
			if (responseMessage.args_size() > 0)
				if (whisperToUser(std::stoi(responseMessage.args(0))) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::UNWHISPER)) {
			if (responseMessage.args_size() > 0)
				if (unwhisperToUser(std::stoi(responseMessage.args(0))) == 0) {
					sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
					continue;
				}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(SystemCommands::PING)) {
			sendMessage(COMMAND_NAME(SystemCommands::PONG));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::USERNAME)) {
			const char *userName;
			if (getCurrentUserName(&userName) == 0) {
				sendMessage(COMMAND_NAME(SystemCommands::SUCCESS), userName);
				continue;
			}
			sendMessage(COMMAND_NAME(SystemCommands::FAIL));
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::MUTE)) {
			if (requestLocalUserMutestate(true) == 0) {
				sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
				continue;
			}
		} else if (responseMessage.command() == COMMAND_NAME(HandlerCommands::UNMUTE)) {
			if (requestLocalUserMutestate(false) == 0) {
				sendMessage(COMMAND_NAME(SystemCommands::SUCCESS));
				continue;
			}
		} else
			sendMessage(COMMAND_NAME(HandlerCommands::NONE));
	}
}

bool Agent::sendMessage(const char *command) {
	mumble_message message;
	message.set_command(command);

	// Send the message
	zmq::message_t request(message.ByteSizeLong());
	message.SerializeToArray(request.data(), request.size());
	zmq::send_result_t result = this->socket->send(request, zmq::send_flags::dontwait);

	if (result.has_value())
		return true;
	else
		return false;
}

bool Agent::sendMessage(const char *command, const char *arg) {
	mumble_message message;
	message.set_command(command);

	if (arg != nullptr)
		message.add_args(arg);

	// Send the message
	zmq::message_t request(message.ByteSizeLong());
	message.SerializeToArray(request.data(), request.size());
	zmq::send_result_t result = this->socket->send(request, zmq::send_flags::dontwait);

	if (result.has_value())
		return true;
	else
		return false;
}

void Agent::terminate() {
	if (this->socket != nullptr)
		this->socket->close();

	if (this->context != nullptr)
		this->context->close();
}