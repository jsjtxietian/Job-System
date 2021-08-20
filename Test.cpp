// #include <atomic>
// #include <array>
// #include <vector>
// #include <iostream>
// #include <thread>
// #include <random>

// class Job
// {
// public:
//     Job() = default;
//     Job::Job(void (*jobFunction)(Job &), Job *parent = nullptr)
//         : _jobFunction{jobFunction},
//           _parent{parent},
//           _unfinishedJobs{1} // 1 means **this** job has not been run
//     {
//         if (_parent != nullptr)
//         {
//             _parent->_unfinishedJobs++;
//         }
//     }

//     void run()
//     {
//         auto jobFunction = _jobFunction;

//         _jobFunction(*this);

//         if (_jobFunction == jobFunction)
//         {
//             // The function has not changed,
//             // mark the job as "no callback"
//             _jobFunction = nullptr;
//         }
//         finish();
//     };

//     bool finished() const
//     {
//         return _unfinishedJobs == 0;
//     }

//     void Job::onFinished(void (*callback)(Job &))
//     {
//         _jobFunction = callback;
//     }

// private:
//     void finish()
//     {
//         _unfinishedJobs--;

//         if (finished())
//         {
//             if (_parent != nullptr)
//             {
//                 _parent->finish();
//             }

//             if (_jobFunction != nullptr)
//             {
//                 // Run the onFinished callback
//                 _jobFunction(*this);
//             }
//         }
//     }

//     void (*_jobFunction)(Job &);
//     Job *_parent;
//     std::atomic_size_t _unfinishedJobs;

//     //One optimization you could apply to the Job type is to align it to the cache line boundary to
//     //prevent false sharing. You can do so by using C++11 alignas(), but we can explicitly declare
//     //the padding bytes and use them for our convenience:
//     static constexpr std::size_t JOB_PAYLOAD_SIZE = sizeof(_jobFunction) + sizeof(_parent) + sizeof(_unfinishedJobs);

//     static constexpr std::size_t JOB_MAX_PADDING_SIZE = std::hardware_destructive_interference_size;
//     static constexpr std::size_t JOB_PADDING_SIZE = JOB_MAX_PADDING_SIZE - JOB_PAYLOAD_SIZE;

//     std::array<unsigned char, JOB_PADDING_SIZE> _padding;

// public:
//     template <typename T, typename... Args>
//     void constructData(Args &&...args)
//     {
//         new (_padding.data()) T(std::forward<Args>(args)...);
//     }

//     template <typename Data>
//     std::enable_if_t<
//         std::is_pod<Data>::value &&
//         (sizeof(Data) <= JOB_PADDING_SIZE)>
//     setData(const Data &data)
//     {
//         std::memcpy(_padding.data(), &data, sizeof(Data));
//     }

//     template <typename Data>
//     const Data &getData() const
//     {
//         return *reinterpret_cast<const Data *>(_padding.data());
//     }

//     template <typename Data>
//     Job(void (*jobFunction)(Job &), const Data &data, Job *parent = nullptr)
//         : Job{jobFunction, parent}
//     {
//         setData(data);
//     }
// };

// typedef void (*JobFunction)(Job &);

// class Pool
// {
// public:
//     Pool(std::size_t maxJobs) : _allocatedJobs{0},
//                                 _storage{maxJobs}
//     {
//     }

//     Job *allocate()
//     {
//         if (!full())
//         {
//             return &_storage[_allocatedJobs++];
//         }
//         else
//         {
//             return nullptr;
//         }
//     }
//     bool full() const { return _allocatedJobs == _storage.size(); };
//     void clear() { _allocatedJobs = 0; };

//     Job *createJob(JobFunction jobFunction, Job *parent = nullptr)
//     {
//         Job *job = allocate();

//         if (job != nullptr)
//         {
//             new (job) Job{jobFunction, parent};
//             return job;
//         }
//         else
//         {
//             return nullptr;
//         }
//     };
//     Job *createJobAsChild(JobFunction jobFunction, Job *parent);

//     template <typename Data>
//     Job *createJob(JobFunction jobFunction, const Data &data);
//     template <typename Data>
//     Job *createJobAsChild(JobFunction jobFunction, const Data &data, Job *parent);
//     template <typename Function>
//     Job *createClosureJob(Function function);
//     template <typename Function>
//     Job *createClosureJobAsChild(Function function, Job *parent);

// private:
//     std::size_t _allocatedJobs;
//     std::vector<Job> _storage;
// };

// class JobQueue
// {
// public:
//     JobQueue(std::size_t maxJobs);

//     bool push(Job *job)
//     {
//         int bottom = _bottom.load(std::memory_order_acquire);

//         if (bottom < static_cast<int>(_jobs.size()))
//         {
//             _jobs[bottom] = job;
//             _bottom.store(bottom + 1, std::memory_order_release);

//             return true;
//         }
//         else
//         {
//             return false;
//         }
//     }
//     Job *pop()
//     {
//         int bottom = _bottom.load(std::memory_order_acquire);
//         bottom = std::max(0, bottom - 1);
//         _bottom.store(bottom, std::memory_order_release);
//         int top = _top.load(std::memory_order_acquire);

//         if (top <= bottom)
//         {
//             Job *job = _jobs[bottom];

//             if (top != bottom)
//             {
//                 // More than one job left in the queue
//                 return job;
//             }
//             else
//             {
//                 int expectedTop = top;
//                 int desiredTop = top + 1;

//                 if (!_top.compare_exchange_weak(expectedTop, desiredTop,
//                                                 std::memory_order_acq_rel))
//                 {
//                     // Someone already took the last item, abort
//                     job = nullptr;
//                 }

