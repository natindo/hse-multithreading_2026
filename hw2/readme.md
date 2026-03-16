# Домашнее задание для лекции по теме "Процессы и потоки"

Реализуйте следующий метод:
```cpp
template <typename T>
void ApplyFunction(std::vector<T>& data, const std::function<void(T&)>& transform, const int threadCount = 1);

```

Данный метод должен применить переданную функцию `transform` к каждому элементу вектора `data`. `threadCount` задает количество потоков, которое нужно использовать для применения функции. Если число потоков превышает число элементов, то число потоков следует взять равным числу элементов.

Напишите тесты для вашей реализации с использованием [gtest](https://google.github.io/googletest/).

Напишите бенчмарк для вашей реализации с использованием [benchmark](https://google.github.io/benchmark/user_guide.html).

В бенчмарке отразите две ситуации -- когда однопоточная версия стабильно быстрее многопоточной и обратную ситуацию. Достигните этого как с помощью подбора размера вектора `data`, так и с помощью подбора функции `transform`.

## Структура решения

- `apply_function.h` -- реализация `ApplyFunction`.
- `tests/apply_function_test.cpp` -- тесты на `gtest`.
- `benchmarks/apply_function_benchmark.cpp` -- бенчмарки на `benchmark`.

## Сборка и запуск

Только тесты:

```bash
cmake -S . -B build-tests -DHW2_BUILD_TESTS=ON -DHW2_BUILD_BENCHMARKS=OFF
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

Тесты и бенчмарки:

```bash
cmake -S . -B build -DHW2_BUILD_TESTS=ON -DHW2_BUILD_BENCHMARKS=ON -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build -j
./build/apply_function_benchmarks
```

## Сценарии бенчмарка

- `BM_SmallVectorLightTransform`: маленький вектор и очень легкая функция, где накладные расходы потоков обычно делают однопоточный запуск быстрее.
- `BM_LargeVectorHeavyTransform`: большой вектор и вычислительно тяжелая функция, где многопоточность обычно выигрывает.
