/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include <iostream>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "errors.h"

void EmptyExpr::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
}

void EmptyExpr::Check(checkT c) {
    if (c == E_CheckType) {
        expr_type = Type::voidType;
    }
}

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}
void IntConstant::PrintChildren(int indentLevel) {
    printf("%d", value);
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
}

void IntConstant::Check(checkT c) {
    if (c == E_CheckDecl) {
        expr_type = Type::intType;
    }
}

void IntConstant::Emit() {
    emit_loc = CG->GenLoadConstant(value);
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}
void DoubleConstant::PrintChildren(int indentLevel) {
    printf("%g", value);
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
}

void DoubleConstant::Check(checkT c) {
    if (c == E_CheckDecl) {
        expr_type = Type::doubleType;
    }
}

void DoubleConstant::Emit() {
    ReportError::Formatted(this->GetLocation(),
            "Double is not supported by compiler back end yet.");
    Assert(0);
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}
void BoolConstant::PrintChildren(int indentLevel) {
    printf("%s", value ? "true" : "false");
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
}

void BoolConstant::Check(checkT c) {
    if (c == E_CheckDecl) {
        expr_type = Type::boolType;
    }
}

void BoolConstant::Emit() {
    emit_loc = CG->GenLoadConstant(value ? 1 : 0);
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}
void StringConstant::PrintChildren(int indentLevel) {
    printf("%s",value);
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
}

void StringConstant::Check(checkT c) {
    if (c == E_CheckDecl) {
        expr_type = Type::stringType;
    }
}

void StringConstant::Emit() {
    emit_loc = CG->GenLoadConstant(value);
}

void NullConstant::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
}

void NullConstant::Check(checkT c) {
    if (c == E_CheckDecl) {
        expr_type = Type::nullType;
    }
}

void NullConstant::Emit() {
    emit_loc = CG->GenLoadConstant(0);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

void Operator::PrintChildren(int indentLevel) {
    printf("%s",tokenString);
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r)
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this);
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r)
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL;
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

void CompoundExpr::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    if (left) left->Print(indentLevel+1);
    op->Print(indentLevel+1);
    right->Print(indentLevel+1);
}

void ArithmeticExpr::CheckType() {
    if (left) left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    if (!strcmp(op->GetOpStr(), "-") && !left) {
        Type *tr = right->GetType();
        if (tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tr == Type::intType) {
            expr_type = Type::intType;
        } else if (tr == Type::doubleType) {
            expr_type = Type::doubleType;
        } else {
            ReportError::IncompatibleOperand(op, tr);
        }
    } else { // for + - * / %
        Type *tl = left->GetType();
        Type *tr = right->GetType();
        if (tl == NULL || tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tl == Type::intType && tr == Type::intType) {
            expr_type = Type::intType;
        } else if ((tl == Type::doubleType && tr == Type::doubleType)
                // && strcmp(op->GetOpStr(), "%") // % can apply to float.
                ) {
            expr_type = Type::doubleType;
        } else {
            ReportError::IncompatibleOperands(op, tl, tr);
        }
    }
}

void ArithmeticExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void ArithmeticExpr::Emit() {
    if (left) left->Emit();
    right->Emit();

    Location *l = left ? left->GetEmitLocDeref() : CG->GenLoadConstant(0);
    emit_loc = CG->GenBinaryOp(op->GetOpStr(), l, right->GetEmitLocDeref());
}

void RelationalExpr::CheckType() {
    left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    // the type of RelationalExpr is always boolType.
    expr_type = Type::boolType;

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == NULL || tr == NULL) {
        // some error accur in left or right, so skip it.
        return;
    }

    if (!(tl == Type::intType && tr == Type::intType) &&
            !(tl == Type::doubleType && tr == Type::doubleType)) {
        ReportError::IncompatibleOperands(op, tl, tr);
    }
}

void RelationalExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void RelationalExpr::Emit() {
    left->Emit();
    right->Emit();

    emit_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
            right->GetEmitLocDeref());
}

void EqualityExpr::CheckType() {
    left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    // the type of EqualityExpr is always boolType.
    expr_type = Type::boolType;

    if (tl == NULL || tr == NULL) {
        // some error accur in left or right, so skip it.
        return;
    }

    if (!tr->IsCompatibleWith(tl) && !tl->IsCompatibleWith(tr)) {
        ReportError::IncompatibleOperands(op, tl, tr);
    }
}

void EqualityExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void EqualityExpr::Emit() {
    left->Emit();
    right->Emit();

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == tr && (tl == Type::intType || tl == Type::boolType)) {
        emit_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                right->GetEmitLocDeref());
    } else if (tl == tr && tl == Type::stringType) {
        emit_loc = CG->GenBuiltInCall(StringEqual, left->GetEmitLocDeref(),
                right->GetEmitLocDeref());
        if (!strcmp(op->GetOpStr(), "!=")) {
            // for s1 != s2, generate s1 == s2, then generate logical not.
            emit_loc = CG->GenBinaryOp("==", CG->GenLoadConstant(0),
                emit_loc);
        }
    } else {
        // array? class? interface?
        // just compare the reference.
        emit_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                right->GetEmitLocDeref());
    }
}

void LogicalExpr::CheckType() {
    if (left) left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    // the type of LogicalExpr is always boolType.
    expr_type = Type::boolType;

    if (!strcmp(op->GetOpStr(), "!")) {
        Type *tr = right->GetType();
        if (tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tr != Type::boolType) {
            ReportError::IncompatibleOperand(op, tr);
        }
    } else { // for && and ||
        Type *tl = left->GetType();
        Type *tr = right->GetType();
        if (tl == NULL || tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tl != Type::boolType || tr != Type::boolType) {
            ReportError::IncompatibleOperands(op, tl, tr);
        }
    }
}

void LogicalExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void LogicalExpr::Emit() {
    if (left) left->Emit();
    right->Emit();

    if (left) {
        emit_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                right->GetEmitLocDeref());
    } else {
        // use 0 == bool_var to compute !bool_var.
        emit_loc = CG->GenBinaryOp("==", CG->GenLoadConstant(0),
                right->GetEmitLocDeref());
    }
}

void AssignExpr::CheckType() {
    left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == NULL || tr == NULL) {
        // some error accur in left or right, so skip it.
        return;
    }

    if (!tl->IsCompatibleWith(tr)) {
        ReportError::IncompatibleOperands(op, tl, tr);
    }
}

void AssignExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void AssignExpr::Emit() {
    right->Emit();
    left->Emit();
    Location *r = right->GetEmitLocDeref();
    Location *l = left->GetEmitLoc();
    if (r && l) {
        // base can be this or class instances.
        if (l->GetBase() != NULL) {
            CG->GenStore(l->GetBase(), r, l->GetOffset());
        } else if (left->IsArrayAccessRef()) {
            CG->GenStore(l, r);
        } else {
            CG->GenAssign(l, r);
        }
        emit_loc = left->GetEmitLocDeref();
    }
}

void This::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
}

void This::CheckType() {
    Decl *d = symtab->LookupThis();
    if (!d || !d->IsClassDecl()) {
        ReportError::ThisOutsideClassScope(this);
    } else {
        // Note: here create a new NamedType for 'this'.
        expr_type = new NamedType(d->GetId());
        expr_type->SetSelfType();
    }
}

void This::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    }
}

void This::Emit() {
    emit_loc = CG->ThisPtr;
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this);
    (subscript=s)->SetParent(this);
}

void ArrayAccess::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    base->Print(indentLevel+1);
    subscript->Print(indentLevel+1, "(subscript) ");
}

void ArrayAccess::CheckType() {
    Type *t;
    int err = 0;

    subscript->Check(E_CheckType);
    t = subscript->GetType();
    if (t == NULL) {
        // some error accur in subscript, so skip it.
    } else if (t != Type::intType) {
        ReportError::SubscriptNotInteger(subscript);
    }

    base->Check(E_CheckType);
    t = base->GetType();
    if (t == NULL) {
        // some error accur in base, so skip it.
        err++;
    } else if (!t->IsArrayType()) {
        ReportError::BracketsOnNonArray(base);
        err++;
    }

    if (!err) {
        // the error of subscript will not affect the type of array access.
        expr_type = (dynamic_cast<ArrayType*>(t))->GetElemType();
    }
}

void ArrayAccess::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        base->Check(c);
        subscript->Check(c);
    }
}

void ArrayAccess::Emit() {
    base->Emit();
    subscript->Emit();
    Location *t0 = subscript->GetEmitLocDeref();

    Location *t1 = CG->GenLoadConstant(0);
    Location *t2 = CG->GenBinaryOp("<", t0, t1);
    Location *t3 = base->GetEmitLocDeref();
    Location *t4 = CG->GenLoad(t3, -4);
    Location *t5 = CG->GenBinaryOp("<", t0, t4);
    Location *t6 = CG->GenBinaryOp("==", t5, t1);
    Location *t7 = CG->GenBinaryOp("||", t2, t6);
    const char *l = CG->NewLabel();
    CG->GenIfZ(t7, l);
    Location *t8 = CG->GenLoadConstant(err_arr_out_of_bounds);
    CG->GenBuiltInCall(PrintString, t8);
    CG->GenBuiltInCall(Halt);
    CG->GenLabel(l);

    Location *t9 = CG->GenLoadConstant(expr_type->GetTypeSize());
    Location *t10 = CG->GenBinaryOp("*", t9, t0);
    Location *t11 = CG->GenBinaryOp("+", t3, t10);
    emit_loc = t11;
}

