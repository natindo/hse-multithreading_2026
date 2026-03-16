# Домашнее задание для лекции по теме "Примитивы синхронизации"

В этой домашней работе два варианта:

* Вариант #1 -- задача buffered_channel.

* Вариант #2 -- задача unbuffered_channel.

Номер варианта вычисляется как остаток от деления на 2 вашего номера в списке группы.

cmake -S . -B build -DHW2_BUILD_TESTS=ON -DHW2_BUILD_BENCHMARKS=ON -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build -j
./build/apply_function_benchmarks