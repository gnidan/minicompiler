/*
 * file:  programext.cpp (implementation of Program classes and interpreter,
 * with function support).
 * Author: Jeremy Johnson 
 * Date: 2/5/07
 *
 * Modified by: G. Nicholas D'Andrea
 * Date: 3/05/08
 * Added: Compilation to Random Access Language
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <list>
#include "programext.h"
#include "ralprogram.h"

using namespace std;

void fillIn(FunctionGap *g, RALFunction *func, Env &e)
{
  /* Fill in the function gap with the appropriate things in the function.
   * Pretty simple, ja? */

  int ARsize = func->getActivationRecord().size();
  g->ADDwithActivationRecordSize->setArgument(
      (void *) getConstant(e.constants, ARsize)
      );

  g->LDOwithReturnValueOffset->setOffset(func->ret_value, e.constants);

  g->LDOwithPrevFPOffset->setOffset(func->prev_fp, e.constants);

  g->STOwithReturnAddressOffset->setOffset(func->ret_addr, e.constants);

  g->STOwithPrevFPOffset->setOffset(func->prev_fp, e.constants);

  g->JMPtoFunction->setArgument((void *) func->getFirstLabel());

  list<STO*>::iterator sto_it;
  list<MemoryLocation*>::iterator memloc_it;
  for(sto_it = g->params.begin(), memloc_it = func->parameters.begin();
      sto_it != g->params.end(),  memloc_it != func->parameters.end();
      sto_it++, memloc_it++)
  {
    (*sto_it)->setOffset(*memloc_it, e.constants);
  }
}

MemoryLocation *getConstant(vector<MemoryLocation *> &constants, int value)
{
  vector<MemoryLocation*>::iterator it;
  for(it = constants.begin(); it != constants.end(); it++)
    if((*it)->type == CONST && (*it)->value == value)
      return *it;

  MemoryLocation *r = new MemoryLocation();
  r->value = value;
  r->type = CONST;
  constants.push_back(r);

  return r;
}

MemoryLocation *getConstant(vector<MemoryLocation *> &constants, Label *value)
{
  vector<MemoryLocation*>::iterator it;
  for(it = constants.begin(); it != constants.end(); it++)
    if((*it)->type == RETURN_ADDRESS && (*it)->label == value)
      return *it;
  
  MemoryLocation *r = new MemoryLocation();
  r->label = value;
  r->type = RETURN_ADDRESS;
  constants.push_back(r);

  return r;
}

MemoryLocation *getConstant(vector<MemoryLocation*> &constants, 
    MemoryLocation *value)
{
  vector<MemoryLocation*>::iterator it;
  for(it = constants.begin(); it != constants.end(); it++)
    if((*it)->type == POINTER && (*it)->location == value)
      return *it;

  MemoryLocation *r = new MemoryLocation();
  r->location = value;
  r->type = POINTER;
  constants.push_back(r);

  return r;
}

Program::Program(StmtList *SL)
{
NameTable_.clear();
FunctionTable_.clear();
SL_ = SL;
}

void Program::dump() 
{
  map<string,int>::iterator p; map<string,Proc*>::iterator f;

  cout << "Dump of Symbol Table" << endl;
  cout << "Name Table" << endl;
  for (p = NameTable_.begin();p != NameTable_.end();p++)
    cout << p->first << " -> " << p->second << endl;
  cout << "Function Table" << endl;
  for (f = FunctionTable_.begin();f != FunctionTable_.end();f++) 
    cout << f->first << endl;
}

void Program::eval() 
{
	SL_->eval(NameTable_, FunctionTable_);
}

RALProgram *Program::compile()
{
  Env e;

  e.fp = new MemoryLocation;
  e.fp->type = POINTER;
  
  e.sp = new MemoryLocation;
  e.sp->type = POINTER;

  e.scratch = new MemoryLocation;
  e.scratch->type = POINTER;
  
  e.scratch2 = new MemoryLocation();
  e.scratch2->type = POINTER;

  e.prev_fp = new MemoryLocation();
  e.prev_fp->type = POINTER;
  
  list<string> *t = new list<string>;
  Proc *main = new Proc(t, SL_);

  /* This is blank so to ensure it's a unique identifier */
  string mainFunction = "";

  RALFunction *f = main->compile(e);

  e.functions[mainFunction] = f;

  delete t;
  delete main;

  RALProgram *r = new RALProgram(e);
  return r;
}