Location * ArrayAccess::GetEmitLocDeref() {
    Location *t = CG->GenLoad(emit_loc, 0);
    return t;
}

FieldAccess::FieldAccess(Expr *b, Identifier *f)
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
}

void FieldAccess::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    if (base) base->Print(indentLevel+1);
    field->Print(indentLevel+1);
}

void FieldAccess::CheckDecl() {
    if (!base) {
        Decl *d = symtab->Lookup(field);
        if (d == NULL) {
            ReportError::IdentifierNotDeclared(field, LookingForVariable);
            return;
        } else {
            field->SetDecl(d);
        }
    } else {
        // if has base, then leave the work to CheckType.
        base->Check(E_CheckDecl);
    }
}

void FieldAccess::CheckType() {
    if (!base) {
        if (field->GetDecl()) {
            if (field->GetDecl()->IsVarDecl()) {
                expr_type = field->GetDecl()->GetType();
            } else {
                ReportError::IdentifierNotDeclared(field, LookingForVariable);
            }
        }
        return;
    }

    // must check the base expr's class type, and find in that class.
    base->Check(E_CheckType);
    Type *base_t = base->GetType();
    if (base_t != NULL) {
        if (!base_t->IsNamedType()) {
            ReportError::FieldNotFoundInBase(field, base_t);
            return;
        }

        Decl *d = symtab->LookupField(
                dynamic_cast<NamedType*>(base_t)->GetId(), field);

        if (d == NULL || !d->IsVarDecl()) {
            ReportError::FieldNotFoundInBase(field, base_t);
        } else {
            // If base is 'this' or any instances of current class,
            // then all the private variable members are accessible.
            // Otherwise the variable members are not accessible.
            // Note: If base is a subclass of current class, and the
            // field is belong to current class, then this field is
            // accessible.
            Decl *cur_class = symtab->LookupThis();
            if (!cur_class || !cur_class->IsClassDecl()) {
                // not in a class scope, all the variable members are
                // not accessible.
                ReportError::InaccessibleField(field, base_t);
                return;
            }
            // in a class scope, the variable members can be
            // accessed by 'this' or the compatible class instance.
            Type *cur_t = cur_class->GetType();
            d = symtab->LookupField(
                    dynamic_cast<NamedType*>(cur_t)->GetId(), field);

            if (d == NULL || !d->IsVarDecl()) {
                ReportError::FieldNotFoundInBase(field, cur_t);
                return;
            }
            if (cur_t->IsCompatibleWith(base_t) ||
                    base_t->IsCompatibleWith(cur_t)) {
                field->SetDecl(d);
                expr_type = d->GetType();
            } else {
                ReportError::InaccessibleField(field, base_t);
            }
        }
    }
}

void FieldAccess::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckType:
            this->CheckType(); break;
        default:;
            // do not check anything.
    }
}

void FieldAccess::Emit() {
    if (base) base->Emit();
    field->Emit();
    emit_loc = field->GetEmitLocDeref();

    // can access a var member in a class scope, so set the base.
    if (base)
        emit_loc = new Location(fpRelative, emit_loc->GetOffset(),
                emit_loc->GetName(), base->GetEmitLocDeref());
}

Location * FieldAccess::GetEmitLocDeref() {
    Location *t = emit_loc;
    if (t->GetBase() != NULL) {
        // this or some class instances.
        t = CG->GenLoad(t->GetBase(), t->GetOffset());
    }
    return t;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    if (base) base->Print(indentLevel+1);
    field->Print(indentLevel+1);
    actuals->PrintAll(indentLevel+1, "(actuals) ");
}

void Call::CheckDecl() {
    if (!base) {
        Decl *d = symtab->Lookup(field);
        if (d == NULL || !d->IsFnDecl()) {
            ReportError::IdentifierNotDeclared(field, LookingForFunction);
            return;
        } else {
            field->SetDecl(d);
            // if function is defined after this, here expr_type is NULL.
            expr_type = d->GetType();
        }
    } else {
        // if has base, then leave the work to CheckType.
        base->Check(E_CheckDecl);
    }
    actuals->CheckAll(E_CheckDecl);
}

