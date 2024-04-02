// Copyright (c) 2024 Meik Jäckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <inverter/Inverter.hpp>

namespace inverter
{
namespace test
{

class InverterTest :
	public ::testing::Test
{
	protected:
	InverterTest() {}
	virtual ~InverterTest() {}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	//
	/**
	 * @brief Code here will be called immediately after the constructor (right before each test).
	 */
	virtual void SetUp() {}

	/**
	 * @brief Code here will be called immediately after each test (right before the destructor).
	 */
	virtual void TearDown() {}

	// Objects declared here can be used by all tests in the test case for Foo.
};

TEST_F(InverterTest, VerifySizeOf_JkBmsWarnMsg)
{
  ASSERT_EQ(sizeof(uint16_t), 2);
}

} //namespace test
} //namespace inverter

// Note: This is just a workaround, to prevent duplicate code for test application startup.
//       If I have found a way to use multiple sources for unit tests within platformio, this include can be removed.
#include <common/main-test.cpp>
