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


Рещультаты бенчмарков 
```
Unable to determine clock rate from sysctl: hw.cpufrequency: No such file or directory
This does not affect benchmark measurements, only the metadata output.
***WARNING*** Failed to set thread affinity. Estimated CPU frequency may be incorrect.
2026-03-23T07:21:13+03:00
Running ./build/apply_function_benchmarks
Run on (8 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x8)
Load Average: 7.69, 6.23, 8.09
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_SmallVectorLightTransform/1        987 ns          985 ns       707335
BM_SmallVectorLightTransform/8      92942 ns        67533 ns        10171
BM_LargeVectorHeavyTransform/1 1143204250 ns   1139786000 ns            1
BM_LargeVectorHeavyTransform/8  203242872 ns      2475990 ns          100
```

Результаты тестирования
```
Running main() from /tmp/googletest-20240731-4513-2m6gxg/googletest-1.15.2/googletest/src/gtest_main.cc
[==========] Running 5 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 5 tests from ApplyFunctionTest
[ RUN      ] ApplyFunctionTest.AppliesTransformSingleThread
[       OK ] ApplyFunctionTest.AppliesTransformSingleThread (0 ms)
[ RUN      ] ApplyFunctionTest.AppliesTransformMultiThread
[       OK ] ApplyFunctionTest.AppliesTransformMultiThread (0 ms)
[ RUN      ] ApplyFunctionTest.UsesElementCountWhenThreadCountTooLarge
[       OK ] ApplyFunctionTest.UsesElementCountWhenThreadCountTooLarge (0 ms)
[ RUN      ] ApplyFunctionTest.WorksForEmptyVector
[       OK ] ApplyFunctionTest.WorksForEmptyVector (0 ms)
[ RUN      ] ApplyFunctionTest.NonPositiveThreadCountFallsBackToSingleThread
[       OK ] ApplyFunctionTest.NonPositiveThreadCountFallsBackToSingleThread (0 ms)
[----------] 5 tests from ApplyFunctionTest (0 ms total)

[----------] Global test environment tear-down
[==========] 5 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 5 tests.
```