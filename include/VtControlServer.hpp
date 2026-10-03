//================================================================================================
/// @file VtControlServer.hpp
///
/// @brief The control interface of the terminal: a TCP port on 127.0.0.1 where test tools read
/// what the terminal shows and press soft keys and buttons and enter values as an operator does.
/// Started with --control-port=<port>; off without it.
///
/// Protocol: one JSON object a line each way (UTF-8).
///   request  {"id": <any>, "op": "<op>", <arguments>}
///   reply    {"id": <the same>, "ok": true, "result": <...>} or {"id": ..., "ok": false, "error": "<why>"}
/// Ops: hello, working_sets, select_working_set {index}, state, screen {all, hidden},
/// object {object_id}, softkey {object_id | position, hold_ms}, button {object_id, hold_ms},
/// set_input {object_id, value | raw | index | text}, screenshot {path}.
/// tools/vt_control.py is a command line and Python client for it.
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef VT_CONTROL_SERVER_HPP
#define VT_CONTROL_SERVER_HPP

#include "VtAutomation.hpp"

#include "JuceHeader.h"

#include <functional>
#include <memory>

class ServerMainComponent;

class VtControlServer : private juce::Thread
{
public:
	static constexpr int MESSAGE_THREAD_TIMEOUT_MS = 5000; ///< How long an op may wait for the GUI thread
	static constexpr int DEFAULT_HOLD_MS = 150; ///< How long a soft key or button is held down
	static constexpr int MAX_HOLD_MS = 10000; ///< The longest hold an op accepts
	static constexpr std::size_t MAX_LINE_BYTES = 1 << 20; ///< The longest request line

	VtControlServer(ServerMainComponent &server, int port);
	~VtControlServer() override;

	/// @brief Listens on 127.0.0.1 at the port; false when it cannot (the port is taken).
	bool start();

	/// @brief Stops listening and closes every connection.
	void stop();

	int get_port() const;

	/// @brief The reply to one request line; called on a connection's thread.
	juce::String handle_line(const juce::String &line);

private:
	class Connection : public juce::Thread
	{
	public:
		Connection(VtControlServer &owner, std::unique_ptr<juce::StreamingSocket> socket);
		~Connection() override;
		void close();

	private:
		void run() override;
		VtControlServer &owner;
		std::unique_ptr<juce::StreamingSocket> socket;
	};

	void run() override;
	juce::var handle(const juce::var &request);
	juce::var on_message_thread(std::function<juce::var(VtAutomation &)> function);

	juce::Component::SafePointer<juce::Component> serverComponent;
	ServerMainComponent &server;
	juce::StreamingSocket listener;
	juce::OwnedArray<Connection> connections;
	juce::CriticalSection connectionsLock;
	int port = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VtControlServer)
};

#endif // VT_CONTROL_SERVER_HPP
