#pragma once
#include "../common/config.h"
#include "../common/deque.h"
#include "../common/spinlock.h"
#include "../task/task.h"
#include "../debug/listener.h"
#include "processer.h"

namespace co {

struct TaskOpt
{
    bool affinity_ = false;
    int lineno_ = 0;
    int dispatch_ = egod_default;
    std::size_t stack_size_ = 0;
    const char* file_ = nullptr;
};

class Scheduler
{
public:
    static Scheduler& getInstance();

    // 获取配置选项
    CoroutineOptions& GetOptions();

    // 创建一个协程
    void CreateTask(TaskF const& fn, TaskOpt const& opt);

    // 当前是否处于协程中
    bool IsCoroutine();

    // 是否没有协程可执行
    bool IsEmpty();

    // 当前协程让出执行权
    void CoYield();

    // 启动调度器
    // @minThreadNumber : 最小调度线程数, 为0时, 设置为cpu核心数.
    // @maxThreadNumber : 最大调度线程数, 为0时, 设置为minThreadNumber.
    //                    如果maxThreadNumber设置为一个较大的值, 则可以在协程中使用阻塞操作.
    void Start(int minThreadNumber = 1, int maxThreadNumber = 0);
    static const int s_ulimitedMaxThreadNumber = 40960;

    // 当前协程总数量
    uint32_t TaskCount();

    // 当前协程ID, ID从1开始（不在协程中则返回0）
    uint64_t GetCurrentTaskID();

    // 当前协程切换的次数
    uint64_t GetCurrentTaskYieldCount();

    // 设置当前协程调试信息, 打印调试信息时将回显
    void SetCurrentTaskDebugInfo(std::string const& info);

    // 获取当前协程的调试信息, 返回的内容包括用户自定义的信息和协程ID
    const char* GetCurrentTaskDebugInfo();

    // 获取当前线程ID.(按执行调度器调度的顺序计)
    int GetCurrentThreadID();

    // 获取当前进程ID.
    int GetCurrentProcessID();

public:
    Task* GetCurrentTask();

private:
    Scheduler();
    ~Scheduler();

    Scheduler(Scheduler const&) = delete;
    Scheduler(Scheduler &&) = delete;
    Scheduler& operator=(Scheduler const&) = delete;
    Scheduler& operator=(Scheduler &&) = delete;

    // 将一个协程加入可执行队列中
    void AddTaskRunnable(Task* tk, int dispatch = egod_default);

    // TODO: dispatcher线程函数
    // 1.根据协程调度排队时间计算负载(抽样跟踪, 将时间戳写入Task结构中), 将高负载的P中的协程steal一些给低负载的P
    // 2.侦测到阻塞的P(单个协程运行时间超过阀值), 将P中的其他协程steal给其他P
    void DispatcherThread();

    Processer* GetProcesser(std::size_t index);

    // deque of Processer, write by start or dispatch thread
    Deque<Processer*> processers_;
    LFLock started_;

    atomic_t<uint32_t> dispatch_robin_index_{0};
    atomic_t<uint32_t> task_count_{0};
    
    int minThreadNumber_ = 1;
    int maxThreadNumber_ = 1;

    // ------------- 兼容旧版架构接口 -------------
public:
    typedef Listener::TaskListener TaskListener;
    inline TaskListener* GetTaskListener() { return Listener::GetTaskListener(); }
    inline void SetTaskListener(TaskListener* listener) { return Listener::SetTaskListener(listener); }

//    // 调度器调度函数, 内部执行协程、调度协程
//    uint32_t Run();
//
//    // 循环Run直到没有协程为止
//    // @loop_task_count: 不计数的常驻协程.
//    //    例如：loop_task_count == 2时, 还剩最后2个协程的时候这个函数就会return.
//    // @remarks: 这个接口会至少执行一次Run.
//    void RunUntilNoTask(uint32_t loop_task_count = 0);
//    
//    // 无限循环执行Run
//    void RunLoop();
//
//    /// sleep switch
//    //  \timeout_ms min value is 0.
//    void SleepSwitch(int timeout_ms);
//
//    /// ------------------------------------------------------------------------
//    // @{ 定时器
//    template <typename DurationOrTimepoint>
//    TimerId ExpireAt(DurationOrTimepoint const& dur_or_tp, CoTimer::fn_t const& fn)
//    {
//        TimerId id = timer_mgr_.ExpireAt(dur_or_tp, fn);
//        DebugPrint(dbg_timer, "add timer id=%llu", (long long unsigned)id->GetId());
//        return id;
//    }
//
//    bool CancelTimer(TimerId timer_id);
//    bool BlockCancelTimer(TimerId timer_id);
//    // }@
//    /// ------------------------------------------------------------------------

    // --------------------------------------------
};

} //namespace co

#define g_Scheduler ::co::Scheduler::getInstance()

namespace co
{
    ALWAYS_INLINE Scheduler& Scheduler::getInstance()
    {
        static Scheduler obj;
        return obj;
    }

    ALWAYS_INLINE CoroutineOptions& Scheduler::GetOptions()
    {
        return CoroutineOptions::getInstance();
    }

} //namespace co