void Call::CheckType() {
    if (!base) {
        if (field->GetDecl() && !expr_type) {
            // if function is defined after this, then assign expr_type again.
            expr_type = field->GetDecl()->GetType();
        }
    } else {
        // must check the base expr's class type, and find in that class.
        base->Check(E_CheckType);
        Type * t = base->GetType();
        if (t != NULL) { // base defined.
            if (t->IsArrayType() && !strcmp(field->GetIdName(), "length")) {
                // support the length() method of array type.
                // length must have no argument.
                int n = actuals->NumElements();
                if (n) {
                    ReportError::NumArgsMismatch(field, 0, n);
                }
                expr_type = Type::intType;
            } else if (!t->IsNamedType()) {
                ReportError::FieldNotFoundInBase(field, t);
            } else {
                Decl *d = symtab->LookupField(
                        dynamic_cast<NamedType*>(t)->GetId(), field);
                if (d == NULL || !d->IsFnDecl()) {
                    ReportError::FieldNotFoundInBase(field, t);
                } else {
                    field->SetDecl(d);
                    expr_type = d->GetType();
                }
            }
        }
    }
    actuals->CheckAll(E_CheckType);
    this->CheckFuncArgs();
}

void Call::CheckFuncArgs() {
    Decl *f = field->GetDecl();
    if (!f || !f->IsFnDecl()) return; // skip the error decl
    FnDecl *fun = dynamic_cast<FnDecl*>(f);
    List<VarDecl*> *formals = fun->GetFormals();

    int n_expected = formals->NumElements();
    int n_given = actuals->NumElements();
    if (n_given != n_expected) {
        ReportError::NumArgsMismatch(field, n_expected, n_given);
    } else {
        for (int i = 0; i < actuals->NumElements(); i++) {
            Type *t_a = actuals->Nth(i)->GetType();
            Type *t_f = formals->Nth(i)->GetType();

            if (t_a && t_f && !t_f->IsCompatibleWith(t_a)) {
                ReportError::ArgMismatch(actuals->Nth(i), i + 1, t_a, t_f);
            }
        }
    }
}

void Call::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckType:
            this->CheckType(); break;
        default:;
            // do not check anything.
    }
}

void Call::Emit() {
    PrintDebug("tac+", "Emit Call %s.", field->GetIdName());
    // TODO: in class scope, methon without base should be ACall.

    if (base) base->Emit();
    field->Emit();
    actuals->EmitAll();

    // deal with array.length().
    if (base && base->GetType()->IsArrayType() &&
            !strcmp(field->GetIdName(), "length")) {
        Location *t0 = base->GetEmitLocDeref();
        Location *t1 = CG->GenLoad(t0, -4);
        emit_loc = t1;
        return;
    }

    FnDecl *fn = dynamic_cast<FnDecl*>(field->GetDecl());
    Assert(fn);
    bool is_ACall = (base != NULL) || (fn->IsClassMember());

    // get VTable entry.
    Location *this_loc;
    if (base) {
        this_loc = base->GetEmitLocDeref(); // VTable entry.
    } else if (fn->IsClassMember()) {
        this_loc = CG->ThisPtr; // in a class scope.
    }

    Location *t;
    if (is_ACall) {
        t = CG->GenLoad(this_loc, 0);
        t = CG->GenLoad(t, fn->GetVTableOffset());
    }

    // PushParam
    for (int i = actuals->NumElements() - 1; i >= 0; i--) {
        Location *l = actuals->Nth(i)->GetEmitLocDeref();
        CG->GenPushParam(l);
    }

    // generate call.
    if (is_ACall) {
        // Push this.
        CG->GenPushParam(this_loc);
        // ACall
        emit_loc = CG->GenACall(t, fn->HasReturnValue());
        // PopParams
        CG->GenPopParams(actuals->NumElements() * 4 + 4);
    } else {
        // LCall
        field->AddPrefix("_"); // main?
        emit_loc = CG->GenLCall(field->GetIdName(),
                expr_type != Type::voidType);
        // PopParams
        CG->GenPopParams(actuals->NumElements() * 4);
    }
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) {
    Assert(c != NULL);
    (cType=c)->SetParent(this);
}

void NewExpr::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    cType->Print(indentLevel+1);
}

void NewExpr::CheckDecl() {
    cType->Check(E_CheckDecl, LookingForClass);
}

void NewExpr::CheckType() {
    cType->Check(E_CheckType);
    // cType is NamedType.
    if (cType->GetType()) { // correct cType
        expr_type = cType;
    }
}

void NewExpr::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckType:
            this->CheckType(); break;
        default:
            cType->Check(c);
    }
}

