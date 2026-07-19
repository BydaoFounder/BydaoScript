# Array - массив

Массивы - это наборы значений любого типа. В BydaoScript массивы могут быть
одного из двух типов:

- индексный массив;
- ассоциативный массив.

Если массив пустой (не содержит элементов), его тип **не определен**. Тип массива
определяется тем, какие элементы он содержит.

## Индексный массив

Индексный массив - это упорядоченный набор (список, коллекция) элементов, в котором
каждый элемент однозначно определяется своим **индексом**. **Индекс** - это целое
число или значение, которое можно привести к целому числу, которое определяет
позицию элемента в массиве. Индексация (нумерация) элементов начинается с 0.

В BydaoScript индексом может быть любое выражение, значением которого является
целое число.

Примеры:

```
// Статическое определение индексного массива
var a = [ 11, 22, 33 ]  // массив из трех элементов

// Динамическое формирование индексного массива
var a = []              // пустой массив
enum Int.range(0,10) as i {
    v.append( i )       // добавление в конец массива числа i
}

// Получение элемента из массива
var b = a[2]            // получить значение с индексом 2

// Замена значения
const INDEX = 2
a[ INDEX ] = 'second'   // замена значения с индексом 2 на строку

// Перебор всех элементов массива
enum a as idx, val {
    Sys.outln( 'Index: ' + idx + ', value: ' + val )
}
```


## Ассоциативный массив

Ассоциативный массив - это набор элементов, в котором каждый элемент имеет уникальный
символьный **ключ**. **Ключ** - это строка символов, уникальная в пределах одного массива,
которая однозначно определяет доступ к связанному значению. Фактически, ассоциативный
массив представляет собой набор данных в формате *"ключ-значение"*.

В BydaoScript ключом может быть любое выражение, значением которого является строка.

Примеры:

```
// Статическое определение ассоциативного массива
var list = [
    '11': [
        'name': 'John'
    ],
    '22': [
        'name': 'Peter'
    ],
    '33': [
        'name': 'Thomas'
    ],
    '44': [
        'name': 'Robert'
    ],
]

// Доступ к элементам ассоциативного массива
var b = a['11']     // b = [ 'name': 'John' ]
var key = '22'
a[ key ]['name'] = 'Paul'

```

## Функции массивов

size(): Int

toString(): String

get( key: String ): Any

set( key: String, val: Any ): Void

slice( start: Int, count: Int = null ): Array

merge( arr: Array ): Array

indexOf( val: Any ): Int

keyOf( val: Any ): String

keys(): Array

values(): Array

forEach( callback: Func ): Void

hasValue( val: Any ): Bool

hasKey( key: String ): Bool

sort( callback: Func( Any ): Bool = null ): Void

ksort( callback: Func( String ): Bool = null ): Void

filter( callback: Func(String,Any):Bool ): Array

filterKey( callback: Func(String):Bool ): Array

filterValue( callback: Func(Any):Bool ): Array

append( val: Any ): Void

insertAt( pos: Int, val: Any ): Void

removeAt( pos: Int ): Void

takeFirst(): Any

removeFirst(): Void

takeLast(): Any

removeLast(): Void

takeKey( key: String ): Any

removeKey( key: String ): Void

hasValue( val: Any ): Bool

hasKey( key: String ): Bool

toJson( options: Int = Json.COMPACT ): String
