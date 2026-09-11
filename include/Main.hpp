/*******************************************************************************
** @file       Main.hpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#pragma once

#include <JuceHeader.h>
#include "ASCIILogFile.hpp"
#include "AppImages.h"
#include "ModernLookAndFeel.hpp"
#include "ServerMainComponent.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"

#include <thread>

#ifdef JUCE_WINDOWS
#include "CANAPI2MachineDeviceConfiguration.hpp"
#endif

//==============================================================================
class AgISOVirtualTerminalApplication : public juce::JUCEApplication
{
public:
	//==============================================================================
	AgISOVirtualTerminalApplication() {}

	const juce::String getApplicationName() override
	{
		return ProjectInfo::projectName;
	}

	const juce::String getApplicationVersion() override
	{
		return ProjectInfo::versionString;
	}

	static std::string getApplicationBuildInfo();
	static std::string getApplicationNameWithBuildInfo();

	bool moreThanOneInstanceAllowed() override
	{
		return true;
	}

	//==============================================================================
	void initialise(const juce::String &commandLineParameters) override
	{
		SystemStats::setApplicationCrashHandler(onCrash);
		LookAndFeel::setDefaultLookAndFeel(&modernLookAndFeel);

		juce::StringArray args;
		args.addTokens(commandLineParameters, true);

#ifdef JUCE_WINDOWS
		for (const auto &arg : args)
		{
			if (arg.startsWith("--set-canapi2-machine-device="))
			{
				CANAPI2MachineDeviceConfiguration::Device requestedDevice;
				int result = 13;
				if (CANAPI2MachineDeviceConfiguration::try_parse_helper_argument(arg.toStdString(), requestedDevice))
				{
					result = CANAPI2MachineDeviceConfiguration::set_machine_default_device(requestedDevice);
				}
				setApplicationReturnValue(result);
				quit();
				return;
			}
		}
#endif

		std::uint8_t vtNumber = 0;
		std::string screenCaptureDir;
		for (const auto &arg : args)
		{
			if (arg.startsWith("--vt-number"))
			{
				vtNumber = arg.fromFirstOccurrenceOf("--vt-number=", false, false).getIntValue();
				if (0 == vtNumber || vtNumber > 32)
				{
					std::cout << "The VT number must be between 1 and 32";
					vtNumber = 0;
				}
			}

			if (arg.startsWith("--screen-capture-dir="))
			{
				screenCaptureDir = arg.fromFirstOccurrenceOf("--screen-capture-dir=", false, false).toStdString();
			}
		}

		mainWindow.reset(new MainWindow(getApplicationNameWithBuildInfo(), logFile, vtNumber, screenCaptureDir));
	}

	void shutdown() override
	{
		if (shutdownThread.joinable())
		{
			shutdownThread.join();
		}

		if (isobus::CANHardwareInterface::is_running())
		{
			isobus::CANHardwareInterface::stop();
		}
		mainWindow = nullptr; // (deletes our window)
		LookAndFeel::setDefaultLookAndFeel(nullptr);
	}

	//==============================================================================
	void systemRequestedQuit() override
	{
		if (shutdownRequested)
		{
			return;
		}

		shutdownRequested = true;
		if (mainWindow)
		{
			mainWindow->setEnabled(false);
			mainWindow->setName(getApplicationNameWithBuildInfo() + " - Shutting down...");
		}

		if (!isobus::CANHardwareInterface::is_running())
		{
			quit();
			return;
		}

		// CAN shutdown joins its worker thread. Run it away from JUCE's message
		// thread so a CAN callback that is finishing a GUI update cannot deadlock
		// against the window close operation.
		shutdownThread = std::thread([this]() {
			isobus::CANHardwareInterface::stop();
			MessageManager::callAsync([this]() {
				quit();
			});
		});
	}

	void anotherInstanceStarted(const juce::String &) override
	{
		// When another instance of the app is launched while this one is running,
		// this method is invoked, and the commandLine parameter tells you what
		// the other instance's command-line arguments were.
	}

	static void onCrash(void *)
	{
		auto stackTrace = SystemStats::getStackBacktrace();
		auto appDataDir = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() +
		                       File::getSeparatorString() +
		                       "Open-Agriculture" +
		                       File::getSeparatorString());
		auto currentTime = Time::getCurrentTime().toString(true, true, true, false);
		auto fileNameTime = currentTime;
		fileNameTime = currentTime.replaceCharacter(' ', '_');
		fileNameTime = currentTime.replaceCharacter(':', '_');

		if (!appDataDir.exists())
		{
			appDataDir.createDirectory();
		}

		auto outputFile = appDataDir.getChildFile(JUCEApplication::getInstance()->getApplicationName() + "_crash_" + currentTime + ".txt");
		outputFile.appendText(stackTrace);
	}

	//==============================================================================
	/*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
	class MainWindow : public juce::DocumentWindow
	{
	public:
		/**
     * @brief MainWindow
     * @param name - window name to be displayed in the window title
	 * @param trafficLogger - controllable Vector ASCII CAN traffic recorder
     * @param vtNumberCmdLineArg - in the range of 1 - 32
     * @param screenCaptureDir - path to the directory where the screen capture results will be saved
     */
		MainWindow(juce::String name, ASCIILogFile &trafficLogger, int vtNumberCmdLineArg = 0, std::string screenCaptureDir = "");

		/* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */
		void closeButtonPressed() override;

	private:
		std::shared_ptr<isobus::InternalControlFunction> serverInternalControlFunction;
		std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> canDrivers;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	ModernLookAndFeel modernLookAndFeel;
	ASCIILogFile logFile;
	std::unique_ptr<MainWindow> mainWindow;
	std::thread shutdownThread;
	bool shutdownRequested = false;
};
