#include <iostream>
#include <atomic>

template<typename T>
class SharedPtr {
public:
	SharedPtr(T* ptr = nullptr): ptr_(ptr), ref_count_(ptr ? new std::atomic<size_t>(1) : nullptr) {}

	SharedPtr(const SharedPtr& other) {
		ptr_ = other.ptr_;
		ref_count_ = other.ref_count_;
		if(ref_count_) {
			// 此处使用std::memory_order_relaxed的原因是，此处只是对引用计数进行了加1操作，不需要保证原子性
			ref_count_->fetch_add(1, std::memory_order_relaxed);
		}
	}

	SharedPtr& operator=(const SharedPtr& other) {
		// 需要判断是否是自赋值
		if(this != &other) {
			release();
			ptr_ = other.ptr_;
			ref_count_ = other.ref_count_;
			if(ref_count_) {
				// 此处使用std::memory_order_relaxed的原因是，此处我们只关心原子地增加引用计数，而不关心与其他原子操作之间的顺序
				ref_count_->fetch_add(1, std::memory_order_relaxed);
			}
		}

		return *this;
	}

	~SharedPtr() {
		release();
	}

	T& operator*() const { return *ptr_; }
	T* operator->() const { return ptr_; }

	size_t use_count() const { return ref_count_ ? ref_count_->load(std::memory_order_relaxed) : 0; }

	void reset() {
		release();
		ptr_ = nullptr;
		ref_count_ = nullptr;
	}

	void reset(T* ptr) {
		if(ptr != ptr_) {
			release();
			ptr_ = ptr;
			ref_count_ = ptr ? new std::atomic<size_t>(1) : nullptr;
		}
	}

private:
	void release() {
		// 需要注意的是，fetch_sub函数的返回值是操作之前的值，所以这里的判断条件是 '== 1'
		// 此处使用std::memory_order_release的原因是，此处对引用计数进行了减1操作，需要保证原子性
		if(ref_count_ && ref_count_->fetch_sub(1, std::memory_order_release) == 1) {
			// std::atomic_thread_fence的作用是，保证前面的引用计数减1操作先于对ptr_的delete操作
			std::atomic_thread_fence(std::memory_order_acquire);
			delete ptr_;
			delete ref_count_;
		}
	}

	T* ptr_;
	std::atomic<size_t>* ref_count_;
};
