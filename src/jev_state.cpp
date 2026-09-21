#include "jev_state.hpp"

namespace duckdb {

JevThreadPool::JevThreadPool(idx_t worker_count_p) : worker_count(worker_count_p) {
	workers.reserve(worker_count);
	for (idx_t i = 0; i < worker_count; i++) {
		workers.emplace_back([this]() { WorkerLoop(); });
	}
}

JevThreadPool::~JevThreadPool() {
	{
		std::unique_lock<std::mutex> guard(lock);
		shutting_down = true;
	}
	work_available.notify_all();
	for (auto &worker : workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

void JevThreadPool::WorkerLoop() {
	while (true) {
		std::packaged_task<void()> task;
		{
			std::unique_lock<std::mutex> guard(lock);
			work_available.wait(guard, [this]() { return shutting_down || !tasks.empty(); });
			if (tasks.empty()) {
				// Shutting down: the queue is drained, so no future is left unfulfilled.
				return;
			}
			task = std::move(tasks.front());
			tasks.pop_front();
		}
		task();
	}
}

std::future<void> JevThreadPool::Submit(std::function<void()> task) {
	std::packaged_task<void()> packaged(std::move(task));
	auto future = packaged.get_future();
	{
		std::unique_lock<std::mutex> guard(lock);
		tasks.push_back(std::move(packaged));
	}
	work_available.notify_one();
	return future;
}

JevState &JevState::Get() {
	// Deliberately leaked: the pool's threads must not be joined from a static
	// destructor while DuckDB is still tearing down.
	static JevState *state = new JevState();
	return *state;
}

bool JevState::Lookup(const string &key, string &answer) {
	std::unique_lock<std::mutex> guard(lock);
	auto entry = cache.find(key);
	if (entry == cache.end()) {
		return false;
	}
	answer = entry->second;
	return true;
}

void JevState::Store(const string &key, const string &answer, idx_t max_entries) {
	std::unique_lock<std::mutex> guard(lock);
	auto inserted = cache.insert(make_pair(key, answer));
	if (!inserted.second) {
		inserted.first->second = answer;
		return;
	}
	insertion_order.push_back(key);
	while (max_entries > 0 && cache.size() > max_entries && !insertion_order.empty()) {
		auto &oldest = insertion_order.front();
		if (oldest != key) {
			cache.erase(oldest);
		}
		insertion_order.pop_front();
	}
}

void JevState::Clear() {
	std::unique_lock<std::mutex> guard(lock);
	cache.clear();
	insertion_order.clear();
}

idx_t JevState::CachedAnswers() {
	std::unique_lock<std::mutex> guard(lock);
	return cache.size();
}

shared_ptr<JevThreadPool> JevState::Pool(idx_t workers) {
	std::unique_lock<std::mutex> guard(lock);
	if (!pool || pool->WorkerCount() != workers) {
		pool = make_shared_ptr<JevThreadPool>(workers);
	}
	return pool;
}

} // namespace duckdb
