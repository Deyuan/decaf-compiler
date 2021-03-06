/* File: ast_type.h
 * ----------------
 * In our parse tree, Type nodes are used to represent and
 * store type information. The base Type class is used
 * for built-in types, the NamedType for classes and interfaces,
 * and the ArrayType for arrays of other types.
 *
 * pp3: You will need to extend the Type classes to implement
 * the type system and rules for type equivalency and compatibility.
 *
 * pp5: You will need to extend the Type classes to implement
 * code generation for types.
 *
 * Author: Deyuan Guo
 */

#ifndef _H_ast_type
#define _H_ast_type

#include "ast.h"
#include "list.h"
#include <iostream>


class Type : public Node
{
  protected:
    char *typeName;

  public :
    static Type *intType, *doubleType, *boolType, *voidType,
                *nullType, *stringType, *errorType;

    Type(yyltype loc) : Node(loc) { expr_type = NULL; }
    Type(const char *str);

    const char *GetPrintNameForNode() { return "Type"; }
    void PrintChildren(int indentLevel);

    virtual void PrintToStream(std::ostream& out) { out << typeName; }
    friend std::ostream& operator<<(std::ostream& out, Type *t)
        { t->PrintToStream(out); return out; }
    virtual bool IsEquivalentTo(Type *other) { return this == other; }
    virtual bool IsCompatibleWith(Type *other) { return this == other; }

    char * GetTypeName() { return typeName; }

    virtual bool IsBasicType()
        { return !this->IsNamedType() && !this->IsArrayType(); }
    virtual bool IsNamedType() { return false; }
    virtual bool IsArrayType() { return false; }
    void Check(checkT c);
    virtual void Check(checkT c, reasonT r) { Check(c); }
    virtual void SetSelfType() { expr_type = this; }

    // code generation
    virtual int GetTypeSize() { return 4; }
};

class NamedType : public Type
{
  protected:
    Identifier *id;
    void CheckDecl(reasonT r);

  public:
    NamedType(Identifier *i);

    const char *GetPrintNameForNode() { return "NamedType"; }
    void PrintChildren(int indentLevel);

    void PrintToStream(std::ostream& out) { out << id; }

    Identifier *GetId() { return id; }

    bool IsEquivalentTo(Type *other);
    bool IsCompatibleWith(Type *other);

    bool IsNamedType() { return true; }
    void Check(checkT c, reasonT r);
    void Check(checkT c) { Check(c, LookingForType); }
};

class ArrayType : public Type
{
  protected:
    Type *elemType;
    void CheckDecl();

  public:
    ArrayType(yyltype loc, Type *elemType);

    const char *GetPrintNameForNode() { return "ArrayType"; }
    void PrintChildren(int indentLevel);

    void PrintToStream(std::ostream& out) { out << elemType << "[]"; }

    Type * GetElemType() { return elemType; }
    bool IsEquivalentTo(Type *other);
    bool IsCompatibleWith(Type *other);

    bool IsArrayType() { return true; }
    void Check(checkT c);
};


#endif

