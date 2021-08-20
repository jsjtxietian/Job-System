# Test Job System





## Useful links

* [Implementing a fiber system (lightweight user space threads) in my C game engine : gamedev (reddit.com)](https://www.reddit.com/r/gamedev/comments/hszalt/implementing_a_fiber_system_lightweight_user/)
* [Simple job system using standard C++ – Wicked Engine Net](https://wickedengine.net/2018/11/24/simple-job-system-using-standard-c/)
* [delscorcho/basic-job-system: Job system (requires C++11) (github.com)](https://github.com/delscorcho/basic-job-system)
* [C++ 11 Job System - Ben Hoffman](https://benhoffman.tech/cpp/general/2018/11/13/cpp-job-system.html)
* [Building a Coroutine based Job System without Standard Library - Tanki Zhang - CppCon 2020 - YouTube](https://www.youtube.com/watch?v=KWi793v5uA8)
* [hlavacs/ViennaGameJobSystem: A job system for game engines (github.com)](https://github.com/hlavacs/ViennaGameJobSystem)
* [Job System 2.0: Lock-Free Work Stealing – Part 3: Going lock-free | Molecular Musings (molecular-matters.com)](https://blog.molecular-matters.com/2015/09/25/job-system-2-0-lock-free-work-stealing-part-3-going-lock-free/)
* [Lock-free job stealing with modern c++ (manu343726.github.io)](https://manu343726.github.io/2017-03-13-lock-free-job-stealing-task-system-with-modern-c/)
* [LumixEngine/job_system.h at master · nem0/LumixEngine (github.com)](https://github.com/nem0/LumixEngine/blob/master/src/engine/job_system.h)
* [nem0/lucy_job_system: Fiber-based job system with extremely simple API (github.com)](https://github.com/nem0/lucy_job_system)

### Jobs

Job is a function pointer with associated void data pointer. When job is executed, the function is called and the data pointer is passed as a parameter to this function.

```lucy::run``` push a job to global queue

```cpp
int value = 42;
lucy::run(&value, [](void* data){
	printf("%d", *(int*)data);
}, nullptr);
```

This prints ```42```. Eventually, after the job is finished, a signal can be triggered, see signals for more information.

### Signals

```cpp
lucy::SignalHandle signal = lucy::INVALID_HANDLE;
lucy::wait(signal); // does not wait, since signal is invalid
lucy::incSignal(&signal);
lucy::wait(signal); // wait until someone calls lucy::decSignal, execute other jobs in the meantime
```

```cpp
lucy::SignalHandle signal = lucy::INVALID_HANDLE;
for (int i = 0; i < N; ++i) {
	lucy::incSignal(&signal);
}
lucy::wait(signal); // wait until lucy::decSignal is called N-times
```

If a signal is passed to ```lucy::run``` (3rd parameter), then the signal is automatically incremented. It is decremented once all the job is finished.

```cpp
lucy::SignalHandle finished = lucy::INVALID_HANDLE;
for(int i = 0; i < 10; ++i) {
	lucy::run(nullptr, job_fn, &finished);
}
lucy::wait(finished); // wait for all 10 jobs to finish
```

There's no need to destroy signals, it's "garbage collected".

Signals can be used as preconditions to run a job. It means a job starts only after the signal is signalled:

```cpp
lucy::SignalHandle precondition = getPrecondition();
lucy::runEx(nullptr, job_fn, nullptr, precondition, lucy::ANY_WORKER);
```

is functionally equivalent to:

```cpp
void job_fn(void*) { 
	lucy::wait(precondition);
	dowork();
}
lucy::run(nullptr, job_fn, nullptr);
```

However, the ```lucy::runEx``` version has better performance.

Finally, a job can be pinned to specific worker thread. This is useful for calling APIs which must be called from the same thread, e.g. OpenGL functions or WinAPI UI functions.

```cpp
lucy::runEx(nullptr, job_fn, nullptr, lucy::INVALID_HANDLE, 3); // run on worker thread 3
```
