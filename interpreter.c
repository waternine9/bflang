#include <stddef.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
#include <algorithm>
#include <string>

typedef enum
{
    BFASSIGN,
    BFADD,
    BFSUB,
    BFMUL,
    BFDIV,
    BFFUNCCALL
} BFTokenType;

typedef struct {
    uint8_t* Name;
    size_t Len;
} BFName;

typedef struct {
    int Val;
    BFName Name;
} BFVariable;

typedef struct {
    int Val;
} BFConstant;

struct _BFToken;

typedef struct _BFScope {
    _BFToken** Lines;
    size_t LineCount;
    BFVariable** Vars; // First vars are the arguments...
    size_t ArgsCount;
    size_t VarsCount;
    _BFScope* Parent;
} BFScope;


typedef struct {
    BFScope RootScope;
    BFName Name;
} BFFunction;

typedef struct _BFToken {
    BFTokenType Type;
    BFVariable* Var;
    BFConstant Const;
    _BFToken** Params;
    size_t NumParams;
    _BFToken* First;
    _BFToken* Second;
    BFScope* NextScope;
    BFFunction* CallFunction;
    bool IsConstant;
    bool IsVar;
    int Val;
} BFToken;

typedef struct
{
    uint8_t* Code;
    size_t Size;
    size_t At;
    BFFunction** Functions;
    size_t BFNumFunctions;
} BFTokenizer;

size_t BFCodeSize(uint8_t* Code)
{
    size_t Size = 0;
    while (*Code++) Size++;
    return Size;
}

bool BFCodeStep(BFTokenizer* Tokenizer)
{
    if (Tokenizer->At >= Tokenizer->Size) return false;
    return true;
}

size_t BFTellNext(BFTokenizer* Tokenizer, uint8_t Char)
{
    int I = Tokenizer->At;
    while (I < Tokenizer->Size)
    {
        if (Tokenizer->Code[I] == Char) return I;

        I++;
    }
    return 0xFFFFFFFF;
}

size_t BFTellNextMatching(BFTokenizer* Tokenizer, uint8_t CharDec, uint8_t CharInc)
{
    size_t Counter = 0;
    int I = Tokenizer->At;
    while (I < Tokenizer->Size)
    {
        if (Tokenizer->Code[I] == CharInc) Counter++;
        if (Tokenizer->Code[I] == CharDec)
        {
            if (Counter == 1) return I;
            Counter--;
        }

        I++;
    }
    return 0xFFFFFFFF;
}
bool BFCmpName(BFName X, BFName Y)
{
    if (X.Len != Y.Len) return false;
    for (int I = 0; I < X.Len; I++)
    {
        if (X.Name[I] != Y.Name[I]) return false;
    }
    return true;
}

size_t BFFindNextOperator(BFTokenizer* Tokenizer)
{
    for (size_t I = Tokenizer->At; I < Tokenizer->Size; I++)
    {
        if (Tokenizer->Code[I] == '+'
            || Tokenizer->Code[I] == '-'
            || Tokenizer->Code[I] == '*'
            || Tokenizer->Code[I] == '/'
            || Tokenizer->Code[I] == '=')
        {
            return I;
        }
    }
    return 0xFFFFFFFF;
}

size_t BFTellNextCom(BFTokenizer* Tokenizer)
{
    int Counter = 0;
    for (size_t I = Tokenizer->At; I < Tokenizer->Size; I++)
    {
        if (Tokenizer->Code[I] == '[') Counter++;
        if (Tokenizer->Code[I] == ']') Counter--;
        if (Tokenizer->Code[I] == ',')
        {
            if (Counter == 0) return I;
        }
    }
    return 0xFFFFFFFF;
}

bool BFFindVariable(BFName SearchFor, BFScope* SearchIn, BFVariable** OutPtr)
{

    for (int I = 0; I < SearchIn->VarsCount; I++)
    {
        if (BFCmpName(SearchFor, SearchIn->Vars[I]->Name))
        {
            *OutPtr = SearchIn->Vars[I];
            return true;
        }
    }
    if (SearchIn->Parent == 0) return false;
    return BFFindVariable(SearchFor, SearchIn->Parent, OutPtr);
}



BFToken* BFTokenizeExpr(BFTokenizer* Tokenizer, BFScope* CurrentScope);

