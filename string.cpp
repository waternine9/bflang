#include "string.hpp"
#include <cstdarg>

String StrFromCStr(const char *str)
{
    String out = String();
    while (*str)
    {
        out.PushBack(*str);
        str++;
    }
    return out;
}

String StrFromArray(char *str, size_t size)
{
    String out = String();

    for (int i = 0; i < size && str[i]; i++)
    {
        out.PushBack(str[i]);
    }

    return out;
}

String StrAppend(String x, String y)
{
    String out;
    for (int i = 0; i < x.size; i++)
    {
        out.PushBack(x[i]);
    }
    for (int i = 0; i < y.size; i++)
    {
        out.PushBack(y[i]);
    }
    return out;
}

bool StrStartsWith(String str, String startswith)
{
    if (startswith.size > str.size)
        return false;
    for (int i = 0; i < str.size; i++)
    {
        if (str[i] == ' ')
            return true;
        if (str[i] != startswith[i])
            return false;
    }
    return true;
}

bool StrEqualsWith(String x, String y)
{
    if (x.size != y.size)
        return false;
    for (int i = 0; i < x.size; i++)
    {
        if (x[i] != y[i])
            return false;
    }
    return true;
}

typedef struct
{
    String *dest;
} destination;

typedef struct
{
    size_t Number;
    size_t Sign;
} num;

const char *BASE_CHARS = "0123456789ABCDEF";

static num Int2Num(int i)
{
    return (num){(i < 0) ? -i : i, i < 0};
}
static num Uint2Num(unsigned int i)
{
    return (num){i, 0};
}
static void WriteDestination(destination *Dest, char C)
{
    Dest->dest->PushBack(C);
}
static void WriteDestinationStr(destination *Dest, const char *S)
{
    while (*S)
    {
        WriteDestination(Dest, *S++);
    }
}
static void WriteDestinationStrN(destination *Dest, const char *S, int N)
{
    for (int I = 0; S[I] && I < N; I++)
    {
        WriteDestination(Dest, S[I]);
    }
}
static int NumWidthBaseN(size_t Value, size_t Base)
{
    if (Base == 1)
        return 0;
    if (Base > 16)
        return 0;

    size_t End = 0;
    do
    {
        Value /= Base;
        End += 1;
    } while (Value);

    return End;
}
static void WriteDestinationBaseN(destination *Dest, size_t Value, size_t Base)
{
    if (Base == 1)
        return;
    if (Base > 16)
        return;

    size_t End = NumWidthBaseN(Value, Base);

    char Buf[64] = {0};
    do
    {
        size_t Index = Value % Base;
        Value /= Base;

        Buf[--End] = BASE_CHARS[Index];
    } while (Value);

    WriteDestinationStr(Dest, Buf);
}
static int IsDigit(int c)
{
    return c >= '0' && c <= '9';
}
static int DigitValue(int c)
{
    return c - '0';
}
static void Number(destination *Dest, int Width, int ZeroPad, num Num, int Base)
{
    if (Num.Sign)
    {
        WriteDestination(Dest, '-');
    }

    int L = NumWidthBaseN(Num.Number, Base);
    for (; L < Width; L++)
    {
        if (ZeroPad)
            WriteDestination(Dest, '0');
        else
            WriteDestination(Dest, ' ');
    }

    WriteDestinationBaseN(Dest, Num.Number, Base);
}
static void Floating(destination *Dest, int Width, int ZeroPad, double Num)
{
    if (Num < 0)
    {
        Num *= -1;
        WriteDestination(Dest, '-');
    }

    int L = NumWidthBaseN(Num, 10);
    for (; L < Width; L++)
    {
        if (ZeroPad)
            WriteDestination(Dest, '0');
        else
            WriteDestination(Dest, ' ');
    }

    WriteDestinationBaseN(Dest, Num, 10);
    WriteDestination(Dest, '.');
    WriteDestinationBaseN(Dest, (Num - (size_t(Num))) * 1000000, 10);
}

size_t FormatCStringLength(const char* Str)
{
    size_t Size = 0;
    while (*Str)
    {
        Size++;
        Str++;
    }
    return Size;
}

static void _String(destination *Dest, int Width, int ZeroPad, int Precision, const char *Str)
{
    // TODO: This can be optimized if we just fetch first Width characters.
    size_t L = FormatCStringLength(Str);
    if (Precision < 0)
        Precision = 0;
    if (Precision != 0 && L > Precision)
        L = Precision;

    for (; L < Width; L++)
    {
        if (ZeroPad)
            WriteDestination(Dest, '0');
        else
            WriteDestination(Dest, ' ');
    }

    if (Precision == 0)
    {
        WriteDestinationStr(Dest, Str);
    }
    else
    {
        WriteDestinationStrN(Dest, Str, Precision);
    }
}

void FormatWriteStringVa(String *dest, const char *Fmt, va_list Va)
{

    destination Buffer = {dest};
    int Width = 0;
    int ZeroPad = 0;
    int Precision = 0;

    for (int I = 0; Fmt[I]; I++)
    {
        Width = 0;
        switch (Fmt[I])
        {
        case '%':
            I++;
            if (Fmt[I] == '0')
            {
                ZeroPad = 1;
                I++;
            }
            while (IsDigit(Fmt[I]))
            {
                Width += DigitValue(Fmt[I++]);
            }
            if (Fmt[I] == '.' && Fmt[I + 1] == '*')
            {
                Precision = va_arg(Va, int);
                I += 2;
            }
            switch (Fmt[I])
            {
            case 'c':
                WriteDestination(&Buffer, va_arg(Va, int));
                break;
            case 'f':
                Floating(&Buffer, Width, ZeroPad, va_arg(Va, double));
                break;
            case 's':
                _String(&Buffer, Width, ZeroPad, Precision, va_arg(Va, const char *));
                break;
            case 'd':
                Number(&Buffer, Width, ZeroPad, Int2Num(va_arg(Va, int)), 10);
                break;
            case 'u':
                Number(&Buffer, Width, ZeroPad, Uint2Num(va_arg(Va, unsigned int)), 10);
                break;
            case 'x':
                Number(&Buffer, Width, ZeroPad, Uint2Num(va_arg(Va, unsigned int)), 16);
                break;
            default:
            case '%':
                WriteDestination(&Buffer, '%');
                break;
            }
            break;
        default:
            WriteDestination(&Buffer, Fmt[I]);
            break;
        }
    }

    WriteDestination(&Buffer, '\0');
}

String StrFormat(const char *format, ...)
{
    String out;
    va_list Args;
    va_start(Args, format);
    FormatWriteStringVa(&out, format, Args);
    va_end(Args);
    out.PopBack();
    return out;
}

bool CStrEqualsWith(char* x, size_t xSize, char* y, size_t ySize)
{
    if (xSize != ySize) return false;
    while (xSize--)
    {
        if (x[xSize] != y[ySize]) return false;
    }
    return true;
}

bool StrHas(String str, char what)
{
    for (int I = 0;I < str.size;I++)
    {
        if (str[I] == what) return true;
    }
    return false;
}