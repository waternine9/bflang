#include "bf.hpp"
#include "mem.hpp"
#include <fstream>
#include <strstream>

size_t BFCodeSize(uint8_t* Code)
{
    size_t Size = 0;
    while (*Code++) Size++;
    return Size;
}

bool BFCodeStep(bf_tokenizer* Tokenizer)
{
    if (Tokenizer->At >= Tokenizer->Size) return false;
    return true;
}

size_t BFTellNext(bf_tokenizer* Tokenizer, uint8_t Char)
{
    int I = Tokenizer->At;
    while (I < Tokenizer->Size)
    {
        if (Tokenizer->Code[I] == Char) return I;

        I++;
    }
    return 0xFFFFFFFF;
}

size_t BFTellNextMatching(bf_tokenizer* Tokenizer, uint8_t CharDec, uint8_t CharInc)
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

size_t BFTellNextSemi(bf_tokenizer* Tokenizer)
{
    size_t Counter = 0;
    int I = Tokenizer->At;
    while (I < Tokenizer->Size)
    {
        if (Tokenizer->Code[I] == '{') Counter++;
        if (Tokenizer->Code[I] == '}') Counter--;
        if (Tokenizer->Code[I] == '(') Counter++;
        if (Tokenizer->Code[I] == ')') Counter--;
        if (Tokenizer->Code[I] == ';')
        {
            if (Counter == 0) return I;
        }

        I++;
    }
    return 0xFFFFFFFF;
}

bool BFCmpName(bf_name X, bf_name Y)
{
    if (X.Len != Y.Len) return false;
    for (int I = 0; I < X.Len; I++)
    {
        if (X.Name[I] == '.') return true;
        if (Y.Name[I] == '.') return true;
        if (X.Name[I] != Y.Name[I]) return false;
    }
    return true;
}

bool BFIsOpChar(uint8_t C)
{
    switch (C)
    {
        case '+':
            return true;
        case '<':
            return true;
        case '>':
            return true;
        case '-':
            return true;
        case '*':
            return true;
        case '/':
            return true;
        case '=':
            return true;
        case '%':
            return true;
        default:
            return false;
    }
}

bf_token_type BFTokenizeOpType(bf_tokenizer *Tokenizer)
{
    String S = String();
    while (Tokenizer->At < Tokenizer->Size)
    {
        if (!BFIsOpChar(Tokenizer->Code[Tokenizer->At])) break;
        S.PushBack(Tokenizer->Code[Tokenizer->At]);
        Tokenizer->At++;
    }
    if (S.size == 1)
    {
        uint8_t C = S[0];
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
            case '<':
                return BFLESSTHAN;
            case '>':
                return BFMORETHAN;
            case '%':
                return BFMODULO;
            default:
                break;
        }
        return BFUNKNOWN;
    }
    if (StrEqualsWith(S, StrFromCStr("=="))) return BFEQUALS;
    else if (StrEqualsWith(S, StrFromCStr(">>"))) return BFSHR;
    else if (StrEqualsWith(S, StrFromCStr("<<"))) return BFSHL;
    return BFUNKNOWN;
}

size_t BFFindNextOperator(bf_tokenizer* Tokenizer)
{
    size_t OldTokAt = Tokenizer->At;
    for (size_t I = Tokenizer->At; I < Tokenizer->Size; I++)
    {
        if (BFIsOpChar(Tokenizer->Code[I]))
        {
            return I;
        }
        /*Tokenizer->At = I;
        if (Tokenizer->Code[I] == ';') return 0xFFFFFFFF;
        if (BFTokenizeOpType(Tokenizer) != BFUNKNOWN)
        {
            Tokenizer->At = OldTokAt;
            return I;
        }*/
    }
    return 0xFFFFFFFF;
}

size_t BFTellNextCom(bf_tokenizer* Tokenizer)
{
    int Counter = 0;
    for (size_t I = Tokenizer->At; I < Tokenizer->Size; I++)
    {
        if (Tokenizer->Code[I] == '(') Counter++;
        if (Tokenizer->Code[I] == ')') Counter--;
        if (Tokenizer->Code[I] == ',')
        {
            if (Counter == 0) return I;
        }
    }
    return 0xFFFFFFFF;
}

bool BFFindVariable(bf_name SearchFor, bf_scope* SearchIn, bf_variable** OutPtr)
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



bf_token* BFTokenizeExpr(bf_tokenizer* Tokenizer, bf_scope* CurrentScope);

bool BFGetStdFunction(bf_name x, bf_token_type &out)
{
    if (CStrEqualsWith((char*)x.Name, x.Len, "__print", 7))
    {
        out = BFPRINT;
        return true;
    }
    return false;
}

bf_token* BFTokenizeFuncCall(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    size_t NextSqrBr = BFTellNext(Tokenizer, '(');
    size_t NextMatchingSqrBr = BFTellNextMatching(Tokenizer, ')', '(');
    Token->Type = BFFUNCCALL;
    bf_name FuncName;
    
    FuncName.Len = NextSqrBr - Tokenizer->At;
    FuncName.Name = Tokenizer->Code + Tokenizer->At;

    bf_token_type tempType;
    if (BFGetStdFunction(FuncName, tempType))
    {
        Token->Type = tempType;
    }

    for (int I = 0; I < Tokenizer->BFNumFunctions; I++)
    {
        if (BFCmpName(FuncName, Tokenizer->Functions[I]->Name))
        {
            Token->CallFunction = Tokenizer->Functions[I];
            break;
        }
    }
    if (!Token->CallFunction) return NULL;

    Token->Params = (bf_token**)kmalloc(sizeof(bf_token*));

    
    Tokenizer->At = NextSqrBr + 1;

    size_t NextCom = BFTellNextCom(Tokenizer);

    if (NextSqrBr + 1 == NextMatchingSqrBr) return Token;

    bool BreakOut = false;

    while (1)
    {
        bf_token* Param = BFTokenizeExpr(Tokenizer, CurrentScope);

        Token->Params[Token->NumParams++] = Param;
        bf_token** NewParams = (bf_token**)kmalloc((Token->NumParams + 1) * sizeof(bf_token*));
        kmemcpy(NewParams, Token->Params, Token->NumParams * sizeof(bf_token*));
        kfree(Token->Params);
        Token->Params = NewParams;

        
        if (NextCom == 0xFFFFFFFF)
        {
            return Token;
        }
        else
        {
            Tokenizer->At = NextCom + 1;
            NextCom = BFTellNextCom(Tokenizer);
        }
    }
    return Token;

}



LinkedList<bf_type> Types;

bool BFTokenizeType(bf_tokenizer *Tokenizer, bf_type &OutType)
{
    String S;
    size_t OldTokAt = Tokenizer->At;
    bool IsFound = false;
    while (Tokenizer->At < Tokenizer->Size)
    {
        if (Tokenizer->Code[Tokenizer->At] == ':')
        {
            Tokenizer->At++;
            IsFound = true;
            break;
        }
        else if (Tokenizer->Code[Tokenizer->At] == ';' || Tokenizer->Code[Tokenizer->At] == ')' || Tokenizer->Code[Tokenizer->At] == '(' || BFIsOpChar(Tokenizer->Code[Tokenizer->At]))
        {
            Tokenizer->At++;
            IsFound = false;
            break;
        }
        S.PushBack(Tokenizer->Code[Tokenizer->At]);
        Tokenizer->At++;
    }
    if (IsFound)
    {
        for (int I = 0;I < Types.size;I++)
        {
            if (StrEqualsWith(Types[I].Name, S))
            {
                OutType = Types[I];
                return true;
            }
        }
    }
    else
    {
        Tokenizer->At = OldTokAt;
    }
    return false;
}

bf_token* BFTokenizeMemberAccess(bf_tokenizer* Tokenizer, bf_variable* CurVariable, bf_type CurType, size_t StopAt)
{
    while (Tokenizer->Code[Tokenizer->At] != '.')
    {
        if (Tokenizer->At == StopAt) return 0;
        Tokenizer->At++;
    }
    Tokenizer->At++;
    String Accum = String();
    while (Tokenizer->Code[Tokenizer->At] != '.')
    {
        if (Tokenizer->At == StopAt) break;
        Accum.PushBack(Tokenizer->Code[Tokenizer->At]);
        Tokenizer->At++;
    }
    if (Accum.size != 0)
    {
        bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
        kmemset(Token, 0, sizeof(Token));
        Token->Type = BFMEMBERACCESS;
        for (int I = 0;I < CurType.StructMembers.size;I++)
        {
            if (StrEqualsWith(Accum, CurType.StructMembers[I].Name))
            {
                Token->Member = &CurType.StructMembers[I];
                break;
            }
        }
        bf_token* Next = BFTokenizeMemberAccess(Tokenizer, CurVariable, Token->Member->Type, StopAt);
        if (Next == 0)
        {
            Token->IsFinalMember = true;
            return Token;
        }
        Token->First = Next;
        Token->IsFinalMember = false;
        return Token;
    }
    return 0;
}