void StmtList::insert(Stmt * S)
{
  SL_.push_front(S);
}

void StmtList::eval(map<string,int> &NT, map<string,Proc*> &FT) 
{
  list<Stmt*>::iterator Sp;
  for (Sp = SL_.begin();Sp != SL_.end();Sp++)
	(*Sp)->eval(NT,FT);
}

RALStmtList *StmtList::compile(Env &e,
                               map<string, MemoryLocation*> &variables,
                               vector<MemoryLocation*> &temps)
{
  RALStmtList *l = new RALStmtList();
  list<RALStmtList*> statements;
  
  list<Stmt*>::iterator it;
  for(it = SL_.begin(); it != SL_.end(); it++)
  {
    RALStmtList *s = (*it)->compile(e, variables, temps);
    if(s != NULL)
    {
      l->append(s);
      statements.push_back(s);
    }
  }

  list<RALStmtList*>::iterator jt;
  for(jt = statements.begin(); jt != statements.end(); jt++)
  {
    if(++jt != statements.end())
    {
      Label *label = (*jt)->getFirstLabel();
      --jt;
      (*jt)->replaceNULLsWith(label);
    }
    else
    {
      --jt;
    }
  }

  return l;
}

AssignStmt::AssignStmt(string name, Expr *E)
{
  name_ = name;
  E_ = E;
}

void AssignStmt::eval(map<string,int> &NT, map<string,Proc*> &FT) const
{
	NT[name_] = E_->eval(NT,FT);
}

RALStmtList *AssignStmt::compile(Env &e,
                                 map<string, MemoryLocation*> &variables,
                                 vector<MemoryLocation*> &temps)
{
  RALStmtList *l = E_->compile(e, variables, temps);
  MemoryLocation *load_from = e.last_written_to;
  MemoryLocation *store_to = variables[name_];

  if(store_to == NULL)
  {
    variables[name_] = new MemoryLocation();
    store_to = variables[name_];
    store_to->type = VARIABLE;
  }

  l->append( new LDO(e.fp, load_from, e) );
  l->append( new STO(e.fp, store_to, e) );
  e.last_written_to = store_to;

  return l;
}

DefineStmt::~DefineStmt()
{
  delete P_; 
}

DefineStmt::DefineStmt(string name, Proc *P)
{
  name_ = name;
  P_ = P;
}

void DefineStmt::eval(map<string,int> &NT, map<string,Proc*> &FT) const
{
	FT[name_] = P_;
}

RALStmtList *DefineStmt::compile(Env &e,
                                 map<string, MemoryLocation*> &variables,
                                 vector<MemoryLocation*> &temps)
{
  /* We've got a name and a proc; We want to compile the proc, then we want
   * to add that function to the e.functions table */
  e.functions[name_] = P_->compile(e);

  /* And now that we have a new function added to the functions table,
   * we might have an incomplete record that we can fill in. Let's do that
   * here */
  list<FunctionGap*>::iterator it;
  for(it = e.toCompile[name_].begin(); it != e.toCompile[name_].end(); it++)
    fillIn(*it, e.functions[name_], e);

  /* This method is going to return null... there's nothing significant in
   * RAL about defining a function that requires statements added into the
   * currently recursed upon statement list.
   * Since DefineStmt is still a subclass of Stmt, we're saying this
   * returns a RALStmtList*... just one that happens to contain no RALStmts */
  return NULL;
}

IfStmt::IfStmt(Expr *E, StmtList *S1, StmtList *S2)
{
  E_ = E;
  S1_ = S1;
  S2_ = S2;
}

IfStmt::~IfStmt() { delete E_; delete S1_; delete S2_; }

void IfStmt::eval(map<string,int> &NT, map<string,Proc*> &FT) const
{
	if (E_->eval(NT,FT) > 0)
		S1_->eval(NT,FT);
	else
		S2_->eval(NT,FT);
}

