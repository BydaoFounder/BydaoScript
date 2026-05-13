Определение типов будет выглядеть примерно так:

```
type Gender oneof {
    male = 'M',
    female = 'F'
}
type People {
    gender: Gender = male,
    firstName: Str = '',
    lastName: Str = '',
    birthday: DateTime = null
}
```
Варианты:

- itemtype Name oneof { ... }


    перечисление (один вариант из набора)

- type Name { ... }

    структура/запись (все поля вместе)

- в будущем можно добавить другие классификаторы типа:

   type Name union { ... }
   type Name tuple { ... }
   и т.д.
