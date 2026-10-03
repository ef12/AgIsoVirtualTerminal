/*******************************************************************************
** @file       VtControlServer.cpp
** @author     The Open-Agriculture Developers
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "VtControlServer.hpp"

#include "ServerMainComponent.hpp"

#include <string>

namespace
{
	constexpr int POLL_MS = 200; ///< How often the threads look whether they should stop

	juce::var reply(const juce::var &id, bool ok, const juce::var &payload)
	{
		auto object = new juce::DynamicObject();
		object->setProperty("id", id);
		object->setProperty("ok", ok);
		object->setProperty(ok ? "result" : "error", payload);
		return juce::var(object);
	}

	int number_argument(const juce::var &request, const juce::Identifier &name, int fallback)
	{
		auto value = request.getProperty(name, juce::var());
		if (value.isVoid())
		{
			return fallback;
		}
		if (!(value.isInt() || value.isInt64() || value.isDouble()))
		{
			throw VtAutomationError(name.toString().toStdString() + " must be a number");
		}
		return static_cast<int>(value);
	}
} // namespace

VtControlServer::VtControlServer(ServerMainComponent &server, int port) :
  juce::Thread("vt-control"),
  serverComponent(&server),
  server(server),
  port(port)
{
}

VtControlServer::~VtControlServer()
{
	stop();
}

bool VtControlServer::start()
{
	if (!listener.createListener(port, "127.0.0.1"))
	{
		return false;
	}
	port = listener.getBoundPort();
	startThread();
	return true;
}

void VtControlServer::stop()
{
	signalThreadShouldExit();
	listener.close();
	stopThread(2 * POLL_MS + 1000);
	const juce::ScopedLock lock(connectionsLock);
	for (auto *connection : connections)
	{
		connection->close();
	}
	connections.clear();
}

int VtControlServer::get_port() const
{
	return port;
}

void VtControlServer::run()
{
	while (!threadShouldExit())
	{
		if (1 != listener.waitUntilReady(true, POLL_MS))
		{
			continue;
		}
		std::unique_ptr<juce::StreamingSocket> socket(listener.waitForNextConnection());
		if (nullptr == socket)
		{
			continue;
		}
		const juce::ScopedLock lock(connectionsLock);
		for (int i = connections.size() - 1; i >= 0; i--)
		{
			if (!connections[i]->isThreadRunning())
			{
				connections.remove(i);
			}
		}
		connections.add(new Connection(*this, std::move(socket)))->startThread();
	}
}

juce::String VtControlServer::handle_line(const juce::String &line)
{
	juce::var request;
	auto parsed = juce::JSON::parse(line, request);
	if (parsed.failed() || !request.isObject())
	{
		return juce::JSON::toString(reply(juce::var(), false, "a request is a JSON object: " + parsed.getErrorMessage()), true);
	}
	auto id = request.getProperty("id", juce::var());
	try
	{
		return juce::JSON::toString(reply(id, true, handle(request)), true);
	}
	catch (const std::exception &error)
	{
		return juce::JSON::toString(reply(id, false, juce::String(error.what())), true);
	}
}

juce::var VtControlServer::handle(const juce::var &request)
{
	const auto op = request.getProperty("op", "").toString();

	if ((op == "softkey") || (op == "button"))
	{
		const int hold = number_argument(request, "hold_ms", DEFAULT_HOLD_MS);
		if ((hold < 0) || (hold > MAX_HOLD_MS))
		{
			throw VtAutomationError("hold_ms must be 0.." + std::to_string(MAX_HOLD_MS));
		}
		auto press = std::make_shared<VtAutomation::Press>();
		const bool softKey = (op == "softkey");
		auto result = on_message_thread([request, press, softKey](VtAutomation &automation) {
			return automation.press_down(request, softKey, *press);
		});
		juce::Thread::sleep(hold);
		on_message_thread([press](VtAutomation &automation) {
			automation.press_up(*press);
			return juce::var();
		});
		return result;
	}
	if (op == "hello")
	{
		return on_message_thread([](VtAutomation &automation) { return automation.hello(); });
	}
	if (op == "working_sets")
	{
		return on_message_thread([](VtAutomation &automation) { return automation.working_sets(); });
	}
	if (op == "select_working_set")
	{
		const int index = number_argument(request, "index", -1);
		return on_message_thread([index](VtAutomation &automation) { return automation.select_working_set(index); });
	}
	if (op == "state")
	{
		return on_message_thread([](VtAutomation &automation) { return automation.state(); });
	}
	if (op == "screen")
	{
		const bool all = static_cast<bool>(request.getProperty("all", false));
		const bool hidden = static_cast<bool>(request.getProperty("hidden", false));
		return on_message_thread([all, hidden](VtAutomation &automation) { return automation.screen(all, hidden); });
	}
	if (op == "object")
	{
		const int objectID = number_argument(request, "object_id", -1);
		return on_message_thread([objectID](VtAutomation &automation) { return automation.object(objectID); });
	}
	if (op == "set_input")
	{
		const int objectID = number_argument(request, "object_id", -1);
		return on_message_thread([objectID, request](VtAutomation &automation) { return automation.set_input(objectID, request); });
	}
	if (op == "screenshot")
	{
		const auto path = request.getProperty("path", "").toString();
		return on_message_thread([path](VtAutomation &automation) { return automation.screenshot(path); });
	}
	throw VtAutomationError("unknown op '" + op.toStdString() +
	                        "'; the ops are hello, working_sets, select_working_set, state, screen, object, softkey, button, set_input, screenshot");
}

juce::var VtControlServer::on_message_thread(std::function<juce::var(VtAutomation &)> function)
{
	struct Call
	{
		juce::WaitableEvent done;
		juce::var result;
		std::string error;
	};
	auto call = std::make_shared<Call>();
	auto component = serverComponent;
	juce::MessageManager::callAsync([call, component, function]() {
		auto *owner = dynamic_cast<ServerMainComponent *>(component.getComponent());
		if (nullptr == owner)
		{
			call->error = "the terminal is shutting down";
		}
		else
		{
			try
			{
				VtAutomation automation(*owner);
				call->result = function(automation);
			}
			catch (const std::exception &error)
			{
				call->error = error.what();
			}
		}
		call->done.signal();
	});
	if (!call->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
	{
		throw VtAutomationError("the terminal's GUI thread did not answer within " + std::to_string(MESSAGE_THREAD_TIMEOUT_MS) + " ms");
	}
	if (!call->error.empty())
	{
		throw VtAutomationError(call->error);
	}
	return call->result;
}

// --- one connection ------------------------------------------------------------------------------

VtControlServer::Connection::Connection(VtControlServer &owner, std::unique_ptr<juce::StreamingSocket> socket) :
  juce::Thread("vt-control-connection"),
  owner(owner),
  socket(std::move(socket))
{
}

VtControlServer::Connection::~Connection()
{
	close();
}

void VtControlServer::Connection::close()
{
	signalThreadShouldExit();
	socket->close();
	stopThread(2 * POLL_MS + VtControlServer::MESSAGE_THREAD_TIMEOUT_MS + VtControlServer::MAX_HOLD_MS);
}

void VtControlServer::Connection::run()
{
	std::string pending;
	char buffer[4096];

	while (!threadShouldExit())
	{
		const int ready = socket->waitUntilReady(true, POLL_MS);
		if (ready < 0)
		{
			return;
		}
		if (0 == ready)
		{
			continue;
		}
		const int count = socket->read(buffer, static_cast<int>(sizeof(buffer)), false);
		if (count <= 0)
		{
			return;
		}
		pending.append(buffer, static_cast<std::size_t>(count));

		std::size_t end;
		while (std::string::npos != (end = pending.find('\n')))
		{
			auto line = juce::String::fromUTF8(pending.data(), static_cast<int>(end)).trim();
			pending.erase(0, end + 1);
			if (line.isEmpty())
			{
				continue;
			}
			auto answer = (owner.handle_line(line) + "\n").toStdString();
			if (socket->write(answer.data(), static_cast<int>(answer.size())) != static_cast<int>(answer.size()))
			{
				return;
			}
		}
		if (pending.size() > VtControlServer::MAX_LINE_BYTES)
		{
			const std::string answer = "{\"id\": null, \"ok\": false, \"error\": \"a request line is at most 1 MiB\"}\n";
			socket->write(answer.data(), static_cast<int>(answer.size()));
			return;
		}
	}
}
