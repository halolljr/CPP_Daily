#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <queue>
#include <vector>
//����һ����ֹ��������Ϳ�����ֵ�Ļ���
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
	//����ģʽ--�ǵ�ֻ��ʼ��һ�Σ��������øú������ȡ��һ�ε�ȫ����
	//C++11֮����ڷ���һ���ֲ���̬���������Ż�
	//��֤����̵߳���ͬһ������ֻ������һ��ʵ��
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
	using Task = std::packaged_task<void(void)>;	//��������
	std::atomic_int thread_num_;	//�����߳���
	std::queue<Task> tasks_;	//�������
	std::vector<std::thread> pool_;	//�̶߳���
	std::atomic_bool stop_;	//�̳߳��Ƿ�ֹͣ
	std::mutex cv_mt_;	//�����̵߳�����
	std::condition_variable cv_lock_;	//�����̵߳Ļ���
};