RALStmtList *IfStmt::compile(Env &e,
                             map<string, MemoryLocation*> &variables,
                             vector<MemoryLocation*> &temps)
{
  RALStmtList *l = E_->compile(e, variables, temps);
  MemoryLocation *load_from = e.last_written_to;

  RALStmtList *s1 = S1_->compile(e, variables, temps);
  RALStmtList *s2 = S2_->compile(e, variables, temps);

  RALStmtList *ldo = new LDO(e.fp, load_from, e);
  RALStmt *jmn = new RALStmt(JMN, s2->getFirstLabel());
  RALStmt *jmz = new RALStmt(JMZ, s2->getFirstLabel());
  RALStmt *jmp = new RALStmt(JMP, NULL);

  l->append(ldo);
  l->append(jmn);
  l->append(jmz);

  s1->replaceNULLsWith(jmp->getLabel());
  l->append(s1);

  l->append(jmp);

  l->append(s2);

  delete s1;
  delete s2;

  return l;
}

WhileStmt::WhileStmt(Expr *E, StmtList *S)
{
  E_ = E;
  S_ = S;
}

WhileStmt::~WhileStmt() { delete E_; delete S_; }

RALStmtList *WhileStmt::compile(Env &e,
                                map<string, MemoryLocation*> &variables,
                                vector<MemoryLocation*> &temps)
{
  RALStmtList *l = E_->compile(e, variables, temps);
  MemoryLocation *load_from = e.last_written_to;

  RALStmtList *s = S_->compile(e, variables, temps);
  
  RALStmtList *ldo = new LDO(e.fp, load_from, e);
  RALStmt *jmn = new RALStmt(JMN, NULL);
  RALStmt *jmz = new RALStmt(JMZ, NULL);
  RALStmt *jmp = new RALStmt(JMP, l->getFirstLabel());

  l->append(ldo);
  l->append(jmn);
  l->append(jmz);
  s->replaceNULLsWith(jmp->getLabel());
  l->append(s);
  l->append(jmp);

  delete s;

  return l;
}

void WhileStmt::eval(map<string,int> &NT, map<string,Proc*> &FT) const
{
	while (E_->eval(NT,FT) > 0) 
		S_->eval(NT,FT);
}

Number::Number(int value)
{
	value_ = value;
}

RALStmtList *Number::compile(Env &e,
                             map<string, MemoryLocation*> &variables,
                             vector<MemoryLocation*> &temps)
{
  /* Set up two memory locations: the constant representing the number
   * and the temporary location to store it */
  MemoryLocation *load_from = getConstant(e.constants, value_),
                 *store_to = new MemoryLocation();

  store_to->type = TEMPORARY;

  RALStmt *sta = new RALStmt(STA, store_to);

  RALStmtList *l = new RALStmtList();
  l->append( new RALStmt(LDA, load_from) );
  l->append( new STO(e.fp, store_to, e) );

  temps.push_back(store_to);
  e.last_written_to = store_to;

  return l;
}

int Number::eval(map<string,int> NT, map<string,Proc*> FT) const
{
	return value_;
}

Ident::Ident(string name)
{
	name_ = name;
}

int Ident::eval(map<string,int> NT, map<string,Proc*> FT) const
{
	return NT[name_];
}

RALStmtList *Ident::compile(Env &e,
                            map<string, MemoryLocation*> &variables,
                            vector<MemoryLocation*> &temps)
{
  MemoryLocation *load_from = variables[name_],
                 *store_to = new MemoryLocation();

  if(load_from == NULL)
  {
    variables[name_] = new MemoryLocation();
    load_from = variables[name_];
    load_from->type = VARIABLE;
  }

  store_to->type = TEMPORARY;

  RALStmtList *l = new RALStmtList();
  l->append( new LDO(e.fp, load_from, e) );
  l->append( new STO(e.fp, store_to, e) );

  temps.push_back(store_to);
  e.last_written_to = store_to;

  return l;
}

Plus::Plus(Expr* op1, Expr* op2)
{
	op1_ = op1;
	op2_ = op2;
}

int Plus::eval(map<string,int> NT, map<string,Proc*> FT) const
{
	return op1_->eval(NT,FT) + op2_->eval(NT,FT);
}

Expr *Plus::simplify()
{
  Expr *r;

  map<string, int> t;
  map<string,Proc*> f;

  if(op1_->isNumber() && op2_->isNumber())
  {
    r = new Number(op1_->eval(t, f) + op2_->eval(t, f));
    delete op1_;
    delete op2_;
  }
  else if(op1_->isNumber() && op1_->eval(t, f) == 0)
  {
    delete op1_;
    r = op2_;
  }
  else if(op2_->isNumber() && op2_->eval(t, f) == 0)
  {
    delete op2_;
    r = op1_;
  }
  else
  {
    return this;
  }

  /* C++ gods please do not smite me for this - I don't want to call the
   * destructor, I just want to free the memory for the current expression. */
  free(this);
  return r;
}

