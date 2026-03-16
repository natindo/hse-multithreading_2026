#pragma once

#include <algorithm>
#include <functional>
#include <thread>
#include <vector>

template <typename T>
void ApplyFunction(
    std::vector<T>& data,
    const std::function<void(T&)>& transform,
    const int threadCount = 1) {
  if (data.empty()) {
    return;
  }

  const int safeThreadCount = std::max(1, threadCount);
  const std::size_t workersCount =
      std::min<std::size_t>(static_cast<std::size_t>(safeThreadCount), data.size());

  if (workersCount == 1) {
    for (auto& value : data) {
      transform(value);
    }
    return;
  }

  const std::size_t chunkSize = data.size() / workersCount;
  const std::size_t remainder = data.size() % workersCount;

  std::vector<std::thread> workers;
  workers.reserve(workersCount);

  std::size_t begin = 0;
  for (std::size_t worker = 0; worker < workersCount; ++worker) {
    const std::size_t extra = worker < remainder ? 1 : 0;
    const std::size_t end = begin + chunkSize + extra;

    workers.emplace_back([begin, end, &data, &transform]() {
      for (std::size_t i = begin; i < end; ++i) {
        transform(data[i]);
      }
    });

    begin = end;
  }

  for (auto& worker : workers) {
    worker.join();
  }
}
