---
layout: default
title: Arrays
nav_order: 7
---

# Arrays
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---
## Arrays

Arrays in Ilex allow you to store multiple items of different types. Arrays are automatically resized when you add or delete items. Arrays in Ilex are very similar to arrays in TypeScript.

```go
array := [1, 2, 3, "yeet", true, [12], null]
```

### Indexing 

Arrays in ilex are 0-index meaning 0 is the first item in the array, 1 is the 2nd and so on. Indexing is done using square brackets.

```ts
println(array[2]) // Print the 3rd item.
```

Updating an item in an array is also done with the square brackets.

```ts
array[1] = 12 // Was 2, now 12.
```

### Slicing

Ilex supports slicing to return a slice, or part, of an array. The start index is inclusive and the end index is exclusive.

```cs
[1, 2, 3, 4, 5][1:]; // [2, 3, 4, 5]
[1, 2, 3, 4, 5][:4]; // [1, 2, 3, 4]
[1, 2, 3, 4, 5][1:4]; // [2, 3, 4]
[1, 2, 3, 4, 5][2:4]; // [3, 4]
```

### array.push(value)

To add an item to an array use the `push` function.

```ts
array := ['hi']
array.push('there') // ['hi', 'there']
```

### array.insert(index: number, value)

To insert an item into a specific spot use the `insert` function.

```ts
array := [1, 2]
array.push(1, 'hi') // [1, 'hi', 2]
```

### array.make(count: number, value (optional)): array

Creates an array and allocates `count` number of items in the array. If `value` is specified the array is filled with that value.

```ts
array := []
array.make(5, 0) // [0, 0, 0, 0, 0]
```

### array.fill(value (optional))

Fills the array with the specified `value`. If no value is specified then 0 is used.

```ts
array := [1, 2, 3]
array.fill() // [0, 0, 0]
array.fill(12) // [12, 12, 12]
```

### array.len(): number

Returns the length of the array.

```ts
array := [3, 4, 5]
array.len() // 3
```

### array.toString(): string

Returns the array as a string.

```ts
array := [3, 4, 5]
array.toString() // '[3, 4, 5]'
```

### array.pop(): value

Removes the last item in the array and returns it.

```ts
array := [5, 6, 7]
array.pop() // Returns 7, array is now [5, 6]
```

### array.erase(index: number, count: number (optional))

Removes `count` items from the array starting at `index`. If count is not specified it defaults to 1.

```ts
array := [5, 6, 7, 8, 9]
array.erase(1) // [5, 7, 8, 9]
array.erase(2, 2) // [5, 7]
```

### array.remove(value): bool

Removes the specified value. Returns true on success, false if it could not find the value.

```ts
array := [5, 6, 7, 8, 9]
array.remove(7) // [5, 6, 8, 9]
```

### array.contains(value): bool

Returns whether or not the array contains the specified value.

```ts
array := ['abc', '123']
array.contains('123') // true
array.contains('xyz') // false
```

### array.indexOf(value, startIndex: number (optional)): number

Returns the first index of the given value starting the search at `startIndex`. If no startIndex is given it defaults to 0. If the value can't be found -1 is returned.

```ts
array := [1, 2, 3, 2, 4, 5, 6]
array.indexOf(2) // 1
array.indexOf(2, 2) // 3
array.indexOf(7) // -1
```

### array.reverse()

Reverses the order of the array.

```ts
array := [1, 2, 3]
array.reverse() // [3, 2, 1]
```

### array.sort(ascending: bool (optional))

Sorts the array. The array must be all numbers. The optional bool `ascending` specifies if it is sorted in ascending or descending order. By default it sorts ascending. This function is likely to change.

```ts
array := [6, 3, 7, 2]
array.sort() // [2, 3, 6, 7]
array.sort(false) // [7, 6, 3, 2]
```

### array.join(delim: string (optional)): string

Combines the items in the array into a single string seperating the items with the specified `delim`. The default delimiter is ', '.

```ts
array := [1, 2, 3, 4]
array.join() // '1, 2, 3, 4'
array.join(' - ') // '1 - 2 - 3 - 4'
array.join('') // '1234'
```

### array.clear()

Deletes all items in the array.

```ts
array := [7, 8, 9]
array.clear() // []
```

### array.isEmpty(): bool

Returns whether or not the array is empty.

```ts
array := [7, 8, 9]
array.isEmpty() // false
array.clear()
array.isEmpty() // true
```

### array.shallowCopy(): array

Using the '=' operator creates a reference to the array. Meaning if you modify one of the arrays both will be changed.

```ts
arr1 := [1, 2, 3]
arr2 := arr1
arr2[0] = 12
println(arr1) // [12, 2, 3]
println(arr2) // [12, 2, 3]
```