RALStmtList *Plus::compile(Env &e,
                           map<string, MemoryLocation*> &variables, 
                           vector<MemoryLocation*> &temps)
{
  RALStmtList *l1 = op1_->compile(e, variables, temps);
  MemoryLocation *load_from_1 = e.last_written_to;

  RALStmtList *l2 = op2_->compile(e, variables, temps);
  MemoryLocation *load_from_2 = e.last_written_to;

  MemoryLocation *store_to = new MemoryLocation();
  store_to->type = TEMPORARY;

  l1->append(l2);
  delete l2;

  l1->append( new LDO(e.fp, load_from_1, e) );
  l1->append( new RALStmt(STA, e.scratch2) );
  l1->append( new LDO(e.fp, load_from_2, e) );
  l1->append( new RALStmt(ADD, e.scratch2) );
  l1->append( new STO(e.fp, store_to, e) );

  temps.push_back(store_to);
  e.last_written_to = store_to;

  return l1;
}

Minus::Minus(Expr* op1, Expr* op2)
{
	op1_ = op1;
	op2_ = op2;
}

int Minus::eval(map<string,int> NT, map<string,Proc*> FT) const
{
	return op1_->eval(NT,FT) - op2_->eval(NT,FT);
}

Expr *Minus::simplify()
{
  Expr *r;

  map<string, int> t;
  map<string,Proc*> f;

  if(op1_->isNumber() && op2_->isNumber())
  {
    r = new Number(op1_->eval(t, f) - op2_->eval(t, f));
    delete op1_;
    delete op2_;
  }
  else if(op2_->isNumber() && op2_->eval(t, f) == 0)
  {
    delete op2_;
    r = op1_;
  }
  else
  {
    return this;
  }

  free(this);
  return r;
}

RALStmtList *Minus::compile(Env &e,
                            map<string, MemoryLocation*> &variables, 
                            vector<MemoryLocation*> &temps)
{
  RALStmtList *l1 = op1_->compile(e, variables, temps);
  MemoryLocation *load_from_1 = e.last_written_to;

  RALStmtList *l2 = op2_->compile(e, variables, temps);
  MemoryLocation *load_from_2 = e.last_written_to;

  MemoryLocation *store_to = new MemoryLocation();
  store_to->type = TEMPORARY;

  l1->append(l2);
  delete l2;

  l1->append( new LDO(e.fp, load_from_2, e) );
  l1->append( new RALStmt(STA, e.scratch2) );
  l1->append( new LDO(e.fp, load_from_1, e) );
  l1->append( new RALStmt(SUB, e.scratch2) );
  l1->append( new STO(e.fp, store_to, e) );

  temps.push_back(store_to);
  e.last_written_to = store_to;

  return l1;
}

Times::Times(Expr* op1, Expr* op2)
{
	op1_ = op1;
	op2_ = op2;
}

int Times::eval(map<string,int> NT, map<string,Proc*> FT) const
{
	return op1_->eval(NT,FT) * op2_->eval(NT,FT);
}

Expr *Times::simplify()
{
  Expr *r;

  map<string, int> t;
  map<string,Proc*> f;

  if(op1_->isNumber() && op2_->isNumber())
  {
    r = new Number(op1_->eval(t, f) * op2_->eval(t, f));
    delete op1_;
    delete op2_;
  }
  else if(op1_->isNumber() && op1_->eval(t, f) == 1)
  {
    r = op2_;
    delete op1_;
  }
  else if(op2_->isNumber() && op2_->eval(t, f) == 1)
  {
    r = op1_;
    delete op2_;
  }
  else if(op1_->isNumber() && op1_->eval(t, f) == 0 ||
          op2_->isNumber() && op2_->eval(t, f) == 0)
  {
    delete op1_;
    delete op2_;
    r = new Number(0);
  }
  else
  {
    return this;
  }

  free(this);
  return r;
}

