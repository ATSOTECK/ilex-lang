---
layout: default
title: Maps
nav_order: 8
---

# Maps
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---
## Maps

Maps store key, value pairs. Maps take strings and numbers as valid key types. Maps are unordered. Meaning the values are not stored in the order they are added.

```go
aMap := { "key1": 12, "key2": "word" }
```

### Indexing

Indexing a map uses square brackets. If you try to use a key that does not exist in the map null will be returned.

```go
aMap := { "key1": 12, "key2": "word" }
aMap["key1"] // 12
aMap["woo"] // null
```

Adding a value and updating a value in the map uses the same syntax.

```go
aMap := { "key1": 12, "key2": "word" }
aMap["key1"] = 19
println(aMap["key1"]) // 19

aMap["key3"] = "Yeet"
println(aMap["key3"]) // Yeet
```

### map.get(key, defaultValue (optional)): value

This is like using the square brackets except if the key doesn't exist an optional default value will be returned. The default return value if no key is found is null.

```go
empty := {}

empty.get("key") // null
empty.get("key", 12) // 12
empty.get("key", "Key not found") // Key not found
```

### map.size(): number

Returns the number of items in the map.

```go
aMap := { "key1": 12, "key2": "word" }
println(aMap.size()) // 2
```

### map.maxSize(): number

Returns the maximum amount of items the map can currently hold. As items are added to the map more space will periodically be allocated so this number will change.

```go
aMap := { "key1": 12, "key2": "word" }
println(aMap.maxSize()) // 8
```

### map.toString(): string

Converts the map to a string and returns it.

```go
aMap := { "key1": 12, "key2": "word" }
println(aMap.toString()) // {"key2": "word", "key1": 12}
```

### map.keys(): value[]

Returns all of the keys in the map as an array.

```go
aMap := { "key1": 12, "hello": "world" }
aMap.keys() // ["hello", "key1"]
```

### map.values(): value[]

Returns all of the values in the map as an array.

```go
aMap := { "key1": 12, "hello": "world" }
aMap.values() // ["world", 12]
```

### map.delete(): bool

Deletes an item from the map. Returns true if the item is deleted, false otherwise.

```go
aMap := { "key0": 0, "key1": 1, "key2": 2 }
aMap.delete("key1") // true
println(aMap) // {"key2": 2, "key0": 0}
aMap.delete("yeet") // false
```

### map.exists(key): bool

Returns whether or not the given key exists.

```go
aMap := { "key0": 0, "key1": 1, "key2": 2 }
aMap.exists("key1") // true
aMap.exists("yeet") // false
```

### map.isEmpty(): bool

Returns whether or not the map has any items in it.

```go
map1 := {}
map2 := {"key": 12}

map1.isEmpty() // true
map2.isEmpty() // false
```

### map.shallowCopy(): map

Creates a shallow copy of the map an returns it.

### map.deepCopy(): map

Created a deep copy of the map and returns it.