bf_token* BFTokenizeIf(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    size_t NextBr = BFTellNext(Tokenizer, '(');
    if (NextBr == 0xFFFFFFFF) 
    {
        printf("ERR: if: Expected (\n");
        return 0;
    }
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) 
    {
        printf("ERR: if: Expected matching )\n");
        return 0;
    }
    Tokenizer->At = NextBr + 1;
    bf_token* Conditional = BFTokenizeExpr(Tokenizer, CurrentScope);
    if (Conditional == 0) 
    {
        printf("ERR: if: Conditional failed to parse!\n");
        return 0;
    }
    Tokenizer->At = NextMatchingBr + 1;
    size_t NextMatchingSqrBr = BFTellNextMatching(Tokenizer, '}', '{');
    if (NextMatchingSqrBr == 0xFFFFFFFF) 
    {
        printf("ERR: if: Expected matching }\n");
        return 0;
    }
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    Token->Type = BFIF;
    Token->First = Conditional;
    bf_scope* IfScope = (bf_scope*)kmalloc(sizeof(bf_scope));
    IfScope->Lines = LinkedList<bf_token*>();
    IfScope->Parent = CurrentScope;
    IfScope->Vars = (bf_variable**)kmalloc(sizeof(bf_variable*));
    IfScope->VarsCount = 0;
    IfScope->ArgsCount = 0;
    
    Token->NextScope = IfScope;

    Tokenizer->At++;
    
    while (Tokenizer->At < NextMatchingSqrBr)
    {
        size_t NextSemi = BFTellNextSemi(Tokenizer);
        bf_token* NewTok = BFTokenizeExpr(Tokenizer, IfScope);
        
        if (NewTok == 0) 
        {
            printf("ERR: if: Line failed to parse!\n");
            return 0;
        }
        
        IfScope->Lines.PushBack(NewTok);
        Tokenizer->At = NextSemi + 1;
    }
    return Token;
}
bf_token* BFTokenizeCast(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    size_t NextBr = BFTellNext(Tokenizer, '(');
    if (NextBr == 0xFFFFFFFF) 
    {
        printf("ERR: cast: Expected (\n");
        return 0;
    }
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) 
    {
        printf("ERR: cast: Expected matching )\n");
        return 0;
    }
    Tokenizer->At = NextBr + 1;
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    Token->Type = BFCAST;
    bf_type OutType;
    if (BFTokenizeType(Tokenizer, OutType))
    {
        Token->CastTo = OutType;
    }
    else
    {

        Token->CastTo = Types[0];
    }
    bf_token* Val = BFTokenizeExpr(Tokenizer, CurrentScope);
    if (Val == 0)
    {
        printf("ERR: cast: Value to cast is not found\n");
    }
    Token->First = Val;
    Tokenizer->At = NextMatchingBr + 1;
    return Token;
}
bf_token* BFTokenizeReturn(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    size_t NextBr = BFTellNext(Tokenizer, '(');
    if (NextBr == 0xFFFFFFFF) 
    {
        printf("ERR: return: Expected (\n");
        return 0;
    }
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) 
    {
        printf("ERR: return: Expected matching )\n");
        return 0;
    }
    Tokenizer->At = NextBr + 1;
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    Token->Type = BFRETURN;
    bf_token* Val = BFTokenizeExpr(Tokenizer, CurrentScope);
    if (Val == 0)
    {
        printf("ERR: return: Value to return is not found\n");
    }
    Token->First = Val;
    Tokenizer->At = NextMatchingBr + 1;
    return Token;
}

bf_token* BFTokenizePrint(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    size_t NextBr = BFTellNext(Tokenizer, '(');
    if (NextBr == 0xFFFFFFFF) 
    {
        printf("ERR: print: Expected (\n");
        return 0;
    }
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) 
    {
        printf("ERR: print: Expected matching )\n");
        return 0;
    }
    Tokenizer->At = NextBr + 1;
    bf_token* Character = BFTokenizeExpr(Tokenizer, CurrentScope);
    if (Character == 0) 
    {
        printf("ERR: print: Character failed to parse!\n");
        return 0;
    }
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    Token->Type = BFPRINT;
    Token->First = Character;
    return Token;
}


bf_token* BFTokenizeWhile(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    size_t NextBr = BFTellNext(Tokenizer, '(');
    if (NextBr == 0xFFFFFFFF) 
    {
        printf("ERR: while: Expected (\n");
        return 0;
    }
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) 
    {
        printf("ERR: while: Expected matching )\n");
        return 0;
    }
    Tokenizer->At = NextBr + 1;
    bf_token* Conditional = BFTokenizeExpr(Tokenizer, CurrentScope);
    if (Conditional == 0) 
    {
        printf("ERR: while: Conditional failed to parse!\n");
        return 0;
    }
    Tokenizer->At = NextMatchingBr + 1;
    size_t NextMatchingSqrBr = BFTellNextMatching(Tokenizer, '}', '{');
    if (NextMatchingSqrBr == 0xFFFFFFFF) 
    {
        printf("ERR: while: Expected matching }\n");
        return 0;
    }
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    Token->Type = BFWHILE;
    Token->First = Conditional;
    bf_scope* IfScope = (bf_scope*)kmalloc(sizeof(bf_scope));
    IfScope->Lines = LinkedList<bf_token*>();
    IfScope->Parent = CurrentScope;
    IfScope->Vars = (bf_variable**)kmalloc(sizeof(bf_variable*));
    IfScope->VarsCount = 0;
    IfScope->ArgsCount = 0;
    
    Token->NextScope = IfScope;

    Tokenizer->At++;
    
    while (Tokenizer->At < NextMatchingSqrBr)
    {
        size_t NextSemi = BFTellNextSemi(Tokenizer);
        bf_token* NewTok = BFTokenizeExpr(Tokenizer, IfScope);
        
        if (NewTok == 0) 
        {
            printf("ERR: while: Line failed to parse!\n");
            return 0;
        }
        
        IfScope->Lines.PushBack(NewTok);
        Tokenizer->At = NextSemi + 1;
    }
    return Token;
}

bf_token* BFTokenizeFor(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    size_t NextBr = BFTellNext(Tokenizer, '(');
    if (NextBr == 0xFFFFFFFF) 
    {
        printf("ERR: for: Expected (\n");
        return 0;
    }
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, ')', '(');
    if (NextMatchingBr == 0xFFFFFFFF) 
    {
        printf("ERR: for: Expected matching )\n");
        return 0;
    }
    
    bf_token* Token = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Token, 0, sizeof(bf_token));
    Token->Type = BFFOR;

    Tokenizer->At = NextBr + 1;
    bf_token* Initial = BFTokenizeExpr(Tokenizer, CurrentScope);
    if (Initial == 0) 
    {
        printf("ERR: for: Initial failed to parse!\n");
        return 0;
    }

    Token->First = Initial;

    size_t NextSemi = BFTellNext(Tokenizer, ';');
    if (NextSemi == 0xFFFFFFFF || NextSemi > NextMatchingBr)
    {
        printf("ERR: for: Expected first semicolon\n");
        return 0;
    }
    Tokenizer->At = NextSemi + 1;
    if (Tokenizer->Code[Tokenizer->At] != ';')
    {
        bf_token* Conditional = BFTokenizeExpr(Tokenizer, CurrentScope);
        if (Conditional == 0) 
        {
            printf("ERR: for: Conditional failed to parse!\n");
            return 0;
        }
        Token->Second = Conditional;
    }
    NextSemi = BFTellNext(Tokenizer, ';');
    if (NextSemi == 0xFFFFFFFF || NextSemi > NextMatchingBr)
    {
        printf("ERR: for: Expected second semicolon\n");
        return 0;
    }
    Tokenizer->At = NextSemi + 1;
    if (Tokenizer->Code[Tokenizer->At] != ')')
    {
        bf_token* Iterator = BFTokenizeExpr(Tokenizer, CurrentScope);
        if (Iterator == 0) 
        {
            printf("ERR: for: Iterator failed to parse!\n");
            return 0;
        }
        Token->Third = Iterator;
    }
    Tokenizer->At = NextMatchingBr + 1;
    size_t NextMatchingSqrBr = BFTellNextMatching(Tokenizer, '}', '{');
    if (NextMatchingSqrBr == 0xFFFFFFFF) 
    {
        printf("ERR: for: Expected matching }\n");
        return 0;
    }
    
    bf_scope* IfScope = (bf_scope*)kmalloc(sizeof(bf_scope));
    IfScope->Lines = LinkedList<bf_token*>();
    IfScope->Parent = CurrentScope;
    IfScope->Vars = (bf_variable**)kmalloc(sizeof(bf_variable*));
    IfScope->VarsCount = 0;
    IfScope->ArgsCount = 0;
    
    Token->NextScope = IfScope;

    Tokenizer->At++;
    
    while (Tokenizer->At < NextMatchingSqrBr)
    {
        size_t NextSemi = BFTellNextSemi(Tokenizer);
        bf_token* NewTok = BFTokenizeExpr(Tokenizer, IfScope);
        
        if (NewTok == 0) 
        {
            printf("ERR: for: Line failed to parse!\n");
            return 0;
        }
        
        IfScope->Lines.PushBack(NewTok);
        Tokenizer->At = NextSemi + 1;
    }
    return Token;
}