void NewExpr::Emit() {
    ClassDecl *d = dynamic_cast<ClassDecl*>(cType->GetId()->GetDecl());
    Assert(d);
    int size = d->GetInstanceSize();
    Location *t = CG->GenLoadConstant(size);
    emit_loc = CG->GenBuiltInCall(Alloc, t);
    Location *l = CG->GenLoadLabel(d->GetId()->GetIdName());
    CG->GenStore(emit_loc, l, 0);
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this);
    (elemType=et)->SetParent(this);
}

void NewArrayExpr::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    size->Print(indentLevel+1);
    elemType->Print(indentLevel+1);
}

void NewArrayExpr::CheckType() {
    Type *t;

    size->Check(E_CheckType);
    t = size->GetType();
    if (t == NULL) {
        // some error accur in size, so skip it.
    } else if (t != Type::intType) {
        ReportError::NewArraySizeNotInteger(size);
    }

    elemType->Check(E_CheckType);
    if (!elemType->GetType()) {
        // skip the error elemType.
        return;
    } else {
        // the error size will not affect the type of new array.
        expr_type = new ArrayType(*location, elemType);
        expr_type->Check(E_CheckDecl);
    }
}

void NewArrayExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        size->Check(c);
        elemType->Check(c);
    }
}

void NewArrayExpr::Emit() {
    size->Emit();
    Location *t0 = size->GetEmitLocDeref();
    Location *t1 = CG->GenLoadConstant(0);
    Location *t2 = CG->GenBinaryOp("<=", t0, t1);

    const char *l = CG->NewLabel();
    CG->GenIfZ(t2, l);
    Location *t3 = CG->GenLoadConstant(err_arr_bad_size);
    CG->GenBuiltInCall(PrintString, t3);
    CG->GenBuiltInCall(Halt);

    CG->GenLabel(l);
    Location *t4 = CG->GenLoadConstant(1);
    Location *t5 = CG->GenBinaryOp("+", t4, t0);
    Location *t6 = CG->GenLoadConstant(elemType->GetTypeSize());
    Location *t7 = CG->GenBinaryOp("*", t5, t6);
    Location *t8 = CG->GenBuiltInCall(Alloc, t7);
    CG->GenStore(t8, t0);
    Location *t9 = CG->GenBinaryOp("+", t8, t6);
    emit_loc = t9;
}

void ReadIntegerExpr::Check(checkT c) {
    if (c == E_CheckType) {
        expr_type = Type::intType;
    }
}

void ReadIntegerExpr::Emit() {
    emit_loc = CG->GenBuiltInCall(ReadInteger);
}

void ReadLineExpr::Check(checkT c) {
    if (c == E_CheckType) {
        expr_type = Type::stringType;
    }
}

void ReadLineExpr::Emit() {
    emit_loc = CG->GenBuiltInCall(ReadLine);
}

PostfixExpr::PostfixExpr(LValue *lv, Operator *o)
    : Expr(Join(lv->GetLocation(), o->GetLocation())) {
    Assert(lv != NULL && o != NULL);
    (lvalue=lv)->SetParent(this);
    (op=o)->SetParent(this);
}

void PostfixExpr::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    lvalue->Print(indentLevel+1);
    op->Print(indentLevel+1);
}

void PostfixExpr::CheckType() {
    lvalue->Check(E_CheckType);
    op->Check(E_CheckType);
    Type *t = lvalue->GetType();
    if (t == NULL) {
        // some error accur in lvalue, so skip it.
    } else if (t != Type::intType) {
        ReportError::IncompatibleOperand(op, t);
    } else {
        expr_type = t;
    }
}

void PostfixExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        lvalue->Check(c);
        op->Check(c);
    }
}

void PostfixExpr::Emit() {
    lvalue->Emit();
    // lvalue can be class var member, array access, or any variables.
    Location *l1 = lvalue->GetEmitLoc();
    Location *l2 = lvalue->GetEmitLocDeref();

    // save the original lvalue.
    Location *t0 = CG->GenTempVar();
    CG->GenAssign(t0, l2);

    // postfix expr should emit ++ or -- at the end of itself.
    l2 = CG->GenBinaryOp(strcmp(op->GetOpStr(), "++") ? "-" : "+",
            l2, CG->GenLoadConstant(1));

    // change the value of lvalue.
    if (l1->GetBase() != NULL) {
        CG->GenStore(l1->GetBase(), l2, l1->GetOffset());
    } else if (lvalue->IsArrayAccessRef()) {
        CG->GenStore(l1, l2);
    } else {
        CG->GenAssign(l1, l2);
    }

    // the value of postfix expr is its original lvalue.
    emit_loc = t0;
}

