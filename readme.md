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

References in BF act just like how they would in C++.
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
## Triangle rasterizer
```txt
vec2 {
    int:x;
    int:y;
}

int:vecSub(&vec2:z, vec2:w)
{
    z.x = z.x - w.x;
    z.y = z.y - w.y;
}

int:vecDot(vec2:x, vec2:y)
{
    int:mX = (x.x * y.x) / 256;
    int:mY = (x.y * y.y) / 256;
    return(mX + mY);
}

int:barycentric(vec2:p0, vec2:p1, vec2:p2, vec2:p, &int:u, &int:v, &int:w)
{
    vec2:v0 = p1;
    vec2:v1 = p2;
    vec2:v2 = p;
    vecSub(v0, p0);
    vecSub(v1, p0);
    vecSub(v2, p0);
    int:d00 = vecDot(v0, v0);
    int:d01 = vecDot(v0, v1);
    int:d11 = vecDot(v1, v1);
    int:d20 = vecDot(v2, v0);
    int:d21 = vecDot(v2, v1);
    int:denom = (((d00 * d11) / 256) - ((d01 * d01) / 256));
    u = (d11 * d20) / 256;
    u = u - ((d01 * d21) / 256);
    u = (u * 256) / denom;
    v = (d00 * d21) >> 8;
    v = v - ((d01 * d20) / 256);
    v = (v * 256) / denom;
    w = (((1 * 256) - u) - v);
}

int:_start()
{
    vec2:p0;
    vec2:p1;
    vec2:p2;
    vec2:p;
    p0.x = 4 << 8;
    p0.y = 4 << 8;
    p1.x = 16 << 8;
    p1.y = 4 << 8;
    p2.x = 6 << 8;
    p2.y = 16 << 8;
    for (int:i = 0;i < 20;i = i + 1)
    {
        for (int:j = 0;j < 20;j = j + 1)
        {
            int:u = 0;
            int:v = 0;
            int:w = 0;
            p.x = j << 8;
            p.y = i << 8;
            barycentric(p0, p1, p2, p, u, v, w);
            if (u > 0)
            {
                if (v > 0)
                {
                    if ((u + v) < (1 << 8))
                    {
                        print(49);
                    };
                    if ((u + v) > ((1 << 8) - 1))
                    {
                        print(48);
                    };
                };
                if (v < 1)
                {
                    print(48);
                };
            };
            if (u < 1)
            {
                print(48);
            };
        };
        print(10);
    };
}
```
## Spinning donut
```txt
int:cos(int:x)
{
    int:modX = x % (1 << 10);
    int:modX2 = x % (2 << 10);
    int:result = 4 * (modX - ((modX * modX) >> 10));
    if (modX2 > (1 << 10))
    {
        return(result * ~1);
    };
    return(result);
}

int:sin(int:x)
{
    x = x + (3 << 9);
    return(cos(x););
}

int:_start()
{
    int[1600]:framebuff;
    int[1600]:depthbuff;
    int:A = 0;
    int:B = 0;
    while (1 < 2)
    {
        for (int:__Count = 0;__Count < 1600;__Count = __Count + 1)
        {
            framebuff[__Count] = 0;
            depthbuff[__Count] = ~1000000;
        };
        for (int:theta = 0;theta < 2000;theta = theta + 4)
        {
            for (int:phi = 0;phi < 2000;phi = phi + 4)
            {
                
                int:nx1 = cos(phi);
                int:ny1 = sin(phi);
                int:y1 = (ny1 * 5) + (10 << 10);
                int:z1 = nx1 * 5;
                int:x1 = 0;

                int:rx1 = ((x1 * cos(theta)) >> 10) - ((y1 * sin(theta)) >> 10); 
                int:ry1 = ((x1 * sin(theta)) >> 10) + ((y1 * cos(theta)) >> 10); 
                ny1 = ((ny1 * cos(theta)) >> 10); 

                int:ry2 = ((z1 * sin(A)) >> 10) + ((ry1 * cos(A)) >> 10); 
                int:rz2 = ((z1 * cos(A)) >> 10) - ((ry1 * sin(A)) >> 10); 
                
                int:ry3 = ((rx1 * sin(B)) >> 10) + ((ry2 * cos(B)) >> 10); 
                int:rx3 = ((rx1 * cos(B)) >> 10) - ((ry2 * sin(B)) >> 10); 

                if (depthbuff[(20 + rx3 >> 10) + 40 * (20 + ry3 >> 10)] < rz2)
                {
                    depthbuff[(20 + rx3 >> 10) + 40 * (20 + ry3 >> 10)] = rz2;
                    if (ny1 < 0)
                    {
                        ny1 = 1 << 7;
                    };
                    framebuff[(20 + rx3 >> 10) + 40 * (20 + ry3 >> 10)] = ny1 >> 7;
                        
                };
            };
        };
        for (int:y = 0;y < 40;y = y + 1)
        {
            for (int:x = 0;x < 40;x = x + 1)
            {
                if (framebuff[x + y * 40] > 1)
                {
                    print(48 + framebuff[x + y * 40]);
                };
                if (framebuff[x + y * 40] == 0)
                {
                    print(32);
                };
                if (framebuff[x + y * 40] == 1)
                {
                    print(95);
                };
            };
            print(10);
        };
        A = A + 40;
        B = B + 16;
        print(10);
    };
}
```
