# OSIC Language

## Keywords
`call` (only internal compiler - calls a method. For normal code just write the methodname eg. `test()` )

`print`

`input`

`public`

`class`

`if`

`fopen`

`fwrite`

`return`

`end`

## types
Actually we got two now types

`int` integer presentaion

`char` representation for a character (can used for character as well `char d="ello";` - but this is an issue!) 

## Controll Structures

### IF
Single "if" produce an direct action. It can be an return or an further complete operation, or even a method call.
```java
public main(){
    square()
}

public square(){
    int X = 1;
    print X * X;
    int X = X + 1;
    if X > 10 THEN return;
    square();
}
```

## method calls
Method can called by its name.
```java
public main(){
    test();
}

public test(){
    print "hello world";
}
```

## Output

## fopen (String filename)
Open or create a file for writing
```sh
fopen ("/home/user/test.txt");
```

## fwrite (Object)
Store and close a array of characters
```sh
fwrite ("this is a text to store");
```
Also used for operative output
```sh
int x = 1;
int r = 3;
fwrite (x+x*r);
```

## print (Object)
Presentation of a variable or string.
Concatination or operations must be enclosed!
```sh
char h="h";
print "hello";
print (h+"ello");
```

## input (String)
Read a console input.
Actually it is a typeless representation.
```sh
print "whats your Name";
input x;
print ("hello " + x); 
```