bf_token* BFTokenizeExpr(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{
    bf_token* Tok = (bf_token*)kmalloc(sizeof(bf_token));
    kmemset(Tok, 0, sizeof(bf_token));

    bf_type IsTyped_Type;
    bool IsTyped = BFTokenizeType(Tokenizer, IsTyped_Type);

    if (!IsTyped)
    {
        if (Tokenizer->Code[Tokenizer->At] == '(')
        {

            size_t NextMatching = BFTellNextMatching(Tokenizer, ')', '(');

            if (NextMatching == 0xFFFFFFFF)
            {
                return NULL;
            }
            Tokenizer->At++;
            bf_token* NextToken = BFTokenizeExpr(Tokenizer, CurrentScope);
            Tokenizer->At = NextMatching + 1;
            
            if (Tokenizer->Code[Tokenizer->At] == ';' || Tokenizer->Code[Tokenizer->At] == ')')
            {
                return NextToken;
            }
            Tok->First = NextToken;
            Tok->Type = BFTokenizeOpType(Tokenizer);
            Tok->Second = BFTokenizeExpr(Tokenizer, CurrentScope);

            return Tok;
        }
    }
    size_t NextOp = BFFindNextOperator(Tokenizer);
    size_t NextSemi = BFTellNext(Tokenizer, ';');
    size_t NextBr = BFTellNext(Tokenizer, ')');
    size_t NextRSqrBr = BFTellNext(Tokenizer, ')');
    size_t NextCom = BFTellNext(Tokenizer, ',');
    if (NextOp < NextBr) NextBr = NextOp;
    if (NextSemi < NextBr) NextBr = NextSemi;
    if (NextRSqrBr < NextBr) NextBr = NextRSqrBr;
    if (NextCom < NextBr) NextBr = NextCom;

    size_t NextSqrBr = BFTellNext(Tokenizer, '(');
    

    if (NextSqrBr != 0xFFFFFFFF)
    {
        String CheckStr = String();
        for (int I = Tokenizer->At;I < NextSqrBr;I++)
        {
            CheckStr.PushBack(Tokenizer->Code[I]);
        }
        if (StrEqualsWith(CheckStr, StrFromCStr("if")))
        {
            return BFTokenizeIf(Tokenizer, CurrentScope);
        }
        else if (StrEqualsWith(CheckStr, StrFromCStr("while")))
        {
            return BFTokenizeWhile(Tokenizer, CurrentScope);
        }
        else if (StrEqualsWith(CheckStr, StrFromCStr("for")))
        {
            return BFTokenizeFor(Tokenizer, CurrentScope);
        }
        else if (StrEqualsWith(CheckStr, StrFromCStr("print")))
        {
            return BFTokenizePrint(Tokenizer, CurrentScope);
        }
        else if (StrEqualsWith(CheckStr, StrFromCStr("cast")))
        {
            return BFTokenizeCast(Tokenizer, CurrentScope);
        }
        else if (StrEqualsWith(CheckStr, StrFromCStr("return")))
        {
            return BFTokenizeReturn(Tokenizer, CurrentScope);
        }
    }
    if (NextSqrBr < NextBr && NextRSqrBr != 0xFFFFFFFF)
    {

        bf_token* MyTok = BFTokenizeFuncCall(Tokenizer, CurrentScope);

        if (NextRSqrBr == NextCom - 1 || NextRSqrBr == NextSemi - 1) return MyTok;

        bf_token* SurroundToken = (bf_token*)kmalloc(sizeof(bf_token));
        kmemset(SurroundToken, 0, sizeof(bf_token));
        Tokenizer->At = NextRSqrBr + 1;
        SurroundToken->First = MyTok;
        SurroundToken->Type = BFTokenizeOpType(Tokenizer);
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
        bf_variable* OutPtr;
        bf_name VarName;
        VarName.Name = Tokenizer->Code + Tokenizer->At;
        VarName.Len = NextBr - Tokenizer->At;
        size_t OldVarLen = VarName.Len;

        for (int I = 0;I < VarName.Len;I++)
        {
            if (VarName.Name[I] == '.')
            {
                VarName.Len = I;
                break;
            }
        }

        if (BFFindVariable(VarName, CurrentScope, &OutPtr))
        {
            Tok->IsVar = true;
            Tok->Var = OutPtr;
            size_t OldTokAt = Tokenizer->At;
            bf_token* IfMemberAccess = BFTokenizeMemberAccess(Tokenizer, OutPtr, OutPtr->Type, Tokenizer->At + OldVarLen);
            Tokenizer->At = OldTokAt;
            if (IfMemberAccess != 0)
            {
                Tok->Type = BFMEMBERACCESS;
                Tok->First = IfMemberAccess;
                Tok->IsFinalMember = false;
            }
        }
        else
        {
            

            bf_variable* NewVar = (bf_variable*)kmalloc(sizeof(bf_variable));
            NewVar->Name = VarName;
            
            NewVar->Val = 0;
            if (IsTyped) NewVar->Type = IsTyped_Type;
            else NewVar->Type = Types[0];
            
            CurrentScope->Vars[CurrentScope->VarsCount++] = NewVar;
            bf_variable** NewVars = (bf_variable**)kmalloc(sizeof(bf_variable*) * (CurrentScope->VarsCount + 1));
            kmemcpy(NewVars, CurrentScope->Vars, sizeof(bf_variable*) * CurrentScope->VarsCount);
            kfree(CurrentScope->Vars);
            CurrentScope->Vars = NewVars;

            Tok->IsVar = true;
            Tok->Var = NewVar;
        }
    }
    
    if (NextBr < NextOp || NextOp == 0xFFFFFFFF)
    {
        return Tok;
    }

    bf_token* SurroundTok = (bf_token*)kmalloc(sizeof(bf_token));
    SurroundTok->First = Tok;

    NextOp = BFFindNextOperator(Tokenizer);
    Tokenizer->At = NextOp;
    SurroundTok->Type = BFTokenizeOpType(Tokenizer);
    
    
    
    SurroundTok->Second = BFTokenizeExpr(Tokenizer, CurrentScope);
    return SurroundTok;
}

bf_token* BFTokenizeLine(bf_tokenizer* Tokenizer, bf_scope* CurrentScope)
{

    return BFTokenizeExpr(Tokenizer, CurrentScope);
}

bf_member BFTokenizeStructMember(bf_tokenizer* Tokenizer)
{
    bf_member CurMember;
    bf_type IsTyped_Type;
    bool IsTyped = BFTokenizeType(Tokenizer, IsTyped_Type);
    if (!IsTyped) return CurMember;
    size_t NextSemi = BFTellNext(Tokenizer, ';');
    String CurMemberName = StrFromArray((char*)Tokenizer->Code + Tokenizer->At, NextSemi - Tokenizer->At); 
    CurMember.Name = CurMemberName;
    CurMember.Type = IsTyped_Type;
    Tokenizer->At = NextSemi + 1;
    return CurMember;
}

void BFTokenizeStruct(bf_tokenizer* Tokenizer)
{
    size_t EndBr = BFTellNext(Tokenizer, '}');
    if (EndBr == 0xFFFFFFFF) return;
    size_t BeginBr = BFTellNext(Tokenizer, '{');
    bf_type CurType;
    CurType.IsStruct = true;
    CurType.StructMembers = LinkedList<bf_member>();
    CurType.Name = StrFromArray((char*)Tokenizer->Code + Tokenizer->At, BeginBr - Tokenizer->At);
    Tokenizer->At = BeginBr + 1;
    while (Tokenizer->At < EndBr)
    {
        CurType.StructMembers.PushBack(BFTokenizeStructMember(Tokenizer));
    }
    Tokenizer->At = EndBr + 1;
    Types.PushBack(CurType);
}

void BFTokenizeFunction(bf_tokenizer* Tokenizer)
{
    
    size_t NextSqrBr = BFTellNext(Tokenizer, '(');
    size_t NextCurBr = BFTellNext(Tokenizer, '{');
    if (NextCurBr < NextSqrBr)
    {
        BFTokenizeStruct(Tokenizer);
        return;
    }
    bf_type OutReturnType;
    bool isReturnTyped = BFTokenizeType(Tokenizer, OutReturnType);
    
    if (NextSqrBr == 0xFFFFFFFF) return;
    
    bf_function* CurrentFunction = (bf_function*)kmalloc(sizeof(bf_function));
    CurrentFunction->Name.Name = Tokenizer->Code + Tokenizer->At;
    CurrentFunction->Name.Len = NextSqrBr - Tokenizer->At;
    CurrentFunction->RootScope.Lines = LinkedList<bf_token*>();
    CurrentFunction->RootScope.Parent = 0;
    if (isReturnTyped) CurrentFunction->Type = OutReturnType;
    else CurrentFunction->Type = Types[0];
    Tokenizer->At = NextSqrBr + 1;
    size_t NextSqrBr2 = BFTellNext(Tokenizer, ')');

    if (NextSqrBr2 == 0xFFFFFFFF) return;


    // Parse Arguments

    CurrentFunction->RootScope.Vars = (bf_variable**)kmalloc(sizeof(bf_variable*));
    CurrentFunction->RootScope.ArgsCount = 0;
    CurrentFunction->Params = LinkedList<String>();

    if (NextSqrBr + 1 != NextSqrBr2)
    {
        while (1)
        {
            size_t NextCom = BFTellNext(Tokenizer, ',');
            bool BreakOut = NextCom > NextSqrBr2 || NextCom == 0xFFFFFFFF;
            if (BreakOut)
            {
                NextCom = NextSqrBr2;
            }
            bf_variable *Var = (bf_variable*)kmalloc(sizeof(bf_variable));
            
            if (Tokenizer->Code[Tokenizer->At] == '&')
            {
                Tokenizer->At++;
                Var->Ref = true;
            }
            else
            {
                Var->Ref = false;
            }
            
            bf_type OutType;
            bool IsTyped = BFTokenizeType(Tokenizer, OutType);
            Var->Name.Name = Tokenizer->Code + Tokenizer->At;
            Var->Name.Len = NextCom - Tokenizer->At;
            Var->Val = 0;
            if (IsTyped) Var->Type = OutType;
            else Var->Type = Types[0];
            CurrentFunction->Params.PushBack(StrFromArray((char*)Var->Name.Name, Var->Name.Len));
            CurrentFunction->RootScope.Vars[CurrentFunction->RootScope.ArgsCount++] = Var;
            bf_variable** NewArgs = (bf_variable**)kmalloc(sizeof(bf_variable*) * (CurrentFunction->RootScope.ArgsCount + 1));
            kmemcpy(NewArgs, CurrentFunction->RootScope.Vars, sizeof(bf_variable*) * CurrentFunction->RootScope.ArgsCount);
            kfree(CurrentFunction->RootScope.Vars);
            CurrentFunction->RootScope.Vars = NewArgs;
            Tokenizer->At = NextCom + 1;
            if (BreakOut) break;

        }
    }

    CurrentFunction->RootScope.VarsCount = CurrentFunction->RootScope.ArgsCount;

    Tokenizer->At = NextSqrBr2 + 1;
    size_t NextMatchingBr = BFTellNextMatching(Tokenizer, '}', '{');
    if (NextMatchingBr == 0xFFFFFFFF) return;

    Tokenizer->At++;


    while (Tokenizer->At < NextMatchingBr)
    {
        size_t NextSemi = BFTellNextSemi(Tokenizer);
        if (NextSemi == 0xFFFFFFFF) return;
        CurrentFunction->RootScope.Lines.PushBack(BFTokenizeLine(Tokenizer, &CurrentFunction->RootScope));

        Tokenizer->At = NextSemi + 1;
    }
    Tokenizer->At = NextMatchingBr + 1;
    Tokenizer->Functions[Tokenizer->BFNumFunctions++] = CurrentFunction;
    bf_function** NewFuncs = (bf_function**)kmalloc(sizeof(bf_function*) * (Tokenizer->BFNumFunctions + 1));
    kmemset(NewFuncs, 0, sizeof(bf_function*) * (Tokenizer->BFNumFunctions + 1));
    kmemcpy(NewFuncs, Tokenizer->Functions, sizeof(bf_function*) * Tokenizer->BFNumFunctions);
    kfree(Tokenizer->Functions);
    Tokenizer->Functions = NewFuncs;
}

// Here we go!
bf_function** BFTokenize(uint8_t* Code, size_t Len)
{
    // First, lets initialize the default types.
    Types = LinkedList<bf_type>();
    bf_type IntType;
    IntType.IsFloat = false;
    IntType.IsStruct = false;
    IntType.Bits = BFBITS32;
    IntType.Name = StrFromCStr("int");
    Types.PushBack(IntType);
    bf_type shortType;
    shortType.IsFloat = false;
    shortType.IsStruct = false;
    shortType.Bits = BFBITS16;
    shortType.Name = StrFromCStr("short");
    Types.PushBack(shortType);
    bf_type byteType;
    byteType.IsFloat = false;
    byteType.IsStruct = false;
    byteType.Bits = BFBITS8;
    byteType.Name = StrFromCStr("byte");
    Types.PushBack(byteType);

    bf_tokenizer Tokenizer;
    Tokenizer.At = 0;
    Tokenizer.Code = Code;
    Tokenizer.Size = Len;
    Tokenizer.Functions = (bf_function**)kmalloc(sizeof(bf_function*));

    Tokenizer.BFNumFunctions = 0;
    while (BFCodeStep(&Tokenizer))
    {
        BFTokenizeFunction(&Tokenizer);
    }
    
    return Tokenizer.Functions;
}

int BFExecuteFunc(bf_function* Func);

int BFExecuteToken(bf_token* Tok)
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

void BFIsVariableIn(String str, LinkedList<String> &variables)
{
    for (int i = 0; i < variables.size; i++) {
        if (StrEqualsWith(variables[i], str)) {
            return;
        }
    }

    variables.PushBack(str); 
}

String BFGetReg(int reg)
{
    switch (reg)
    {
        case 0:
            return StrFromCStr("eax");
        case 1:
            return StrFromCStr("ebx");
        case 2:
            return StrFromCStr("ecx");
        case 3:
            return StrFromCStr("edx");
        default:
            return StrFromCStr("UNKNOWN_REG");
    }
}

String BFGetStrBits(bf_bits Bits)
{
    switch (Bits)
    {
        case BFBITS8:
            return StrFromCStr("byte");
        case BFBITS16:
            return StrFromCStr("word");
        case BFBITS32:
            return StrFromCStr("dword");
        default:
            return StrFromCStr("UNKNOWN_BITS");
    }
}

int ConditionalCount;

String BFGetCurReg(int &regCount, LinkedList<String> &OutCode)
{
    if (regCount < 4)
    {
        return BFGetReg(regCount++);
    }
    else
    {
        return StrFromCStr("edx");
    }
}

// Could break
void BFCompileStructCopy(String Dest, String Src, bf_type Type, LinkedList<String> &OutCode)
{
    for (int I = 0;I < Type.StructMembers.size;I++)
    {
        String DestMember = String();
        String SrcMember = String();
        DestMember = StrAppend(DestMember, Dest);
        SrcMember = StrAppend(SrcMember, Src);
        DestMember.PushBack('.');
        SrcMember.PushBack('.');
        DestMember = StrAppend(DestMember, Type.StructMembers[I].Name);
        SrcMember = StrAppend(SrcMember, Type.StructMembers[I].Name);
        if (Type.StructMembers[I].Type.IsStruct)
        {
            BFCompileStructCopy(DestMember, SrcMember, Type.StructMembers[I].Type, OutCode);
            continue; 
        }
        bf_type MemberType = Type.StructMembers[I].Type;
        
        SrcMember.PushFront('[');
        SrcMember.PushBack(']');
        DestMember.PushFront('[');
        DestMember.PushBack(']');

        if (MemberType.Bits == BFBITS8)
        {
            String output = StrFromCStr("mov al, ");
            output = StrAppend(output, SrcMember);
            OutCode.PushBack(output);
            output = StrFromCStr("mov ");
            output = StrAppend(output, DestMember);
            output = StrAppend(output, StrFromCStr(", al"));
            OutCode.PushBack(output);
        }
        else if (MemberType.Bits == BFBITS16)
        {
            String output = StrFromCStr("mov ax, ");
            output = StrAppend(output, SrcMember);
            OutCode.PushBack(output);
            output = StrFromCStr("mov ");
            output = StrAppend(output, DestMember);
            output = StrAppend(output, StrFromCStr(", ax"));
            OutCode.PushBack(output);
        }
        else if (MemberType.Bits == BFBITS32)
        {
            String output = StrFromCStr("mov eax, ");
            output = StrAppend(output, SrcMember);
            OutCode.PushBack(output);
            output = StrFromCStr("mov ");
            output = StrAppend(output, DestMember);
            output = StrAppend(output, StrFromCStr(", eax"));
            OutCode.PushBack(output);
        }
    }
}

bf_type DesiredType;
bf_type FuncReturnType;

bf_compiled_token BFCompileToken(bf_token* Tok, LinkedList<String> &OutCode, LinkedList<String> &Variables, String jmpTo)
{
    if (Tok->IsConstant)
    {
        return { StrFormat("%d", Tok->Const.Val), DesiredType };
    }
    if (Tok->IsVar)
    {
        
        String varName = StrFromArray((char*)Tok->Var->Name.Name, Tok->Var->Name.Len);
        bf_type CurType = Tok->Var->Type;
        if (Tok->Type == BFMEMBERACCESS)
        {
            varName.PushFront('.');
            do
            {
                Tok = Tok->First;
                varName.PushBack('.');
                varName = StrAppend(varName, Tok->Member->Name);
                CurType = Tok->Member->Type;
            } while (!Tok->IsFinalMember);
        }
        else 
        {
            BFIsVariableIn(varName, Variables);
        
            varName.PushFront('.');
        }
        if (!CurType.IsStruct)
        {
            varName.PushFront('[');
            varName.PushBack(']');
            varName.PushFront(' ');
            varName = StrAppend(BFGetStrBits(CurType.Bits), varName);
        }
        return { varName, CurType };
    }
    String output;
    String optOutput = String();
    
    bool Override = false;
    bool MovToFirst = false;
    if (Tok->Type == BFIF)
    {
        String NewJmpTo = StrFormat(".econditional_%d", ConditionalCount++);
        BFCompileToken(Tok->First, OutCode, Variables, NewJmpTo);
        NewJmpTo.PushBack(':');
        for (int I = 0;I < Tok->NextScope->Lines.size;I++)
        {
            BFCompileToken(Tok->NextScope->Lines[I], OutCode, Variables, NewJmpTo);    
        }
        OutCode.PushBack(NewJmpTo);
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFWHILE)
    {
        String NewJmpTo = StrFormat(".while_%d:", ConditionalCount++);
        OutCode.PushBack(NewJmpTo);
        String EndNewJmpTo = StrFormat(".ewhile_%d", ConditionalCount - 1);
        BFCompileToken(Tok->First, OutCode, Variables, EndNewJmpTo);
        EndNewJmpTo.PushBack(':');
        for (int I = 0;I < Tok->NextScope->Lines.size;I++)
        {
            int newRegCount = 0;
            BFCompileToken(Tok->NextScope->Lines[I], OutCode, Variables, NewJmpTo);    
        }
        output = StrFromCStr("jmp ");
        NewJmpTo.PopBack();
        output = StrAppend(output, NewJmpTo);
        
        OutCode.PushBack(output);
        OutCode.PushBack(EndNewJmpTo);
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFFOR)
    {
        BFCompileToken(Tok->First, OutCode, Variables, StrFromCStr(""));
        String NewJmpTo = StrFormat(".for_%d:", ConditionalCount++);
        
        OutCode.PushBack(NewJmpTo);
        String EndNewJmpTo = StrFormat(".efor_%d", ConditionalCount - 1);
        if (Tok->Second) BFCompileToken(Tok->Second, OutCode, Variables, EndNewJmpTo);
        EndNewJmpTo.PushBack(':');
        for (int I = 0;I < Tok->NextScope->Lines.size;I++)
        {
            int newRegCount = 0;
            BFCompileToken(Tok->NextScope->Lines[I], OutCode, Variables, NewJmpTo);    
        }
        if (Tok->Third) BFCompileToken(Tok->Third, OutCode, Variables, EndNewJmpTo);

        output = StrFromCStr("jmp ");
        NewJmpTo.PopBack();
        output = StrAppend(output, NewJmpTo);
        OutCode.PushBack(output);
        OutCode.PushBack(EndNewJmpTo);
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFPRINT)
    {
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, StrFromCStr(""));
        if (firstStr.Type.IsStruct)
        {
            printf("compile: print: FATAL! Expected integral, got struct\n");  
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8) output = StrFromCStr("mov al, ");
        else if (firstStr.Type.Bits == BFBITS16) output = StrFromCStr("mov ax, ");
        else if (firstStr.Type.Bits == BFBITS32) output = StrFromCStr("mov eax, ");
        OutCode.PushBack(StrAppend(output, firstStr.Asm));
        OutCode.PushBack(StrFromCStr("mov [__print_c], al"));
        OutCode.PushBack(StrFromCStr("mov eax, 4"));
        OutCode.PushBack(StrFromCStr("mov ebx, 1"));
        OutCode.PushBack(StrFromCStr("mov ecx, __print_c"));
        OutCode.PushBack(StrFromCStr("mov edx, 1"));
        OutCode.PushBack(StrFromCStr("int 0x80"));  
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFCAST)
    {
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, StrFromCStr(""));
        if (firstStr.Type.IsStruct)
        {
            printf("compile: cast: FATAL! Expected integral to cast, got struct\n");  
            return { StrFromCStr(""), Types[0] };
        }
        if (Tok->CastTo.IsStruct)
        {
            printf("compile: cast: FATAL! Can't cast to struct\n");  
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8) output = StrFromCStr("mov bl, ");
        else if (firstStr.Type.Bits == BFBITS16) output = StrFromCStr("mov bx, ");
        else if (firstStr.Type.Bits == BFBITS32) output = StrFromCStr("mov ebx, ");
        
        output = StrAppend(output, firstStr.Asm);

        if (Tok->CastTo.Bits == BFBITS8) 
        {
            OutCode.PushBack(StrFromCStr("xor bl, bl"));
            OutCode.PushBack(output);
            return { StrFromCStr("bl"), Tok->CastTo };
        }
        else if (Tok->CastTo.Bits == BFBITS16) 
        {
            OutCode.PushBack(StrFromCStr("xor bx, bx"));
            OutCode.PushBack(output);
            return { StrFromCStr("bx"), Tok->CastTo };
        }
        else if (Tok->CastTo.Bits == BFBITS32) 
        {
            OutCode.PushBack(StrFromCStr("xor ebx, ebx"));
            OutCode.PushBack(output);
            return { StrFromCStr("ebx"), Tok->CastTo };
        }
        return { StrFromCStr(""), Types[0] };
    }

    else if (Tok->Type == BFRETURN)
    {
        DesiredType = FuncReturnType;
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, StrFromCStr(""));
        if (firstStr.Type.IsStruct)
        {
            printf("compile: return: FATAL! Expected integral to cast, got struct\n");  
            return { StrFromCStr(""), Types[0] };
        }
        if (FuncReturnType.IsStruct)
        {
            printf("compile: return: FATAL! Can't return a struct\n");  
            return { StrFromCStr(""), Types[0] };
        }
        if (!StrEqualsWith(firstStr.Type.Name, FuncReturnType.Name))
        {
            printf("compile: return: FATAL! Type mismatch with function return type\n");  
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8) output = StrFromCStr("mov bl, ");
        else if (firstStr.Type.Bits == BFBITS16) output = StrFromCStr("mov bx, ");
        else if (firstStr.Type.Bits == BFBITS32) output = StrFromCStr("mov ebx, ");
        
        output = StrAppend(output, firstStr.Asm);

        OutCode.PushBack(output);

        OutCode.PushBack(StrFromCStr("ret"));

        if (FuncReturnType.Bits == BFBITS8) return { StrFromCStr("bl"), FuncReturnType };
        else if (FuncReturnType.Bits == BFBITS16) return { StrFromCStr("bx"), FuncReturnType };
        else if (FuncReturnType.Bits == BFBITS32) return { StrFromCStr("ebx"), FuncReturnType };
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFASSIGN)
    {
        DesiredType = Types[0];
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, StrFromCStr(""));
        DesiredType = firstStr.Type;
        
        bf_compiled_token secondStr = BFCompileToken(Tok->Second, OutCode, Variables, StrFromCStr(""));
        
        if (!StrEqualsWith(firstStr.Type.Name, secondStr.Type.Name))
        {
            printf("compile: assign: FATAL! Type mismatch\n");
            return { StrFromCStr(""), Types[0] };
        }

        if (firstStr.Type.IsStruct)
        {
            BFCompileStructCopy(firstStr.Asm, secondStr.Asm, firstStr.Type, OutCode);
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8)
        {
            output = StrFromCStr("mov al, ");
            output = StrAppend(output, secondStr.Asm);
            OutCode.PushBack(output);
            output = StrFromCStr("mov ");
            output = StrAppend(output, firstStr.Asm);
            output = StrAppend(output, StrFromCStr(", al"));
            OutCode.PushBack(output);
            OutCode.PushBack(StrFromCStr("mov bl, al"));
            return { StrFromCStr("bl"), firstStr.Type };
        }
        else if (firstStr.Type.Bits == BFBITS16)
        {
            output = StrFromCStr("mov ax, ");
            output = StrAppend(output, secondStr.Asm);
            OutCode.PushBack(output);
            output = StrFromCStr("mov ");
            output = StrAppend(output, firstStr.Asm);
            output = StrAppend(output, StrFromCStr(", ax"));
            OutCode.PushBack(output);
            OutCode.PushBack(StrFromCStr("mov bx, ax"));
            return { StrFromCStr("bx"), firstStr.Type };
        }
        else if (firstStr.Type.Bits == BFBITS32)
        {
            output = StrFromCStr("mov eax, ");
            output = StrAppend(output, secondStr.Asm);
            OutCode.PushBack(output);
            output = StrFromCStr("mov ");
            output = StrAppend(output, firstStr.Asm);
            output = StrAppend(output, StrFromCStr(", eax"));
            OutCode.PushBack(output);
            OutCode.PushBack(StrFromCStr("mov ebx, eax"));
            return { StrFromCStr("ebx"), firstStr.Type };
        }
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFADD)
    {
        output = StrFromCStr("add ");
    }
    else if (Tok->Type == BFSUB)
    {
        output = StrFromCStr("sub ");
    }
    else if (Tok->Type == BFSHR || Tok->Type == BFSHL)
    {
        DesiredType = Types[0];
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, jmpTo);
        DesiredType = firstStr.Type;
        bf_compiled_token secondStr = BFCompileToken(Tok->Second, OutCode, Variables, jmpTo);
        
        if (firstStr.Type.IsStruct || secondStr.Type.IsStruct)
        {
            printf("compile: bitshift: FATAL! Expected integral, got struct\n");  
            return { StrFromCStr(""), Types[0] };
        }

        if (firstStr.Type.IsFloat || secondStr.Type.IsFloat)
        {
            printf("compile: bitshift: FATAL! Expected integral, got float\n");  
            return { StrFromCStr(""), Types[0] };
        }        

        if (secondStr.Type.Bits == BFBITS8) OutCode.PushBack(StrAppend(StrFromCStr("mov cl, "), secondStr.Asm));
        else if (secondStr.Type.Bits == BFBITS16) OutCode.PushBack(StrAppend(StrFromCStr("mov cx, "), secondStr.Asm));
        else if (secondStr.Type.Bits == BFBITS32) OutCode.PushBack(StrAppend(StrFromCStr("mov ecx, "), secondStr.Asm));
        
        if (firstStr.Type.Bits == BFBITS8) OutCode.PushBack(StrAppend(StrFromCStr("mov al, "), firstStr.Asm));
        else if (firstStr.Type.Bits == BFBITS16) OutCode.PushBack(StrAppend(StrFromCStr("mov ax, "), firstStr.Asm));
        else if (firstStr.Type.Bits == BFBITS32) OutCode.PushBack(StrAppend(StrFromCStr("mov eax, "), firstStr.Asm));
        
        output = Tok->Type == BFSHR ? StrFromCStr("shr ") : StrFromCStr("shl ");
        if (firstStr.Type.Bits == BFBITS8) 
        {
            OutCode.PushBack(StrAppend(output, StrFromCStr("al, cl")));
            OutCode.PushBack(StrFromCStr("mov bl, al"));
            return { StrFromCStr("bl"), firstStr.Type };
        }
        else if (firstStr.Type.Bits == BFBITS16) 
        {
            OutCode.PushBack(StrAppend(output, StrFromCStr("ax, cl")));
            OutCode.PushBack(StrFromCStr("mov bx, ax"));
            return { StrFromCStr("bx"), firstStr.Type };
        }
        else if (firstStr.Type.Bits == BFBITS32) 
        {
            OutCode.PushBack(StrAppend(output, StrFromCStr("eax, cl")));
            OutCode.PushBack(StrFromCStr("mov ebx, eax"));
            return { StrFromCStr("ebx"), firstStr.Type };
        }
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFLESSTHAN)
    {
        output = StrFromCStr("cmp ");
        optOutput = StrFromCStr("jge ");
        optOutput = StrAppend(optOutput, jmpTo);
        Override = true;
    }
    else if (Tok->Type == BFMORETHAN)
    {
        output = StrFromCStr("cmp ");
        optOutput = StrFromCStr("jle ");
        optOutput = StrAppend(optOutput, jmpTo);
        Override = true;
    }
    else if (Tok->Type == BFEQUALS)
    {
        output = StrFromCStr("cmp ");
        optOutput = StrFromCStr("jne ");
        optOutput = StrAppend(optOutput, jmpTo);
        Override = true;
    }
    else if (Tok->Type == BFMUL)
    {
        
        DesiredType = Types[0];
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, jmpTo);
        DesiredType = firstStr.Type;
        if (firstStr.Type.IsStruct)
        {
            printf("compile: multiply: FATAL! Expected float or integral, got struct\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8) output = StrFromCStr("mov al, ");
        else if (firstStr.Type.Bits == BFBITS16) output = StrFromCStr("mov ax, ");
        else if (firstStr.Type.Bits == BFBITS32) output = StrFromCStr("mov eax, ");
        OutCode.PushBack(StrAppend(output, firstStr.Asm));
        bf_compiled_token secondStr = BFCompileToken(Tok->Second, OutCode, Variables, jmpTo);
        if (!StrEqualsWith(firstStr.Type.Name, secondStr.Type.Name))
        {
            printf("compile: multiply: FATAL! Type mismatch\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (secondStr.Type.Bits == BFBITS8)
        { 
            output = StrFromCStr("mov bl, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("imul bl"));
            OutCode.PushBack(StrFromCStr("mov bl, al"));
            return { StrFromCStr("bl"), firstStr.Type };
        }
        else if (secondStr.Type.Bits == BFBITS16)
        { 
            output = StrFromCStr("mov bx, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("imul bx"));
            OutCode.PushBack(StrFromCStr("mov bx, ax"));
            return { StrFromCStr("bx"), firstStr.Type };
        }
        else if (secondStr.Type.Bits == BFBITS32)
        { 
            output = StrFromCStr("mov ebx, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("imul ebx"));
            OutCode.PushBack(StrFromCStr("mov ebx, eax"));
            return { StrFromCStr("ebx"), firstStr.Type };
        }
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFDIV)
    {
        DesiredType = Types[0];
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, jmpTo);
        DesiredType = firstStr.Type;
        if (firstStr.Type.IsStruct)
        {
            printf("compile: divide: FATAL! Expected float or integral, got struct\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8) output = StrFromCStr("mov al, ");
        else if (firstStr.Type.Bits == BFBITS16) output = StrFromCStr("mov ax, ");
        else if (firstStr.Type.Bits == BFBITS32) output = StrFromCStr("mov eax, ");
        OutCode.PushBack(StrAppend(output, firstStr.Asm));
        OutCode.PushBack(StrFromCStr("xor edx, edx"));
        bf_compiled_token secondStr = BFCompileToken(Tok->Second, OutCode, Variables, jmpTo);
        if (!StrEqualsWith(firstStr.Type.Name, secondStr.Type.Name))
        {
            printf("compile: divide: FATAL! Type mismatch\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (secondStr.Type.Bits == BFBITS8)
        { 
            output = StrFromCStr("mov bl, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("idiv bl"));
            OutCode.PushBack(StrFromCStr("mov bl, al"));
            return { StrFromCStr("bl"), firstStr.Type };
        }
        else if (secondStr.Type.Bits == BFBITS16)
        { 
            output = StrFromCStr("mov bx, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("idiv bx"));
            OutCode.PushBack(StrFromCStr("mov bx, ax"));
            return { StrFromCStr("bx"), firstStr.Type };
        }
        else if (secondStr.Type.Bits == BFBITS32)
        { 
            output = StrFromCStr("mov ebx, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("idiv ebx"));
            OutCode.PushBack(StrFromCStr("mov ebx, eax"));
            return { StrFromCStr("ebx"), firstStr.Type };
        }
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFMODULO)
    {
        DesiredType = Types[0];
        bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, jmpTo);
        DesiredType = firstStr.Type;
        if (firstStr.Type.IsStruct)
        {
            printf("compile: modulo: FATAL! Expected float or integral, got struct\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (firstStr.Type.Bits == BFBITS8) output = StrFromCStr("mov al, ");
        else if (firstStr.Type.Bits == BFBITS16) output = StrFromCStr("mov ax, ");
        else if (firstStr.Type.Bits == BFBITS32) output = StrFromCStr("mov eax, ");
        OutCode.PushBack(StrAppend(output, firstStr.Asm));
        OutCode.PushBack(StrFromCStr("xor edx, edx"));
        bf_compiled_token secondStr = BFCompileToken(Tok->Second, OutCode, Variables, jmpTo);
        if (!StrEqualsWith(firstStr.Type.Name, secondStr.Type.Name))
        {
            printf("compile: modulo: FATAL! Type mismatch\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (secondStr.Type.Bits == BFBITS8)
        { 
            output = StrFromCStr("mov bl, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("idiv bl"));
            OutCode.PushBack(StrFromCStr("mov bl, ah"));
            return { StrFromCStr("bl"), firstStr.Type };
        }
        else if (secondStr.Type.Bits == BFBITS16)
        { 
            output = StrFromCStr("mov bx, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("idiv bx"));
            OutCode.PushBack(StrFromCStr("mov bx, dx"));
            return { StrFromCStr("bx"), firstStr.Type };
        }
        else if (secondStr.Type.Bits == BFBITS32)
        { 
            output = StrFromCStr("mov ebx, ");
            OutCode.PushBack(StrAppend(output, secondStr.Asm));
            OutCode.PushBack(StrFromCStr("idiv ebx"));
            OutCode.PushBack(StrFromCStr("mov ebx, edx"));
            return { StrFromCStr("ebx"), firstStr.Type };
        }
        return { StrFromCStr(""), Types[0] };
    }
    else if (Tok->Type == BFFUNCCALL)
    {
        LinkedList<bf_compiled_token> passedCache;
        for (int I = 0; I < Tok->NumParams; I++)
        {
            DesiredType = Types[0];
            bf_compiled_token passed = BFCompileToken(Tok->Params[I], OutCode, Variables, jmpTo);
            passedCache.PushBack(passed);
            if (!StrEqualsWith(passed.Type.Name, Tok->CallFunction->RootScope.Vars[I]->Type.Name))
            {
                printf("compile: function call: FATAL! Argument type mismatch\n");
                return { StrFromCStr(""), Types[0] };
            }
            if (passed.Type.IsStruct)
            {
                String Dest = String();
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    Dest.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                Dest.PushBack('.');
                Dest = StrAppend(Dest, Tok->CallFunction->Params[I]);
                BFCompileStructCopy(Dest, passed.Asm, passed.Type, OutCode);
            }
            if (passed.Type.Bits == BFBITS8)
            {
                String sParam = StrFromCStr("mov al, ");
                sParam = StrAppend(sParam, passed.Asm);
                OutCode.PushBack(sParam);
                sParam = StrFromCStr("mov [");
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    sParam.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                sParam.PushBack('.');
                sParam = StrAppend(sParam, Tok->CallFunction->Params[I]);
                sParam = StrAppend(sParam, StrFromCStr("], al"));
                OutCode.PushBack(sParam);
            }
            else if (passed.Type.Bits == BFBITS16)
            {
                String sParam = StrFromCStr("mov ax, ");
                sParam = StrAppend(sParam, passed.Asm);
                OutCode.PushBack(sParam);
                sParam = StrFromCStr("mov [");
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    sParam.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                sParam.PushBack('.');
                sParam = StrAppend(sParam, Tok->CallFunction->Params[I]);
                sParam = StrAppend(sParam, StrFromCStr("], ax"));
                OutCode.PushBack(sParam);
            }
            else if (passed.Type.Bits == BFBITS32)
            {
                String sParam = StrFromCStr("mov eax, ");
                sParam = StrAppend(sParam, passed.Asm);
                OutCode.PushBack(sParam);
                sParam = StrFromCStr("mov [");
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    sParam.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                sParam.PushBack('.');
                sParam = StrAppend(sParam, Tok->CallFunction->Params[I]);
                sParam = StrAppend(sParam, StrFromCStr("], eax"));
                OutCode.PushBack(sParam);
            }
        }
        output = StrFromCStr("call ");
        output = StrAppend(output, StrFromArray((char*)Tok->CallFunction->Name.Name, Tok->CallFunction->Name.Len));
        OutCode.PushBack(output);

        for (int I = 0; I < Tok->NumParams; I++)
        {
            bf_compiled_token passed = passedCache[I];
            if (!Tok->CallFunction->RootScope.Vars[I]->Ref) continue;
            if (passed.Type.IsStruct)
            {
                String Dest = String();
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    Dest.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                Dest.PushBack('.');
                Dest = StrAppend(Dest, Tok->CallFunction->Params[I]);
                BFCompileStructCopy(passed.Asm, Dest, passed.Type, OutCode);
            }
            if (passed.Type.Bits == BFBITS8)
            {
                String sParam = StrFromCStr("mov al, [");
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    sParam.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                sParam.PushBack('.');
                sParam = StrAppend(sParam, Tok->CallFunction->Params[I]);
                sParam.PushBack(']');
                OutCode.PushBack(sParam);
                sParam = StrFromCStr("mov ");
                sParam = StrAppend(sParam, passed.Asm);
                sParam = StrAppend(sParam, StrFromCStr(", al"));
                OutCode.PushBack(sParam);
            }
            else if (passed.Type.Bits == BFBITS16)
            {
                String sParam = StrFromCStr("mov ax, [");
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    sParam.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                sParam.PushBack('.');
                sParam = StrAppend(sParam, Tok->CallFunction->Params[I]);
                sParam.PushBack(']');
                OutCode.PushBack(sParam);
                sParam = StrFromCStr("mov ");
                sParam = StrAppend(sParam, passed.Asm);
                sParam = StrAppend(sParam, StrFromCStr(", ax"));
                OutCode.PushBack(sParam);
            }
            else if (passed.Type.Bits == BFBITS32)
            {
                String sParam = StrFromCStr("mov eax, [");
                for (int I = 0;I < Tok->CallFunction->Name.Len;I++)
                {
                    sParam.PushBack(Tok->CallFunction->Name.Name[I]);
                }
                sParam.PushBack('.');
                sParam = StrAppend(sParam, Tok->CallFunction->Params[I]);
                sParam.PushBack(']');
                OutCode.PushBack(sParam);
                sParam = StrFromCStr("mov ");
                sParam = StrAppend(sParam, passed.Asm);
                sParam = StrAppend(sParam, StrFromCStr(", eax"));
                OutCode.PushBack(sParam);
            }
        }

        if (Tok->CallFunction->Type.IsStruct)
        {
            printf("compile: function call: FATAL! Calling function that has return type of struct. Do not give functions return type of struct\n");
            return { StrFromCStr(""), Types[0] };
        }
        if (Tok->CallFunction->Type.Bits == BFBITS8) 
        {
            return { StrFromCStr("bl"), Tok->CallFunction->Type };
        }
        else if (Tok->CallFunction->Type.Bits == BFBITS16) 
        {
            return { StrFromCStr("bx"), Tok->CallFunction->Type };
        }
        else if (Tok->CallFunction->Type.Bits == BFBITS32) 
        {
            return { StrFromCStr("ebx"), Tok->CallFunction->Type };
        }
        return { StrFromCStr(""), Types[0] };
    }
    DesiredType = Types[0];
    bf_compiled_token firstStr = BFCompileToken(Tok->First, OutCode, Variables, jmpTo);
    DesiredType = firstStr.Type;
    if (firstStr.Type.Bits == BFBITS8) OutCode.PushBack(StrAppend(StrFromCStr("mov al, "), firstStr.Asm));
    else if (firstStr.Type.Bits == BFBITS16) OutCode.PushBack(StrAppend(StrFromCStr("mov ax, "), firstStr.Asm));
    else if (firstStr.Type.Bits == BFBITS32) OutCode.PushBack(StrAppend(StrFromCStr("mov eax, "), firstStr.Asm));
    OutCode.PushBack(StrFromCStr("push rax"));
    bf_compiled_token secondStr = BFCompileToken(Tok->Second, OutCode, Variables, jmpTo);
    OutCode.PushBack(StrFromCStr("pop rax"));
    if (secondStr.Type.IsStruct)
    {
        printf("compile: FATAL! Struct used illegally\n");
        return { StrFromCStr(""), Types[0] };
    }
    if (!StrEqualsWith(firstStr.Type.Name, secondStr.Type.Name))
    {
        printf("compile: FATAL! Type mismatch\n");
        return { StrFromCStr(""), Types[0] };
    }
    if (firstStr.Type.Bits == BFBITS8)
    {
        OutCode.PushBack(StrAppend(StrAppend(output, StrFromCStr("al, ")), secondStr.Asm));
        if (MovToFirst) OutCode.PushBack(StrAppend(StrAppend(StrFromCStr("mov "), firstStr.Asm), StrFromCStr(", al"))); 
        if (optOutput.size != 0) OutCode.PushBack(optOutput);
        else OutCode.PushBack(StrFromCStr("mov bl, al"));
        return { StrFromCStr("bl"), firstStr.Type };
    }
    else if (firstStr.Type.Bits == BFBITS16)
    {
        OutCode.PushBack(StrAppend(StrAppend(output, StrFromCStr("ax, ")), secondStr.Asm));
        if (MovToFirst) OutCode.PushBack(StrAppend(StrAppend(StrFromCStr("mov "), firstStr.Asm), StrFromCStr(", ax"))); 
        if (optOutput.size != 0) OutCode.PushBack(optOutput);
        else OutCode.PushBack(StrFromCStr("mov bx, ax"));
        return { StrFromCStr("bx"), firstStr.Type };
    }
    else if (firstStr.Type.Bits == BFBITS32)
    {
        OutCode.PushBack(StrAppend(StrAppend(output, StrFromCStr("eax, ")), secondStr.Asm));
        if (MovToFirst) OutCode.PushBack(StrAppend(StrAppend(StrFromCStr("mov "), firstStr.Asm), StrFromCStr(", eax"))); 
        if (optOutput.size != 0) OutCode.PushBack(optOutput);
        else OutCode.PushBack(StrFromCStr("mov ebx, eax"));
        return { StrFromCStr("ebx"), firstStr.Type };
    }
    return { StrFromCStr(""), Types[0] };
}

int BFExecuteFunc(bf_function* Func)
{
    for (int I = 0; I < Func->RootScope.Lines.size; I++)
    {
        int R = BFExecuteToken(Func->RootScope.Lines[I]);

        if (I == Func->RootScope.Lines.size - 1)
        {
            return R;
        }
    }
}

bf_variable* BFGetVarFromStr(String S, bf_scope* Scope)
{
    for (int I = 0;I < Scope->VarsCount;I++)
    {
        String curS = StrFromArray((char*)Scope->Vars[I]->Name.Name, Scope->Vars[I]->Name.Len);
        if (StrEqualsWith(S, curS))
        {
            return Scope->Vars[I];
        }
    }
    if (Scope->Parent != 0) return BFGetVarFromStr(S, Scope->Parent);
    return NULL;
}

void BFCompileDecl(bf_type VarType, String CurVariableName, LinkedList<String> &OutCode)
{
    if (CurVariableName[0] != '.') CurVariableName.PushFront('.');
    if (VarType.IsStruct == false) 
    {
        if (VarType.Bits == BFBITS8) OutCode.PushBack(StrAppend(CurVariableName, StrFromCStr(" db 0")));
        else if (VarType.Bits == BFBITS16) OutCode.PushBack(StrAppend(CurVariableName, StrFromCStr(" dw 0")));
        else if (VarType.Bits == BFBITS32) OutCode.PushBack(StrAppend(CurVariableName, StrFromCStr(" dd 0")));
        
    }
    else
    {
        OutCode.PushBack(StrAppend(CurVariableName, StrFromCStr(":")));
        for (int I = 0;I < VarType.StructMembers.size;I++)
        {
            String NextVarName;
            NextVarName = StrAppend(NextVarName, CurVariableName);
            NextVarName.PushBack('.');
            NextVarName = StrAppend(NextVarName, VarType.StructMembers[I].Name);
            BFCompileDecl(VarType.StructMembers[I].Type, NextVarName, OutCode);
        }
    }
}

void BFCompileFunc(bf_function* Func, LinkedList<String> &OutCode)
{
    String label;
    for (int i = 0;i < Func->Name.Len;i++)
    {
        label.PushBack(Func->Name.Name[i]);
    }
    String labelGlobal = StrFromCStr("global ");
    labelGlobal = StrAppend(labelGlobal, label);
    OutCode.PushBack(labelGlobal);
    label.PushBack(':');
    LinkedList<String> LocalCode;
    OutCode.PushBack(StrFromCStr("section .text"));
    OutCode.PushBack(label);
    LinkedList<String> Variables;
    for (int i = 0;i < Func->Params.size;i++)
    {
        Variables.PushBack(Func->Params[i]);
    }
    FuncReturnType = Func->Type;
    int tempVarCount = 0;
    for (int I = 0; I < Func->RootScope.Lines.size; I++)
    {
        int regCount = 0;
        BFCompileToken(Func->RootScope.Lines[I], LocalCode, Variables, StrFromCStr(""));
    }
    for (int i = 0;i < LocalCode.size;i++)
    {
        OutCode.PushBack(LocalCode[i]);
    }
    if (!StrEqualsWith(label, StrFromCStr("_start:"))) OutCode.PushBack(StrFromCStr("ret"));
    else
    {
        OutCode.PushBack(StrFromCStr("mov ebx, eax"));
        OutCode.PushBack(StrFromCStr("mov eax, 1"));
        OutCode.PushBack(StrFromCStr("int 0x80"));
    }
    OutCode.PushBack(StrFromCStr("section .data"));
    for (int I = 0;I < Variables.size;I++)
    {
        bf_variable* CurVariable = BFGetVarFromStr(Variables[I], &Func->RootScope);
        if (CurVariable) BFCompileDecl(CurVariable->Type, Variables[I], OutCode);
        else
        {
            if (Variables[I][0] != '.') Variables[I].PushFront('.');
            
            OutCode.PushBack(StrAppend(Variables[I], StrFromCStr(" dd 0")));
        }
    }
}

int BFRun(bf_function** Funcs)
{
    bf_name MainName;
    MainName.Len = 4;
    MainName.Name = (uint8_t*)kmalloc(MainName.Len);
    const char* SMainName = "Main";
    kmemcpy(MainName.Name, SMainName, 4);
    while (*Funcs)
    {
        if (BFCmpName((*Funcs)->Name, MainName))
        {
            return BFExecuteFunc(*Funcs);
        }
        Funcs++;
    }
    return -1;
}

LinkedList<String> BFCompile(bf_function** Funcs)
{
    ConditionalCount = 0;
    LinkedList<String> OutCode;
    OutCode.PushBack(StrFromCStr("[BITS 64]"));
    while (*Funcs)
    {
        BFCompileFunc(*Funcs, OutCode);
        Funcs++;
    }
    OutCode.PushBack(StrFromCStr("__print_c: db 0"));
    return OutCode;
}

int BFRunSource(String Code)
{
    char* Sanitized = (char*)kmalloc(Code.size);
    kmemset(Sanitized, 0, Code.size);
    size_t SanitizedSize = 0;
    for (int I = 0;I < Code.size;I++)
    {
        if (Code[I] != ' ' && Code[I] != '\t' && Code[I] != '\n')
        {
            Sanitized[SanitizedSize++] = Code[I];
        }
    }


    bf_function** Funcs = BFTokenize((uint8_t*)Sanitized, SanitizedSize);
    return BFRun(Funcs);
}

LinkedList<String> BFCodeGen(String Code)
{
    char* Sanitized = (char*)kmalloc(Code.size);
    kmemset(Sanitized, 0, Code.size);
    size_t SanitizedSize = 0;
    for (int I = 0;I < Code.size;I++)
    {
        if (Code[I] != ' ' && Code[I] != '\t' && Code[I] != '\n')
        {
            Sanitized[SanitizedSize++] = Code[I];
        }
    }

    bf_function** Funcs = BFTokenize((uint8_t*)Sanitized, SanitizedSize);
    return BFCompile(Funcs);
}

int main()
{
    String input;
    std::ifstream in("test.bf");
    std::strstream instream;
    instream << in.rdbuf();
    std::string instring = instream.str();

    for (char c : instring)
    {
        input.PushBack(c);
    }

    LinkedList<String> out = BFCodeGen(input);

    for (int j = 0;j < out.size;j++)
    {
        for (int i = 0;i < out[j].size;i++)
        {
            printf("%c", out[j][i]);
        }
        printf("\n");
    }

    return 0;
}