RALStmtList *Times::compile(Env &e,
                            map<string, MemoryLocation*> &variables, 
                            vector<MemoryLocation*> &temps)
{
  RALStmtList *l1 = op1_->compile(e, variables, temps);
  MemoryLocation *load_from_1 = e.last_written_to;

  RALStmtList *l2 = op2_->compile(e, variables, temps);
  MemoryLocation *load_from_2 = e.last_written_to;

  MemoryLocation *store_to = new MemoryLocation();
  store_to->type = TEMPORARY;

  l1->append(l2);
  delete l2;

  l1->append( new LDO(e.fp, load_from_1, e) );
  l1->append( new RALStmt(STA, e.scratch2) );
  l1->append( new LDO(e.fp, load_from_2, e) );
  l1->append( new RALStmt(MUL, e.scratch2) );
  l1->append( new STO(e.fp, store_to, e) );

  temps.push_back(store_to);
  e.last_written_to = store_to;

  return l1;
}

FunCall::FunCall(string name, list<Expr*> *AL)
{
	name_= name;
	AL_ = AL;
}

RALStmtList *FunCall::compile(Env &e,
                              map<string, MemoryLocation *> &variables, 
                              vector<MemoryLocation *> &temps)
{
  /* Set up the statement list we're going to return */
  RALStmtList *l = new RALStmtList();

  /* We're going to make a struct of things to fill in later; we're assuming
   * we know nothing about the function we're calling so that we can support
   * recursion */
  FunctionGap *g = new FunctionGap;

  /* First we've got to compile each of the expressions passed as arguments
   * to the function.
   * We're also keeping track of the memory locations they're written to in a
   * list, so that we can write them to the activation record once we update 
   * the FP and SP */
  list<Expr*>::iterator AL_it;
  list<MemoryLocation*> arguments;
  for(AL_it = AL_->begin(); AL_it != AL_->end(); AL_it++)
  {
    l->append( (*AL_it)->compile(e, variables, temps) );
    arguments.push_back(e.last_written_to);
  }

  /* Now we've got to update the FP and SP - this means we've got to store the
   * fp in prev_fp, update them. We won't know what to update the sp with yet */
  l->append( new RALStmt(LDA, e.fp) );
  l->append( new RALStmt(STA, e.prev_fp) );

  l->append( new RALStmt(LDA, e.sp) );
  l->append( new RALStmt(ADD, getConstant(e.constants, 1)) );
  l->append( new RALStmt(STA, e.fp) );

  l->append( new RALStmt(LDA, e.sp) );
  g->ADDwithActivationRecordSize = new RALStmt(ADD, NULL);
  l->append( g->ADDwithActivationRecordSize );
  l->append( new RALStmt(STA, e.sp) );

  /* Now let's save the previous fp to the stack so we know where
   * to go back to */
  l->append( new RALStmt(LDA, e.prev_fp) );
  g->STOwithPrevFPOffset = new STO(e.fp, NULL, e);
  l->append( g->STOwithPrevFPOffset );

  /* We need the label from the statement to return to */
  LDO *ret_stmt = new LDO(e.fp, NULL, e);
  
  /* So that we can create a constant to load from to store the return address
   * ... if this doesn't work we might just want to make a "call" instruction
   * and use that */
  l->append( 
      new RALStmt(LDA, getConstant(e.constants, ret_stmt->getFirstLabel()))
      );
  g->STOwithReturnAddressOffset = new STO(e.fp, NULL, e);
  l->append( g->STOwithReturnAddressOffset );

  /* Iterate through the arguments list and store them in the new
   * activation record */
  list<MemoryLocation*>::iterator arg_it;
  for(arg_it = arguments.begin(); arg_it != arguments.end(); arg_it++)
  {
    l->append( new LDO(e.prev_fp, *arg_it, e) );
    g->params.push_back(new STO(e.fp, NULL, e));
    l->append( g->params.back() );
  }

  /* Make the jump statement and add it to the incomplete record struct */
  g->JMPtoFunction = new RALStmt(JMP, NULL);
  l->append( g->JMPtoFunction );

  /* And add the statement we've already made: Get the prev_fp from the stack,
   * then store it in e.prev_fp */
  g->LDOwithPrevFPOffset = ret_stmt;
  l->append( g->LDOwithPrevFPOffset ); 
  l->append( new RALStmt(STA, e.prev_fp) );

  /* Set up a MemoryLocation (this is still and Expr so we need to store
   * its result) */
  MemoryLocation *store_to = new MemoryLocation;
  store_to->type = TEMPORARY;
  temps.push_back(store_to);

  /* Fetch the return value and store it to the memory location we just made */
  g->LDOwithReturnValueOffset = new LDO(e.fp, NULL, e);
  l->append( g->LDOwithReturnValueOffset );
  l->append( new STO(e.prev_fp, store_to, e) );

  /* Move the sp back */
  l->append( new RALStmt(LDA, e.fp) );
  l->append( new RALStmt(SUB, getConstant(e.constants, 1)) );
  l->append( new RALStmt(STA, e.sp) );

  /* And the fp */
  l->append( new RALStmt(LDA, e.prev_fp) );
  l->append( new RALStmt(STA, e.fp) );

  /* Mark store_to as the last memory location we've written,
   * add our gap to the list of incomplete records or fill it in,
   * and return the statement list we made */
  e.last_written_to = store_to;
  
  if(e.functions[name_] != NULL)
    fillIn(g, e.functions[name_], e);
  else
    e.toCompile[name_].push_back(g);

  return l;
}