To get around this you can make a copy of the array.

```ts
arr1 := [1, 2, 3]
arr2 := arr1.shallowCopy()
arr2[0] = 12
println(arr1) // [1, 2, 3]
println(arr2) // [12, 2, 3]
```

However if the array has an array in it the reference issue will still apply. To fis that use `array.deepCopy()`

### array.deepCopy(): array

```ts
arr1 := [[1, 2, 3]]
arr2 := arr1.shallowCopy()
arr2[0][1] = 12
println(arr1) // [[1, 12, 3]]
println(arr2) // [[1, 12, 3]]
```

To get around that use `array.deepCopy()`

```ts
arr1 := [[1, 2, 3]]
arr2 := arr1.deepCopy()
arr2[0][1] = 12
println(arr1) // [[1, 2, 3]]
println(arr2) // [[1, 12, 3]]
```

### array.allTruthy(startIndex: number (optional), endIndex: number (optional)): bool

Returns true if all the items in the between `startindex` and `stopIndex` array are truthy. By default `startIndex` is 0 and `endIndex` is the index of the last item in the array. The start index is inclusive and the end index is exclusive.

```ts
array := [1, true, 'yeet', [1]]
array.allTruthy() // true

array[0] = 0
array.allTruthy() // false
```

### array.allFalsy(startIndex: number (optional), endIndex: number (optional)): bool

Returns true if all the items in the between `startindex` and `stopIndex` array are falsy. By default `startIndex` is 0 and `endIndex` is the index of the last item in the array. The start index is inclusive and the end index is exclusive.

```ts
array := [0, false, '', null, []]
array.allFalsy() // true

array[0] = 1
array.allFalsy() // false
```

### array.anyTruthy(startIndex: number (optional), endIndex: number (optional)): bool

Returns true if any of the items in the between `startindex` and `stopIndex` array are truthy. By default `startIndex` is 0 and `endIndex` is the index of the last item in the array. The start index is inclusive and the end index is exclusive.

```ts
array := [0, true, '', null, []]
array.anyTruthy() // true

array[1] = false
array.anyTruthy() // false

array = [1, true, 'nice']
array.anyTruthy() // true
```

### array.forEach(callback: function)

Calls `callback` on each item in the array. The callback function expects one argument which will be the current value. This function will change in the future.

```ts
array := [1, 2, 3, 4, 5]

array.forEach(fn |value| -> {
    println('Value:', value)
})
```

### array.map(callback: function): array

Calls `callback` on each item in the array. The callback function expects one argument which will be the current value. The function returns a new array made up up values generated by the callback function. This function will change in the future.

```ts
array := [1, 2, 3, 4, 5]

squared := array.map(fn |n| -> {
    return n ** 2
})

println(squared) // [1, 4, 9, 16, 25]
```

### array.filter(callback: function): array

Calls `callback` on each item in the array. The callback function expects one argument which will be the current value. The function returns a new array made up up values that weren't filtered out by the callback. The callback function must return a bool. This function will change in the future.

```ts
array := [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

evens := array.filter(fn |n| n % 2 == 0)

println('Even numbers only:', evens) // [2, 4, 6, 8, 10]
```

### array.reduce(callback: function, initialValue: value (optional)): value

Calls `callback` on each item in the array passing the return value from the calculation on the preceding item. The final result of running the callback across all items of the array is a single value. An optional initial value can be specified. If not specified the item at the start of the array will be used. This function will change in the future.

```ts
array := [1, 2, 3, 4, 6, 8, 10]
array.reduce(fn |accumulator, item| accumulator + item) // 34
array.reduce(fn |accumulator, item| accumulator + item, 100) // 134
```

### array.allOf(callback: funnction): bool

Calls `callback` on each item in the array. The callback function expects one argument which will be the current value. This function returns true if all of the values passed to the callback function return true.

```ts
array := [2, 4, 6, 8, 10]
array.allOf(fn |n| n % 2 == 0) // true
array[0] = 1
array.allOf(fn |n| n % 2 == 0) // false
```

### array.noneOf(callback: function): bool

Calls `callback` on each item in the array. The callback function expects one argument which will be the current value. This function returns true if all of the values passed to the callback function return false.

```ts
array := [1, 3, 5, 7, 9]
array.noneOf(fn |n| n % 2 == 0) // true
array[0] = 2
array.noneOf(fn |n| n % 2 == 0) // false
```

### array.anyOf(callback: function): bool

Calls `callback` on each item in the array. The callback function expects one argument which will be the current value. This function returns true if any of the values passed to the callback function return true.

```ts
array := [2, 3, 5, 7, 9]
array.anyOf(fn |n| n % 2 == 0) // true
array[0] = 1
array.anyOf(fn |n| n % 2 == 0) // false
```

