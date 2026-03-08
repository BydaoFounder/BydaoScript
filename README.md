*BydaoScript* - экспериментальный скриптовый язык с ролевым полиморфизмом.

Сам язык, его компилятор в байткод и виртуальная машина - в процессе разработки. Все средства еще очень далеки от реального использования.

Основная цель разработки языка - максимально компактный и семантически ясный синтаксис.

Пример скрипта:

```
use Sys  // использование пакета Sys (System)
var sum = 0
var i = 0
var t1 = Sys.time()
while i < 1000000 next i = i + 1 {  // оператор в части next выполняется в конце цикла (на следующей итерации)
    sum = sum + i
}
t1 = Sys.time() - t1
Sys.outln( 'time: ' + t1 + ' msec, sum = ' + sum )

```

Другой пример:

```
use Sys

var sum
var t

Sys.outln( 'Test: enum Int.range' )
sum = 0
t = Sys.time()
enum Int.range( 0, 1000000 ) as v {    // перечисление элементов диапазона целых чисел как переменую v
    sum = sum + v
}
t = Sys.time() - t
Sys.outln( 'time: ' + t + ' msec, sum = ' + sum )

Sys.outln( 'Test: iter Int.range' )
sum = 0
t = Sys.time()
iter Int.range( 0, 1000000 ) as it {    // итерация диапазона целых чисел как итератор it
    sum = sum + it.value
}
t = Sys.time() - t
Sys.outln( 'time: ' + t + ' msec, sum = ' + sum )

```
Оператора for в языке пока нет и, скорее всего, не будет :-) 

Идеи относительно ролевого полиморфизма изложены в папке "/docs".
