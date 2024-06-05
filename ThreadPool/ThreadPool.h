#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <vector>
//定义一个阻止拷贝构造和拷贝赋值的基类
class NoneCopy {
public:
	~NoneCopy() {}
protected:
	NoneCopy() {}
private:
	NoneCopy(const NoneCopy&) = delete;
	NoneCopy& operator=(const NoneCopy&) = delete;
};

class ThreadPool :public NoneCopy {
public:
	~ThreadPool() {
		stop();
	}
	//单例模式--记得只初始化一次，后续调用该函数会读取上一次的全部。
	//C++11之后对于返回一个局部静态变量做了优化
	//保证多个线程调用同一个函数只会生成一个实例
	static ThreadPool& instance() {
		static ThreadPool ins;
		return ins;
	}
protected:
	void start();
	void stop();
	template<class F,class... Args>
	auto commit(F&& f, Args&&... args) -> 
	std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))>;
private:
	ThreadPool(unsigned int num = std::thread::hardware_concurrency()) :stop_(false) {
		if (num <= 1) {
			thread_num_ = 2;
		}
		else {
			thread_num_ = num;
		}
		start();
	}
	using Task = std::packaged_task<void(void)>;	//任务类型
	std::atomic_int thread_num_;	//空闲线程数
	std::queue<Task> tasks_;	//任务队列
	std::vector<std::thread> pool_;	//线程队列
	std::atomic_bool stop_;	//线程池是否停止
	std::mutex cv_mt_;	//控制线程的休眠
	std::condition_variable cv_lock_;	//控制线程的唤醒
};