BFToken* BFTokenizeFuncCall(BFTokenizer* Tokenizer, BFScope* CurrentScope)
{
    BFToken* Token = (BFToken*)malloc(sizeof(BFToken));
    memset(Token, 0, sizeof(BFToken));
    size_t NextSqrBr = BFTellNext(Tokenizer, '[');
    size_t NextMatchingSqrBr = BFTellNextMatching(Tokenizer, ']', '[');
    Token->Type = BFFUNCCALL;
    BFName FuncName;
    FuncName.Len = NextSqrBr - Tokenizer->At;
    FuncName.Name = Tokenizer->Code + Tokenizer->At;

    for (int I = 0; I < Tokenizer->BFNumFunctions; I++)
    {
        if (BFCmpName(FuncName, Tokenizer->Functions[I]->Name))
        {
            Token->CallFunction = Tokenizer->Functions[I];
            break;
        }
    }
    if (!Token->CallFunction) return NULL;

    Token->Params = (BFToken**)malloc(sizeof(BFToken*));

    
    Tokenizer->At = NextSqrBr + 1;
    size_t NextCom = BFTellNextCom(Tokenizer);
    if (NextSqrBr + 1 == NextMatchingSqrBr) return Token;
    bool BreakOut = false;
    while (1)
    {
        BFToken* Param = BFTokenizeExpr(Tokenizer, CurrentScope);

        Token->Params[Token->NumParams++] = Param;
        BFToken** NewParams = (BFToken**)malloc((Token->NumParams + 1) * sizeof(BFToken*));
        memcpy(NewParams, Token->Params, Token->NumParams * sizeof(BFToken*));
        free(Token->Params);
        Token->Params = NewParams;

        Tokenizer->At = NextCom + 1;

        if (NextCom == 0xFFFFFFFF)
        {
            std::cout << "PARAMS" << Token->NumParams << "\n";
            return Token;
        }
        else
        {
            NextCom = BFTellNextCom(Tokenizer);
        }
    }
    return Token;

}

BFTokenType BFGetOpType(uint8_t C)
{
    switch (C)
    {
        case '+':
            return BFADD;
        case '-':
            return BFSUB;
        case '*':
            return BFMUL;
        case '/':
            return BFDIV;
        case '=':
            return BFASSIGN;
        default:
            return BFADD;
    }
}

BFToken* BFTokenizeExpr(BFTokenizer* Tokenizer, BFScope* CurrentScope)
{
    
    BFToken* Tok = (BFToken*)malloc(sizeof(BFToken));
    memset(Tok, 0, sizeof(BFToken));

    if (Tokenizer->Code[Tokenizer->At] == '(')
    {
        
        size_t NextMatching = BFTellNextMatching(Tokenizer, ')', '(');
        
        if (NextMatching == 0xFFFFFFFF) 
        {
            return NULL;
        }
        Tokenizer->At++;
        BFToken* NextToken = BFTokenizeExpr(Tokenizer, CurrentScope);
        Tok->First = NextToken;
        Tokenizer->At = NextMatching + 1;
        Tok->Type = BFGetOpType(Tokenizer->Code[Tokenizer->At]);
        Tokenizer->At++;

        Tok->Second = BFTokenizeExpr(Tokenizer, CurrentScope);

        return Tok;
    }

    size_t NextOp = BFFindNextOperator(Tokenizer);
    size_t NextSemi = BFTellNext(Tokenizer, ';');
    size_t NextBr = BFTellNext(Tokenizer, ')');
    size_t NextRSqrBr = BFTellNext(Tokenizer, ']');
    size_t NextCom = BFTellNext(Tokenizer, ',');
    if (NextOp < NextBr) NextBr = NextOp;
    if (NextSemi < NextBr) NextBr = NextSemi;
    if (NextRSqrBr < NextBr) NextBr = NextRSqrBr;
    if (NextCom < NextBr) NextBr = NextCom;

    size_t NextSqrBr = BFTellNext(Tokenizer, '[');

    if (NextSqrBr < NextBr && NextRSqrBr != 0xFFFFFFFF)
    {
        if (NextOp != 0xFFFFFFFF) Tokenizer->At = NextOp + 1;
        
        
        BFToken* MyTok = BFTokenizeFuncCall(Tokenizer, CurrentScope);
        
        if (NextRSqrBr == NextCom - 1 || NextRSqrBr == NextSemi - 1) return MyTok;

        BFToken* SurroundToken = (BFToken*)malloc(sizeof(BFToken));
        memset(SurroundToken, 0, sizeof(BFToken));
        SurroundToken->First = MyTok;
        Tokenizer->At = NextRSqrBr + 1;
        SurroundToken->Second = BFTokenizeExpr(Tokenizer, CurrentScope);
        return SurroundToken;        
    }

        
    if ('0' <= Tokenizer->Code[Tokenizer->At] && Tokenizer->Code[Tokenizer->At] <= '9')
    {
        int Multiplier = 1;
        int Val = 0;
        for (int I = NextBr - 1;I >= Tokenizer->At;I--)
        {
            int C = Tokenizer->Code[I];
            if (C < '0' || C > '9') return 0;
            Val += Multiplier * (C - '0');
            Multiplier *= 10;
        }
        Tok->IsConstant = true;
        Tok->Const.Val = Val;
    }
    else
    {
        BFVariable* OutPtr;
        BFName VarName;
        VarName.Name = Tokenizer->Code + Tokenizer->At;
        VarName.Len = NextBr - Tokenizer->At;
        
        if (BFFindVariable(VarName, CurrentScope, &OutPtr))
        {
            Tok->IsVar = true;
            Tok->Var = OutPtr;
        }
        else
        {
            BFVariable* NewVar = (BFVariable*)malloc(sizeof(BFVariable));
            NewVar->Name = VarName;
            NewVar->Val = 0;
            CurrentScope->Vars[CurrentScope->VarsCount++] = NewVar;
            BFVariable** NewVars = (BFVariable**)malloc(sizeof(BFVariable*) * (CurrentScope->VarsCount + 1));
            memcpy(NewVars, CurrentScope->Vars, sizeof(BFVariable*) * CurrentScope->VarsCount);
            free(CurrentScope->Vars);
            CurrentScope->Vars = NewVars;

            Tok->IsVar = true;
            Tok->Var = NewVar;
        }
    }
    

    if (NextOp > NextBr)
    {
        std::cout << Tok->Const.Val << std::endl;
        return Tok;
    }

    BFToken* SurroundTok = (BFToken*)malloc(sizeof(BFToken));
    SurroundTok->First = Tok;

    SurroundTok->Type = BFGetOpType(Tokenizer->Code[NextOp]);

    Tokenizer->At = NextBr + 1;

    SurroundTok->Second = BFTokenizeExpr(Tokenizer, CurrentScope);
    return SurroundTok;
}