//                 _bottom.store(top + 1, std::memory_order_release);
//                 return job;
//             }
//         }
//         else
//         {
//             // Queue already empty
//             _bottom.store(top, std::memory_order_release);
//             return nullptr;
//         }
//     }
//     Job *steal();
//     std::size_t size() const;
//     bool empty() const;

// private:
//     std::vector<Job *> _jobs;
//     std::atomic<int> _top, _bottom;
// };

// class Engine;

// class Worker
// {
// public:
//     enum class Mode
//     {
//         Background,
//         Foreground
//     };

//     enum class State
//     {
//         Idle,
//         Running,
//         Stopping
//     };

//     Worker(Engine *engine, std::size_t maxJobs, Mode mode = Mode::Background);
//     ~Worker();

//     void run()
//     {
//         //Background workers fetch work in an infinite loop run by the worker thread:
//         if (_mode == Mode::Background)
//         {
//             while (_running)
//             {
//                 Job *job = getJob();

//                 if (job != nullptr)
//                 {
//                     job->run();
//                 }
//             }
//         }
//     }
//     void stop();
//     bool running() const {return _running;}
//     Pool &pool();
//     void submit(Job *job) { _workQueue.push(job); }
//     //foreground workers use Worker::wait() to run jobs in the caller thread waiting until an specific job is finished
//     void wait(Job *waitJob)
//     {
//         while (!waitJob->finished())
//         {
//             Job *job = getJob();

//             if (job != nullptr)
//             {
//                 job->run();
//             }
//         }
//     }

// private:
//     Pool _pool;
//     Engine *_engine;
//     JobQueue _workQueue;
//     bool _running;
//     std::thread::id _threadId;
//     std::thread _thread;
//     std::atomic<State> _state;
//     std::atomic<Mode> _mode;

//     Job *getJob()
//     {
//         Job *job = _workQueue.pop();

//         if (job != nullptr)
//         {
//             job->run();
//         }
//         else
//         {
//             Worker *worker = _engine->randomWorker();

//             if (worker != this)
//             {
//                 Job *job = worker->_workQueue.steal();

//                 if (job != nullptr)
//                 {
//                     return job;
//                 }
//                 else
//                 {
//                     std::this_thread::yield();
//                     return nullptr;
//                 }
//             }
//             else
//             {
//                 std::this_thread::yield();
//                 return nullptr;
//             }
//         }
//     }
//     void join();
// };

// class Engine
// {
// public:
//     Engine::Engine(std::size_t workerThreads, std::size_t jobsPerThread) : _workers{workerThreads}
//     {
//         std::size_t jobsPerQueue = jobsPerThread;
//         _workers.emplace_back(this, jobsPerQueue, Worker::Mode::Foreground);

//         for (std::size_t i = 1; i < workerThreads; ++i)
//         {
//             _workers.emplace_back(this, jobsPerQueue, Worker::Mode::Background);
//         }

//         for (auto &worker : _workers)
//         {
//             worker.run();
//         }
//     }

//     Worker *randomWorker()
//     {
//         std::uniform_int_distribution<std::size_t> dist{0, _workers.size()};
//         std::default_random_engine randomEngine{std::random_device()()};

//         Worker *worker = &_workers[dist(randomEngine)];

//         if (worker->running())
//         {
//             return worker;
//         }
//         else
//         {
//             return nullptr;
//         }
//     }
//     Worker *threadWorker() { return findThreadWorker(std::this_thread::get_id()); }

// private:
//     std::vector<Worker> _workers;

//     Worker *findThreadWorker(const std::thread::id threadId)
//     {
//         for (auto &worker : _workers)
//         {
//             if (worker.threadId() == threadId)
//             {
//                 return &worker;
//             }
//         }

//         return nullptr;
//     }
// };

// template <typename Func>
// void closure(Job *job, Func function, Job *parent = nullptr)
// {
//     auto jobFunction = [](Job &job)
//     {
//         const auto &function = job.getData<Func>();

//         function(job);

//         // Destroy the bound object after
//         // running the job
//         function.~Func();
//     };

//     job = new (job) Job{jobFunction, parent};
//     job->constructData<Func>(function);
// }

// int main()
// {
//     // Pool pool(52);

//     // Job *job = pool.allocate();
//     // std::string message = "hello!";
//     // closure(job, [message](Job &job)
//     //         { std::cout << message << "\n"; });
//     // job->run();

//     // Job job{[](Job &job)
//     //         {
//     //             std::cout << "FUCK" << std::endl;
//     //             job.onFinished([](Job &job)
//     //                            { std::cout << "job finished!\n"; });
//     //         }};
//     // job.run();

//     // Job *root = pool.allocate();
//     // std::vector<Job *> jobs(43);

//     // std::string message = "hello!";
//     // closure(root, [message](Job &root)
//     //         {
//     //             for (std::size_t i = 0; i < 2; ++i)
//     //             {
//     //                 Job *temp = pool.allocate();
//     //                 closure(
//     //                     temp, [&message](Job &child)
//     //                     { std::cout << message << "\n"; },
//     //                     &root);
//     //             }
//     //         });

//     // root->run();

//     // pool._storage[1].run();
//     // pool._storage[2].run();

//     // 4 threads, 60k jobs
//     Engine engine{4, 60 * 1000};
//     Worker *worker = engine.threadWorker();

//     Job *root = worker->pool().createJob([](Job &job) {});

//     for (std::size_t i = 0; i < 60 * 1000; ++i)
//     {
//         Job *child = worker->pool().createClosureJobAsChild([i](Job &job) {},
//                                                             root);

//         worker->submit(child);
//     }

//     worker->submit(root);
//     worker->wait(root);

//     return 0;
// }