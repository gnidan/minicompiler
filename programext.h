#ifndef __PROGRAMEXT_H__
#define __PROGRAMEXT_H__

/*
 * file:  programext.h (contains declarations of Program, StmtList, Stmt, Expr)
 * Author: Jeremy Johnson 
 * Date: 2/05/07
 *
 * Modified by: G. Nicholas D'Andrea
 * Date: 3/05/08
 * Description: Compile methods and classes, etc.
 */
#include <iostream>
#include <string>
#include <map>
#include <list>

#include "ralprogram.h"

using namespace std;

void fillIn(FunctionGap *g, RALFunction *func, Env &e);

MemoryLocation *getConstant(vector<MemoryLocation *> &constants, int value);
MemoryLocation *getConstant(vector<MemoryLocation *> &constants, Label *value);
MemoryLocation *getConstant(vector<MemoryLocation *> &constants,
    MemoryLocation *value);

// forward declarations 
// StmtList used by IfStmt and WhileStmt which are Stmt
// Proc which contains StmtList used in Expr, Stmt, StmtList 
class StmtList;
class Proc;

class Expr
{
 public:
	Expr() {};
	virtual ~Expr() {};  
	virtual int eval( map<string,int> NT, map<string,Proc*> FT ) const = 0;  
	
	/* Postcondition: the last element in temps should be the (MemoryLocation*)
   * where the value of the expression is stored */
  virtual RALStmtList *compile(Env &e, 
                               map<string, MemoryLocation*> &variables, 
                               vector<MemoryLocation*> &temps){};

  virtual bool isNumber() { return false; };

  virtual Expr *simplify() { return this; };

 private:
};

class Number : public Expr
{
 public:
	Number( int value = 0 );
	int eval( map<string,int> NT, map<string,Proc*> FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);

  bool isNumber() { return true; };
  
 private:
	int value_;
};


class Ident : public Expr
{
 public:
	Ident( string name = "" );
	int eval( map<string,int> NT, map<string,Proc*> FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);
      
 private:
	string name_;
};

class Times : public Expr
{
 public:
	Times( Expr * op1 = NULL, Expr * op2 = NULL );
	~Times() {delete op1_; delete op2_;};
	int eval( map<string,int> NT, map<string,Proc*> FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);

  Expr *simplify();
	
 private:
	Expr* op1_;
	Expr* op2_;
};

class Plus : public Expr
{
 public:
	Plus( Expr* op1 = NULL, Expr* op2 = NULL );
	~Plus() {delete op1_; delete op2_;};
	int eval( map<string,int> NT, map<string,Proc*> FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);

  Expr *simplify();
	
 private:
	Expr* op1_;
	Expr* op2_;
};

class Minus : public Expr
{
 public:
	Minus( Expr* op1 = NULL, Expr* op2 = NULL );
	~Minus() {delete op1_; delete op2_;};
	int eval( map<string,int> NT, map<string,Proc*> FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);

  Expr *simplify();
  
 private:
	Expr* op1_;
	Expr* op2_;
};

class FunCall : public Expr
{
 public:
	FunCall( string name, list<Expr*> *AL );
	~FunCall();
	int eval( map<string,int> NT, map<string,Proc*> FT ) const;

	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);
 private:
	string name_;
	list<Expr*> *AL_;
};


class Stmt 
{
 public:
	Stmt() {};
	virtual ~Stmt() {};  
	virtual void eval( map<string,int> &NT, map<string,Proc*> &FT ) const = 0;  

	virtual RALStmtList *compile(Env &e, 
                               map<string, MemoryLocation*> &variables, 
                               vector<MemoryLocation*> &temps) = 0;
      
 private:
};


class AssignStmt: public Stmt
{
 public:
	AssignStmt( string name="", Expr *E=NULL );
	~AssignStmt() {delete E_;}; 
	void eval( map<string,int> &NT, map<string,Proc*> &FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);
	
 private:
	string name_;
	Expr* E_;
};

class DefineStmt : public Stmt
{ 
 public:
	DefineStmt( string name="", Proc *P=NULL );
	~DefineStmt();  
	void eval( map<string,int> &NT, map<string,Proc*> &FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);
    
 private:
	string name_;
	Proc* P_;
};


class IfStmt: public Stmt
{
 public:
	IfStmt( Expr *E,StmtList *S1, StmtList *S2 );
	~IfStmt();
	void eval( map<string,int> &NT, map<string,Proc*> &FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);
	
 private:
	Expr* E_;
	StmtList *S1_;
	StmtList *S2_;
};

class WhileStmt: public Stmt
{
 public:
	WhileStmt( Expr *E,StmtList *S );
        ~WhileStmt();
	void eval( map<string,int> &NT, map<string,Proc*> &FT ) const;
	
	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);
  
 private:
	Expr* E_;
	StmtList *S_;
};


class StmtList 
{
 public:
	StmtList() {};
	void eval( map<string,int> &NT, map<string,Proc*> &FT );  
	void insert( Stmt *T );  

	RALStmtList *compile(Env &e, 
                       map<string, MemoryLocation*> &variables, 
                       vector<MemoryLocation*> &temps);

 private:
	list<Stmt*> SL_;
};

class Proc
{
 public:
	Proc( list<string> *PL, StmtList *SL );
	~Proc() {delete SL_; };  
	int apply( map<string,int> &NT, map<string,Proc*> &FT, list<Expr*> *EL );

	RALFunction *compile(Env &e);

 private:
	StmtList *SL_;
	list<string> *PL_;
	int NumParam_;
};

class Program 
{
 public:
	Program( StmtList *SL );
	~Program() { delete SL_; };
	void dump();
	void eval();
	
	RALProgram *compile();

 private:
	StmtList *SL_;
	map<string,int> NameTable_;
	map<string,Proc*> FunctionTable_;
};

#endif
