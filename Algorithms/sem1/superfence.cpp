#include <iostream>
#include <algorithm>
#include <cstring>
#include <stdexcept>

class Vector {
public:
	Vector(const Vector &a) {
		*this = a;
	}
	Vector() {
		size_ = 0;
		buffer_ = new int[1];
		capacity_ = 1;
	}
	~Vector() {
		delete[] buffer_;
	}
	void push_back(int a) {
		if (size_ >= capacity_) {
			ReallocMemory();
		}
		buffer_[size_] = a;
		size_++;
	}
	int pop_back() {
		return buffer_[size_-- - 1];
	}
	int front() const{
		return buffer_[0];
	}
	int back() const{
		return buffer_[size_ - 1];
	}
	unsigned int size() const{
		return size_;
	}
	int &operator [](int i) {
		if (i >= size_) {
			throw std::runtime_error("vector: index out of range");
		}
		return buffer_[i];
	}
	Vector &operator =(const Vector &a) {
		if (buffer_) {
			delete[] buffer_;
		}
		buffer_ = new int[a.capacity_];
		std::memcpy(buffer_, a.buffer_, a.capacity_ * sizeof(int));
		capacity_ = a.capacity_;
		size_ = a.size_;
		return *this;
	}
private:
	int* buffer_;
	unsigned int size_;
	unsigned int capacity_;
	void ReallocMemory() {
		int* new_buffer = new int[capacity_ << 1];
		std::memcpy(new_buffer, buffer_, capacity_ * sizeof(int));
		delete[] buffer_;
		buffer_ = new_buffer;
		capacity_ = capacity_ << 1;
	}
};

class Stack {
public:
	Stack() {
		buffer_ = Vector();
		size_ = 0;
	}
	Stack(const Stack &a) {
		*this = a;
	}
	void push(int a) {
		size_++;
		buffer_.push_back(a);
	}
	int peek() const{
		return buffer_.back();
	}
	int pop() {
		return buffer_.pop_back();
	}
	unsigned int size() const{
		return size_;
	}
	bool empty() const{
		return size_ == 0;
	}
	Stack &operator =(const Stack &a) {
		buffer_ = a.buffer_;
		size_ = a.size_;
		return *this;
	}
protected:
	Vector buffer_;
	unsigned int size_;
};

class StackWithMaximum {
public:
	StackWithMaximum() {
		size_ = 0;
		main_stack_ = Stack();
		maximum_observer_ = Stack();
	}
	StackWithMaximum(const StackWithMaximum &a) {
		*this = a;
	}
	void push(int a) {
		size_++;
		main_stack_.push(a);
		if (maximum_observer_.size() == 0) {
			maximum_observer_.push(a);
			return;
		}
		maximum_observer_.push(std::max(a, maximum_observer_.peek()));
	}
	int pop() {
		size_--;
		maximum_observer_.pop();
		return main_stack_.pop();
	}
	int peek() {
		return main_stack_.peek();
	}
	int maximum() const{
		return maximum_observer_.peek();
	}
	unsigned int size() const{
		return size_;
	}
	bool empty() const{
		return size_ == 0;
	}
	StackWithMaximum &operator =(const StackWithMaximum &a) {
		main_stack_ = a.main_stack_;
		maximum_observer_ = a.maximum_observer_;
		size_ = a.size_;
		return *this;
	}
private:
	Stack main_stack_;
	Stack maximum_observer_;
	unsigned int size_;
};

class QueueWithMaximum {
public:
	QueueWithMaximum() {
		left_ = StackWithMaximum();
		right_ = StackWithMaximum();
	}
	QueueWithMaximum(const QueueWithMaximum &a) {
		*this = a;
	}
	void push(int a) {
		left_.push(a);
	}
	int pop() {
		if (right_.empty()) {
			pour();
		}
		return right_.pop();
	}
	int front() {
		if (right_.empty()) {
			pour();
		}
		return right_.peek();
	}
	int maximum() const{
		if (left_.empty()) {
			return right_.maximum();
		} else if (right_.empty()) {
			return left_.maximum();
		} else {
			return std::max(left_.maximum(), right_.maximum());
		}
	}
	unsigned int size() const{
		return left_.size() + right_.size();
	}
	QueueWithMaximum &operator =(const QueueWithMaximum &a) {
		left_ = a.left_;
		right_ = a.right_;
		return *this;
	}
private:
	StackWithMaximum left_;
	StackWithMaximum right_;
	void pour() {
		while (!left_.empty()) {
			right_.push(left_.pop());
		}
	}
};

int GetDifference(QueueWithMaximum a, int sum) {
	return a.maximum() * (int)a.size() - sum;
}

int main() {
	int n, w;
	int *fence;
	QueueWithMaximum main_queue;
	std::cin >> n >> w;
	fence = new int[n];
	int queue_sum = 0;
	int i = 0, j = 0, next;
	while (i < n) {
		std::cin >> next;
		fence[i] = next;
		if (j < w) {
			main_queue.push(next);
			queue_sum += next;
			j++;
		}
		i++;
	}
	int min_dif;
	min_dif = GetDifference(main_queue, queue_sum);
	for (int i = w; i < n; i++) {
		queue_sum -= main_queue.pop();
		queue_sum += fence[i];
		main_queue.push(fence[i]);
		min_dif = std::min(min_dif, GetDifference(main_queue, queue_sum));
	}
	std::cout << min_dif;
	delete[] fence;
	return 0;
}