BFToken* BFTokenizeLine(BFTokenizer* Tokenizer, BFScope* CurrentScope)
{

    return BFTokenizeExpr(Tokenizer, CurrentScope);
}



void BFTokenizeFunction(BFTokenizer* Tokenizer)
{
    size_t NextSqrBr = BFTellNext(Tokenizer, '[');
    if (NextSqrBr == 0xFFFFFFFF) return;
    BFFunction* CurrentFunction = (BFFunction*)malloc(sizeof(BFFunction));
    CurrentFunction->Name.Name = Tokenizer->Code + Tokenizer->At;
    CurrentFunction->Name.Len = NextSqrBr - Tokenizer->At;
    CurrentFunction->RootScope.Lines = (BFToken**)malloc(sizeof(BFToken*));
    CurrentFunction->RootScope.LineCount = 0;
    CurrentFunction->RootScope.Parent = 0;
    Tokenizer->At = NextSqrBr + 1;
    size_t NextSqrBr2 = BFTellNext(Tokenizer, ']');
    if (NextSqrBr2 == 0xFFFFFFFF) return;


    // Parse Arguments

    CurrentFunction->RootScope.Vars = (BFVariable**)malloc(sizeof(BFVariable*));
    CurrentFunction->RootScope.ArgsCount = 0;

    if (NextSqrBr + 1 != NextSqrBr2)
    {
        while (1)
        {
            uint8_t NextCom = BFTellNext(Tokenizer, ',');
            bool BreakOut = NextCom > NextSqrBr2 || NextCom == 0xFFFFFFFF;
            if (BreakOut)
            {
                NextCom = NextSqrBr2;
            }
            BFVariable *Var = (BFVariable*)malloc(sizeof(BFVariable));
            Var->Name.Name = Tokenizer->Code + Tokenizer->At;
            Var->Name.Len = NextCom - Tokenizer->At;
            Var->Val = 0;
            CurrentFunction->RootScope.Vars[CurrentFunction->RootScope.ArgsCount++] = Var;
            BFVariable** NewArgs = (BFVariable**)malloc(sizeof(BFVariable*) * (CurrentFunction->RootScope.ArgsCount + 1));
            memcpy(NewArgs, CurrentFunction->RootScope.Vars, sizeof(BFVariable*) * CurrentFunction->RootScope.ArgsCount);
            free(CurrentFunction->RootScope.Vars);
            CurrentFunction->RootScope.Vars = NewArgs;
            Tokenizer->At = NextCom + 1;
            if (BreakOut) break;

        }
    }

    CurrentFunction->RootScope.VarsCount = CurrentFunction->RootScope.ArgsCount;

    Tokenizer->At = NextSqrBr2 + 1;
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) return;

    Tokenizer->At++;

    
    while (Tokenizer->At < NextMatchingBr)
    {
        size_t NextSemi = BFTellNext(Tokenizer, ';');
        if (NextSemi == 0xFFFFFFFF) return;
        CurrentFunction->RootScope.Lines[CurrentFunction->RootScope.LineCount++] = BFTokenizeLine(Tokenizer, &CurrentFunction->RootScope);
        BFToken** NewLines = (BFToken**)malloc(sizeof(BFToken*) * (CurrentFunction->RootScope.LineCount + 1));
        memcpy(NewLines, CurrentFunction->RootScope.Lines, sizeof(BFToken*) * CurrentFunction->RootScope.LineCount);
        free(CurrentFunction->RootScope.Lines);
        CurrentFunction->RootScope.Lines = NewLines;

        Tokenizer->At = NextSemi + 1;
    }
    Tokenizer->At = NextMatchingBr + 1;
    Tokenizer->Functions[Tokenizer->BFNumFunctions++] = CurrentFunction;
    BFFunction** NewFuncs = (BFFunction**)malloc(sizeof(BFFunction*) * (Tokenizer->BFNumFunctions + 1));
    memset(NewFuncs, 0, sizeof(BFFunction*) * (Tokenizer->BFNumFunctions + 1));
    memcpy(NewFuncs, Tokenizer->Functions, sizeof(BFFunction*) * Tokenizer->BFNumFunctions);
    free(Tokenizer->Functions);
    Tokenizer->Functions = NewFuncs;
}

