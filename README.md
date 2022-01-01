<img src="https://github.com/OSIClang/compiler/blob/master/logo/logo_with_font_small.png">

# OSIC Language Interpreter
This Version is the long planed reboot of OSIC Language and moves from a JAVAish Syntax to his very own more simple one.
The Language is designed to create fast and simple Web Apps and API with a built-in Server and an extensible Package System.

The actual implementation is Version 21, a typical internal known as O21.

### Builds

[![Build Status](https://travis-ci.org/travis-ci/travis-web.svg?branch=master)](https://travis-ci.org/travis-ci/travis-web)

### Binaries and Informations
For further information, news and binaries, feel free to visit our major website [osiclang.com](https://osiclang.com)


# Language Concept

### Everything is a method

OSIC doesn't have classes or methods, not even variables.
Because at least everything is a method. This makes it fast to write and develop.

```
let a = "hello"
```
This is a Method that simply returns a string (every language do it this way!)

```
let shrink = -> n -> {
  ret height - n      // return statement
}
```
This is a Method that returns something... easy...
So why there wouldn't be functions with functions in it, they can even have functions in it... cool, eh?

```
// define a recursive iterative sum function
let sum = -> nums -> {
    let sumIter = -> nums, s -> {
      if first(nums) == nil: s
      else sumIter(rest(nums), first(nums)+s)
    }
    ret sumIter(nums, 0)
}

let resultA = sum(nums)
```
This is what coding should be... freaking fast and free

Ahhh, wait... we are also OOP with using Methods as well.. the 2000's calling :-)
```
let person = -> n -> {                          // person acts like a construtor returning a 'new' person
  let name = n                              // local variable 'name'
  let sayhi = -> -> println("hello, " + name)  // local function 'sayhi'
  ret -> -> person                             // return a closure of self. This is the new class
}

let p1 = person("ted")
let p2 = person("bob")

p1.name              // prints "ted"
p2.name = "kyle"    // change p2's name to "kyle"
p2.sayhi!          // prints "hello, kyle"

```

### Import of code

If it is nessecary to Import code, you can use
```
//import some_folder/some_file.o
```
This will import code from other osic-source files into your code. 
This code will proceed before the code from the importing source will run.


### First come is served first

OSIC handel the Code line by line. In this meaning you should create classes and methods before you use them.

```
// this will not work

let p1 = person("ted")
let p2 = person("bob")

let person = -> n -> { 
  let name = n
  let sayhi = -> -> println("hello, " + name) 'sayhi'
  ret -> -> person
}
```
This Code will end up with an error. You should better move the Class `person` to the top or create an own file for it.

Best practice would be:

File: `main.o`
```
let p1 = person("ted")
let p2 = person("bob")

p1.name
p2.name = "kyle"
p2.sayhi!  

```

File: `person.o`
```
let person = -> n -> { 
  let name = n
  let sayhi = -> -> println("hello, " + name) 'sayhi'
  ret -> -> person
}
```

When you run `main.o` the file `person.o` will be imported if it is in the same folder or in a subfolder and parsed before `main.o`.


# Keywords
There is a small set of keywords to keep the language simple as possible. Most functionality comes from expressions and symbols

|Command |Example  | Description|
--- | --- | ---
|let|`let a = "hello"`|defines a variable, method or class|
|if|`if a == "hello": sayhi!`|a if statement|
|while|`while a > 0: sayhi!` or `while p = pop(a): println(p)`|create a while loop|
|else|`if a == "hello": sayhi! else saybye!`|the else statement for if's|
|ret|`ret 4`|a return statement to return a value from a method| 

# Common types

|Type |Example  | Description|
--- | --- | ---
|true| `let b = true` |a boolean type of TRUE|
|false| `let b = false` | a boolean type of FALSE|
|nil| `if pop(a) == nil: "error"` or `if !pop(a): "error"`| a NULL |type|
|int|`let b = 1`|a 64bit integer|
|float|`let b = 1.0`|a 64bit float|
|bool|`let b = false` or `let b = true`|a boolean|
|string|`let b = "OSIC is nice"`|a string|
|error|`if pop(a) == nil: "error"`|a string that reflects an Error|
|array|`let a = [1,2,3,4]`|a Array of Common types|

# Examples

## Hello World 

```
println("Hello, world!")
```

## Hello with User Input

```
let name = readln!
println(name)
```

## More examples can be found in the examples folder
map and reduce functional examples

## Some language examples
### let statements
```
// let statements are the basis of creating variables

let name = "friendo"             // bind a string literal
let hobby = 'unit testing'      // bind another string using single quote
let age = 20                   // bind a int64 literal
let height = 187.3            // bind a float64 literal

let array = [name, hobby, age, height] // put them into an array

```

### operations on strings and arrays
```
let a = 'buddy'
a[0]  // 'b'
a[-1] // 'y'  arrays/strings wrap negatively around back to 0

let chant = [2,4,6,8]
chant = push(chant, "who do we appreciate?") // appends the string to the back of chant and returns the new array

// alternately you can use the '+' or  '+=' to concat arrays
chant += ["infix operators!"]
```

### Builtin functions
Builtin functions are functions that allowing to do common operations for Strings, Files etc. 

### basic array functions
```
let a = [1,2,3,4]
len(a)     // 4
first(a)   // 1
last(a)    // 4
rest(a)    // 2,3,4
lead(a)    // 1,2,3
push(a, 5) // 1,2,3,4,5
pop(a)     // 1,2,3,4
alloc(256, 'a') // creates an array of 256 a's.. can be any value
set(a, 0, 6) // a[0] = 6,2,3,4
```

### basic string functions
```
let s = "hello, friend"
split(s, '')     // splits s into an array of it's characters ['h', 'e', 'l', 'l' ... ]
split(s, ', ')  // splits by ', '. ['hello', 'world']
join(a, '')    // joins an array into a string of it's objects
join(a, '.')  // joins with a '.' in between each element
```

### Input and Output functions

With these builtins you can write to console
println(*object)
print(*object)

With these builtins you can read users input of the console
readln()
read()
readc()
readall()

### read arguments
`let args = args()`
args() return a array of string of all Arguments that where given.
This includes the interpreter, the filename and everything after.

### File operations
```
//read a textfile
let text = readfile("test.txt")

// create a file
createfile("newFile.txt")

// write some String to file
writefile(newFile, "Some Text")
```
### File bultin commands 
`readfile(*STRING)` read a file filename *STRING
`createfile(*STRING)` creates a file filename *STRING
`writefile(*STRING, *OBJECT)` write *Objects String representation a filename *STRING
`readdir(*STRING, *BOOLEAN)` reads all Filenames with Path from directory *STRING (*BOOLEAN is for recursive - non optional)  

### Conversion
```
atoi('a')  // 97
itoa(97)   // 'a'
```

### Functions
```
// functions are literals aswell (see the beginning of this Readme.md)
// functions are defined with the '->' arg1, arg2 '->' syntax
// return statement is 'ret'

// if a function only has one statement or expression it can directly follow the definition on the same line
let birthday = -> -> age += 1      // function with no args. increments age by 1

// if there is more than one line use braces starting on the same line
let shrink = -> n -> {
  ret height - n      // return statement
}

let grow = -> n -> {
  height + n          // return statement is optional if the last statement if the result to be returned
}

// usages

// when a function call has no args you can optionally use '!' syntax instead of '()'
birthday!   // optionally birthday()

let newHeight = grow(45)
newHeight = shrink(age) // assign newHeight to the result of shrink
```
### power operator
```
// power operator
4^3 // returns 4 to the power of 3
```
### If statements
```
// compact syntax
if true: "yes" else "no"

// full syntax
if true {
  ret "yes"
} else {
  ret "no"
}

// syntax can be mixed and matched
if true {
  if a == b: "yes" else {
    ret "no"
  }
} else "no"

// if else
if a == 1 {
  "one"
} else if a == 2 {
  "two"
} else "huge!"
```
### Closures
```
// define a function that takes one arg and returns a function that sums it's argument together

let newAdder = -> a -> {
  ret -> b -> {
    ret a + b
  }
}
// compact syntax of same definition let newAdder = -> a-> -> b -> a + b

let add2 = newAdder(2)

add2(4) // 6
```

### Classes
```
let person = -> n -> {                          // person acts like a construtor returning a 'new' person
  let name = n                              // local variable 'name'
  let sayhi = -> -> println("hello, " + name)  // local function 'sayhi'
  ret -> -> person                             // return a closure of self. This is the new class
}

let p1 = person("ted")
let p2 = person("bob")

p1.name              // prints "ted"
p2.name = "kyle"    // change p2's name to "kyle"
p2.sayhi!          // prints "hello, kyle"

```
### Help improving OSIC

# Add your Review (a little checklist)
- Make sure that methods of styling are similar to already existing related methods. Look at POJO's and helpers. This ensures that the code has a readable style.
- Do not leave TODOs: Marks or comments in code. The code is more readable if there are no unnecessary comments in the code.
- Describe clearly in the pullrequest what the code does and which problem is solved. New features can be submitted as issues.

# Code of Conduct

Everyone may feel invited to participate in this programming language.

However, there are a few small rules to follow:
- New code must always be reviewed by a code review and the pull request should explain exactly why the code should be changed.