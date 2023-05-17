# What is BFLang
BFLang is designed to be an improved, cleaner version of C.
BFLang currently compiles to only 64-bit NASM-compatible assembly for Linux (windows support in future update.)

## Comments

There are no comments **yet**

## Entrypoint

The entrypoint in BF is `_start`, so instead of C's `main` you have to use `_start`

## Constants

Constants/numbers can only be represented as decimal for now, no hexadecimal and no octal.

## Types

You have these default types:
- `int`, equivalent to C's `int32_t`
- `short`, equivalent to C's `int16_t`
- `byte`, equivalent to C's `int8_t`
No unsigned yet ;)

Here's an example on how you declare a variable with a default type:
`int:some_variable = 0;`

## Structs

There is also structs, which are, for example, defined like this:
```txt
PutYourStructNameHere {
    int:firstMember;
    short:secondMember;
    int:thirdMember;
}
```

Note that structs can only be defined in the top-level of your code.

After you declare a struct, you can use structs as if they are default types.

YOU CAN:
- You can copy a struct instance to another struct instance of the same type. For example:
`SomeStruct:x = y;`
- You can access a structs members **just like how you would in C.**
- You can have members of a struct have a struct type.

HOWEVER:
- Structs cannot be the return type of a function
- You cannot cast to a struct, and a struct cannot be casted

## Functions

First of all, here's an example on how to declare a function with a return type of `byte`, that takes in two `int` parameters:
```txt
byte:SomeFunction(int:x, int:y)
{
    
}
```

To return a value from a function, you can use `return(...)` (YOU **NEED** THE PARENTHESES) to return a value, like so:
```txt
int:AddOne(int:x)
{
    return(x + 1);
}
```

Also, functions can **only be defined in the top-level of your code**, just like structs.

### References

References in BF act just like how they would in C.
You can specify that you want a parameter to be passed by reference, using `&` like so:
```txt
int:AddOne(&int:toBePassedByReference)
{
    toBePassedByReference = toBePassedByReference + 1;
}
```

## Operators

Operators are used just like how they're used in C.

- All operators require that both operands have **matching types**.

- The order of operations is right-to-left, but you can use parentheses if necessary.

- You cannot use structs for any operator, except `=` to copy as shown earlier.

### List of all operators
- `=`
- `+`
- `-`
- `*`
- `/`
- `%`
- `<`
- `>`
- `==`
- `>>`
- `<<`

The definitions for them is the same as C.

### Conditionals

Conditional operators may only be used in the `if`, `while` or `for` statements.

## `if` Statement

The `if` statement is used like in C, but has a semicolon after its closing bracket. Example:
```txt
if (1 < 2)
{
    
};
```

## Loops

### `while` loop
The `while` loop is used like in C, but has a semicolon after its closing bracket. Example:
```txt
while (x > y)
{
    
};
```

### `for` loop

The `for` loop is used like in C, but has a semicolon after its closing bracket, and **the first statement is required.** Example:
```txt
for (int:x = 0;x < 100;x = x + 1)
{
    
};
```

## Casting

Casting **only works for integral default types** for now, until I implement floating-point types. 
Here's how to cast a short to an int for example, which should be self-explanatory:
```
byte:someByte = 120;
int:final = cast(int:someByte);
```

## I/O
So far, there is no input, but there is a handy `print` function which takes in a constant or a variable of type `byte`, `short` or `int`, and prints it as a single character.
Example:
`print(10);`
In this example, it would print a newline, since the decimal ASCII code for a newline is 10.

# Examples

## Check if number is prime
```txt
byte:IsPrime(int:x)
{
    for (int:i = 2;i < x;i = i + 1)
    {
        if ((x % i) == 0)
        {
            return(0);
        };
    };
    return(1);
}
```
## Draw a "number triangle"
```txt
_start()
{
    int:scale = 4;
    for (int:i = 1;i < scale + 1;i = i + 1)
    {
        for (int:j = 0;j < i;j = j + 1)
        {
            print(48 + j);
        };
        print(10);
    };
}
```