// Here we go!
BFFunction** BFTokenize(uint8_t* Code, size_t Len)
{
    BFTokenizer Tokenizer;
    Tokenizer.At = 0;
    Tokenizer.Code = Code;
    Tokenizer.Size = Len;
    Tokenizer.Functions = (BFFunction**)malloc(sizeof(BFFunction*));

    Tokenizer.BFNumFunctions = 0;
    while (BFCodeStep(&Tokenizer))
    {
        BFTokenizeFunction(&Tokenizer);
    }

    return Tokenizer.Functions;
}

int BFExecuteFunc(BFFunction* Func);

int BFExecuteToken(BFToken* Tok)
{
    if (Tok->IsConstant) return Tok->Const.Val;
    if (Tok->IsVar) 
    {
        return Tok->Var->Val;
    }
    switch (Tok->Type)
    {
    case BFASSIGN:
        Tok->First->Var->Val = BFExecuteToken(Tok->Second);
        
        return Tok->First->Var->Val;
    case BFADD:
        return BFExecuteToken(Tok->First) + BFExecuteToken(Tok->Second);
    case BFSUB:
        return BFExecuteToken(Tok->First) - BFExecuteToken(Tok->Second);
    case BFMUL:
        return BFExecuteToken(Tok->First) * BFExecuteToken(Tok->Second);
    case BFDIV:
        return BFExecuteToken(Tok->First) / BFExecuteToken(Tok->Second);
    case BFFUNCCALL:
        for (int I = 0; I < Tok->NumParams; I++)
        {
            Tok->CallFunction->RootScope.Vars[I]->Val = BFExecuteToken(Tok->Params[I]);
        }
        return BFExecuteFunc(Tok->CallFunction);
    }
}

int BFExecuteFunc(BFFunction* Func)
{
    for (int I = 0; I < Func->RootScope.LineCount; I++)
    {
        int R = BFExecuteToken(Func->RootScope.Lines[I]);

        if (I == Func->RootScope.LineCount - 1)
        {
            return R;
        }
    }
}

int BFRun(BFFunction** Funcs)
{
    BFName MainName;
    MainName.Len = 4;
    MainName.Name = (uint8_t*)malloc(MainName.Len);
    const char* SMainName = "Main";
    memcpy(MainName.Name, SMainName, 4);
    while (*Funcs)
    {
        if (BFCmpName((*Funcs)->Name, MainName))
        {
            std::cout << "Found Main, executing..." << std::endl;
            std::cout << "Program returned with exit code " << BFExecuteFunc(*Funcs) << std::endl;
        }
        Funcs++;
    }
    return 0;
}
