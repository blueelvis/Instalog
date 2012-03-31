// Copyright � 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/EventLog.hpp"

using Instalog::SystemFacades::EventLog;

TEST(EventLog, Test)
{
	EventLog eventLog;

	eventLog.ReadEvents();
}