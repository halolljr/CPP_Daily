#include "ThreadPool.h"
void ThreadPool::start() {
	for (int i = 0; i < thread_num_; ++i) {
		//每一个线程都执行如下循环（使用emplace_back因为线程不支持拷贝赋值或者拷贝构造）
		pool_.emplace_back(
			[this]() {
				while (!this->stop_.load()) {
					Task task;
					{
						std::unique_lock<std::mutex> cv_mt(cv_mt_);
						//lambda返回true即解除阻塞（在阻塞期间会释放uique_lock）
						this->cv_lock_.wait(cv_mt, [this]() {
							return this->stop_.load() || !this->tasks_.empty();
							});
						//判断如果队列为空则说明是因为收到停止信号所以直接返回退出
						if (this->tasks_.empty()) {
							return;
						}
						task = std::move(this->tasks_.front());
						this->tasks_.pop();
					}
					--this->thread_num_;
					task();
					++this->thread_num_;
				}
			}//Lambda表达式
		);

	}
}

void ThreadPool::stop() {
	stop_.store(true);
	cv_lock_.notify_all();
	for (auto& td : pool_) {
		if (td.joinable()) {
			//std::cout << "join thread..." << std::endl;
			td.join();
		}
	}
}

template<class F, class... Args>
auto ThreadPool::commit(F&& f, Args&&... args)->std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
	//返回值类型
	using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
	if (stop_.load()) {
		return std::future<RetType>{};
	}
	//借助bind()封装成一个RetType()类型的
	auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	std::future<RetType> ret = task->get_future();
	{
		std::lock_guard<std::mutex> cv_mt(cv_mt_);
		//任务队列需要的是一个void(void),我们的是一个RetType(void), 因此借助lambda表达式完成一个伪闭包
		tasks_.emplace([task] { (*task)(); });
	}
	cv_lock_.notify_one();
	return ret;
}