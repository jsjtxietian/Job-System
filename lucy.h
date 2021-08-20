#pragma once

#ifndef ENGINE_API
#define ENGINE_API
#endif

namespace lucy
{
	using u8 = unsigned char;
	using u32 = unsigned;
	using u64 = unsigned long long;
	using SignalHandle = u32;
	constexpr u8 ANY_WORKER = 0xff;
	constexpr SignalHandle INVALID_HANDLE = 0xffFFffFF;

	ENGINE_API bool init(u8 workers_count);
	ENGINE_API void shutdown();
	ENGINE_API int getWorkersCount();
	ENGINE_API void incSignal(SignalHandle *signal);
	ENGINE_API void decSignal(SignalHandle signal);
	ENGINE_API void run(void *data, void (*task)(void *), SignalHandle *on_finish);
	ENGINE_API void runEx(void *data, void (*task)(void *), SignalHandle *on_finish, SignalHandle precondition, u8 worker_index);
	ENGINE_API void wait(SignalHandle waitable);
	ENGINE_API inline bool isValid(SignalHandle waitable) { return waitable != INVALID_HANDLE; }

}; // namespace lucy