FunCall::~FunCall() { delete AL_; }

int FunCall::eval(map<string,int> NT, map<string,Proc*> FT) const
{
	return FT[name_]->apply(NT, FT, AL_);
}

Proc::Proc(list<string> *PL, StmtList *SL)
{
	SL_ = SL;
	PL_ = PL;
	NumParam_ = PL->size();
}

int Proc::apply(map<string,int> &NT, map<string,Proc*> &FT, list<Expr*> *EL) 
{
	map<string,int> NNT;
	NNT.clear();

	// bind parameters in new name table

	list<string>::iterator p;
	list<Expr*>::iterator e;
	if (NumParam_ != EL->size()) {
		cout << "Param count does not match" << endl;
		exit(1);
	}
	for( p = PL_->begin(), e = EL->begin(); p != PL_->end(); p++, e++ ) 
		NNT[*p] = (*e)->eval(NT,FT);

	// evaluate function body using new name table and old function table

	SL_->eval(NNT,FT);
	if ( NNT.find("return") != NNT.end() )
		return NNT["return"];
	else {
		cout << "Error:  no return value" << endl;
		exit(1);
	}
}

RALFunction *Proc::compile(Env &e) 
{
  /* variables contains the function variables and temps contains all
   * the temporaries. Later we'll merge both of these into temp and call
   * that the activation record */
  map<string, MemoryLocation*> variables = map<string,MemoryLocation*>();
  vector<MemoryLocation*> temps = vector<MemoryLocation*>();

  MemoryLocation *prev_fp = new MemoryLocation;
  prev_fp->type = POINTER;

  MemoryLocation *ret_addr = new MemoryLocation;
  ret_addr->type = RETURN_ADDRESS;

  list<string>::iterator it;
  for(it = PL_->begin(); it != PL_->end(); it++)
  {
    variables[(*it)] = new MemoryLocation;
    variables[(*it)]->type = PARAMETER;
  }
  
  RALStmtList *statements = SL_->compile(e, variables, temps);

  RALStmtList *return_from_function = new RALStmtList();
  return_from_function->append( new LDO(e.fp, ret_addr, e) );
  return_from_function->append( new RALStmt(STA, e.scratch) );
  return_from_function->append( new RALStmt(JA, e.scratch) );

  statements->replaceNULLsWith(return_from_function->getFirstLabel());
  statements->append(return_from_function);

  RALFunction *function = new RALFunction();
  function->setStatementList(statements);

  function->prev_fp = prev_fp;
  function->ret_addr = ret_addr;

  if(variables.find("return") != variables.end())
  {
    function->ret_value = variables["return"];
    function->ret_value->type = RETURN_VALUE;
  }

  for(it = PL_->begin(); it != PL_->end(); it++)
    function->parameters.push_back(variables[(*it)]);

  map<string, MemoryLocation*>::iterator jt;
  for(jt = variables.begin(); jt != variables.end(); jt++)
  {
    temps.push_back(jt->second);
  }

  temps.push_back(prev_fp);
  temps.push_back(ret_addr);

  function->setActivationRecord(temps);
  function->link();

  return function;
}
