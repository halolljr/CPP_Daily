#include "ThreadPool.h"
void ThreadPool::start() {
	for (int i = 0; i < thread_num_; ++i) {
		//ÿһ���̶߳�ִ������ѭ����ʹ��emplace_back��Ϊ�̲߳�֧�ֿ�����ֵ���߿������죩
		pool_.emplace_back(
			[this]() {
				while (!this->stop_.load()) {
					Task task;
					{
						std::unique_lock<std::mutex> cv_mt(cv_mt_);
						//lambda����true������������������ڼ���ͷ�uique_lock��
						this->cv_lock_.wait(cv_mt, [this]() {
							return this->stop_.load() || !this->tasks_.empty();
							});
						//�ж��������Ϊ����˵������Ϊ�յ�ֹͣ�ź�����ֱ�ӷ����˳�
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
			}//Lambda���ʽ
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
	//����ֵ����
	using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
	if (stop_.load()) {
		return std::future<RetType>{};
	}
	//����bind()��װ��һ��RetType()���͵�
	auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	std::future<RetType> ret = task->get_future();
	{
		std::lock_guard<std::mutex> cv_mt(cv_mt_);
		//���������Ҫ����һ��void(void),���ǵ���һ��RetType(void), ��˽���lambda���ʽ���һ��α�հ�
		tasks_.emplace([task] { (*task)(); });
	}
	cv_lock_.notify_one();
	return ret;
}