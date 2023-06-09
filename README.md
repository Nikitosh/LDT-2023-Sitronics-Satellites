# Описание решения кейса 

## Задача

По информации о расписании движения спутников и интервалах их видимости с наземных станций составить расписание фотосъемки и передачи данных на Землю таким образом, чтобы передать на Землю как можно больше данных. 

## Реализованная функциональность
* Решение задачи, которое максимизирует суммарный объем данных в два этапа: 
  * Построение приближенного решения при помощи жадного алгоритма, использующего взвешенный алгоритм Куна и эвристики
  * Итеративная оптимизация полученного решения методом локальных оптимизаций
 
## Лучший результат
* Теоретический максимум переданных данных: `838'986.27 GiB`
* Наибольшее количество переданных данных: `822'711.39 GiB`, 98.1% от теоретического максимума
* Ссылка на архив с построенным расписанием (размер файла ~175 MiB): https://disk.yandex.ru/d/Fw83tseIFbBLkA.

## Особенности проекта

### Алгоритмическое решение
* Основано на идее разделения на отрезки, концами которых являются начала и концы интервалов видимости станций и спутников. Эти отрезки в рамках задачи считаются единым целым (то есть на которых, в частности, невозможно изменение назначений действий для спутников и станций)
* В рамках одного отрезка времени для построения соответствия между спутниками и станциями применяется взвешенный алгоритм Куна для поиска максимального паросочетания между спутниками и станциями.
* Каждому спутнику, присваивается вес, зависящий от объема заполненного ЗУ, скорости заполнения ЗУ и скорости сброса данных на Землю. Чем больше вес спутника, тем более приоритетно скинуть данные с данного спутника.
* Режим до-оптимизации готового решения
  * Применяется метод локальных оптимизаций на уже готовое решение; в качестве мутации используется переключение в режим съемки для выбранного спутника 
  * В теории, исходное решение может быть реализовано другим алгоритмом
* Линейное время работы относительно входного периода времени
  * Основное решение работает за 4.5 секунды при запуску на Macbook Pro M1  
  * Позволяет строить расписание на целый год за пару минут
  * Позволяет быстро получить новое расписание в случае критических изменений
* Расширяемость для добавления новых ограничений
  * Достаточно одной новой записи в конфигурации для добавления нового спутника
  * Легкая адаптация для конфигурации, при которой на спутниках уже имеются какие-то данные
* Расширяемость для тестирования новых гипотез
  * Достаточно реализации одного интерфейса для добавления новых решающих стратегий
* Отдельно реализован механизм квантования времени, разделения на независимые отрезки и решение задачи на них (дало худшие результаты)

### Верификатор
* Также дополнительно реализован верификатор, который проверяет, что выведенные отрезки действительно являются вложенными в соответствующие отрезки видимости и не пересекаются друг с другом (тем самым, никакой объект не пытается совершить два действия одновременно)
* По результатам работы верификатор выводит объем скинутых данных (должен совпадать с тем, что выдает алгоритм)

## Основной стек технологий

* C++, Nlohmann JSON

### Сборка решения

1. Склонируйте репозиторий  
~~~
git clone https://github.com/Nikitosh/LDT-2023-Sitronics-Satellites.git
~~~
2. Для сборки алгоритмической части решения выполните:
~~~
cd src
g++ -O2 -std=gnu++17 -g solution.cpp -o solution
~~~
3. Для запуска алгоритмической части выполните (результат будет сохранен в `../Results/...`):
~~~
./solution
~~~
либо, для запуска уже собранных исполняемых файлов (важно: запуск должен производиться из директории `src`).
~~~
./solution_*platform* 
~~~
4. Для сборки верифицирующей части решения выполните:
~~~
cd src
g++ -O2 -std=gnu++17 -g verifier.cpp -o verifier
~~~
5. Для запуска верифицирующей части выполните:
~~~
./verifier
~~~

## Команда 

* Никита Подгузов [telegram](https://t.me/Nikitosh)
* Всеволод Степанов [telegram](https://t.me/Tehnar5)
