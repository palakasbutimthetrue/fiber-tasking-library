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

#include <atomic>


struct ThreadArgs {
	std::size_t Affinity;
	std::atomic<uint> *ErrorCounter;
};


#if defined(FTL_OS_WINDOWS)

FTL_THREAD_FUNC_DECL ThreadStart(void *arg) {
	ThreadArgs *threadArgs = (ThreadArgs *)arg;

	DWORD_PTR mask = 1ull << threadArgs->Affinity;
	HANDLE currentThread = GetCurrentThread();

	// Do an initial check
	DWORD_PTR comparison = SetThreadAffinityMask(currentThread, mask);
	if (mask != comparison) {
		threadArgs->ErrorCounter->fetch_add(1);
		FTL_THREAD_FUNC_END;
	}

	// Sleep
	ftl::ThreadSleep(250);

	// Check again
	comparison = SetThreadAffinityMask(currentThread, mask);
	if (mask != comparison) {
		threadArgs->ErrorCounter->fetch_add(1);
	}

	FTL_THREAD_FUNC_END;
}

#elif defined(FTL_OS_LINUX)

FTL_THREAD_FUNC_DECL ThreadStart(void *arg) {
	ThreadArgs *threadArgs = (ThreadArgs *)arg;

	pthread_t currentThread = pthread_self();
	cpu_set_t reference;
	CPU_ZERO(&reference);
	CPU_SET(threadArgs->Affinity, &reference);

	// Do an initial check
	cpu_set_t comparison;
	CPU_ZERO(&comparison);
	if (pthread_getaffinity_np(currentThread, sizeof(cpu_set_t), &comparison) != 0) {
		threadArgs->ErrorCounter->fetch_add(1);
		FTL_THREAD_FUNC_END;
	}
	if (!CPU_EQUAL(&reference, &comparison)) {
		threadArgs->ErrorCounter->fetch_add(1);
		FTL_THREAD_FUNC_END;
	}

	// Sleep
	ftl::ThreadSleep(250);

	// Check again
	CPU_ZERO(&comparison);
	if (pthread_getaffinity_np(currentThread, sizeof(cpu_set_t), &comparison) != 0) {
		threadArgs->ErrorCounter->fetch_add(1);
		FTL_THREAD_FUNC_END;
	}
	if (!CPU_EQUAL(&reference, &comparison)) {
		threadArgs->ErrorCounter->fetch_add(1);
	}

	FTL_THREAD_FUNC_END;
}

#elif defined(FTL_OS_MAC) || defined(FTL_OS_iOS)

FTL_THREAD_FUNC_DECL ThreadStart(void *arg) {
	FTL_THREAD_FUNC_END;
}

#else
#error Platform not implemented
#endif


TEST(ThreadAbstraction, CreatedThreadAffinityCheck) {
	std::atomic<uint> errorCounter(0);
	uint coreCount = ftl::GetNumHardwareThreads();

	ThreadArgs *threadArgs = new ThreadArgs[coreCount];
	ftl::ThreadType *threads = new ftl::ThreadType[coreCount];
	for (uint i = 0; i < coreCount; ++i) {
		threadArgs[i].ErrorCounter = &errorCounter;
		threadArgs[i].Affinity = i;

		ftl::CreateThread(524288, ThreadStart, &threadArgs[i], i, &threads[i]);
	}

	// Wait for the threads to finish
	for (uint i = 0; i < coreCount; ++i) {
		ftl::JoinThread(threads[i]);
	}

	// Check for errors
	ASSERT_EQ(errorCounter.load(), 0);
}
