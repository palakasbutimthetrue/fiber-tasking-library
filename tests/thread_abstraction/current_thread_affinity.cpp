/**
 * FiberTaskingLib - A tasking library that uses fibers for efficient task switching
 *
 * This library was created as a proof of concept of the ideas presented by
 * Christian Gyrling in his 2015 GDC Talk 'Parallelizing the Naughty Dog Engine Using Fibers'
 *
 * http://gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine
 *
 * FiberTaskingLib is the legal property of Adrian Astley
 * Copyright Adrian Astley 2015 - 2018
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ftl/typedefs.h"
#include "ftl/thread_abstraction.h"
#include "ftl/config.h"

#include "gtest/gtest.h"


#if defined(FTL_OS_WINDOWS)


static void AffinityTest() {
	HANDLE currentThread = GetCurrentThread();

	for (std::size_t i = 0; i < ftl::GetNumHardwareThreads(); ++i) {
		DWORD_PTR mask = 1ull << i;
		// Initial set
		ftl::SetCurrentThreadAffinity(i);

		// Do an initial check
		DWORD_PTR comparison = SetThreadAffinityMask(currentThread, mask);
		ASSERT_EQ(mask, comparison);

		// Sleep
		ftl::ThreadSleep(250);

		// Check again
		comparison = SetThreadAffinityMask(currentThread, mask);
		ASSERT_EQ(mask, comparison);
	}
}

#elif defined(FTL_OS_LINUX)

static void AffinityTest() {
	pthread_t currentThread = pthread_self();

	for (std::size_t i = 0; i < ftl::GetNumHardwareThreads(); ++i) {
		// Initial set
		ftl::SetCurrentThreadAffinity(i);
		
		cpu_set_t reference;
		CPU_ZERO(&reference);
		CPU_SET(i, &reference);

		// Do an initial check
		cpu_set_t comparison;
		CPU_ZERO(&comparison);
		ASSERT_EQ(pthread_getaffinity_np(currentThread, sizeof(cpu_set_t), &comparison), 0);
		ASSERT_TRUE(CPU_EQUAL(&reference, &comparison));

		// Sleep
		ftl::ThreadSleep(250);

		// Check again
		CPU_ZERO(&comparison);
		ASSERT_EQ(pthread_getaffinity_np(currentThread, sizeof(cpu_set_t), &comparison), 0);
		ASSERT_TRUE(CPU_EQUAL(&reference, &comparison));
	}
}

#elif defined(FTL_OS_MAC) || defined(FTL_OS_iOS)

static void AffinityTest() {

}

#else
#error Platform not implemented
#endif

TEST(ThreadAbstraction, CurrentThreadAffinityCheck) {
	AffinityTest();
}
