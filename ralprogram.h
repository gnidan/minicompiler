#ifndef __RALPROGRAM_H__
#define __RALPROGRAM_H__
/*
 * file:  ralprogram.h
 * Author: G. Nicholas D'Andrea 
 * Date: 2/17/08
 *
 * Description: Declarations for RALProgram storage and linking
 * Modified: 3/05/08
 * Added functions, etc.
 */
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "programext.h"

using namespace std;

enum LocationType
{ 
  CONST, VARIABLE, TEMPORARY, PARAMETER, POINTER, RETURN_VALUE, RETURN_ADDRESS
};

typedef enum LocationType LocationType;

struct Label {
  int line;
};

typedef struct Label Label;

struct MemoryLocation {
  LocationType type;
  int address;
  union {
    int value;
    MemoryLocation *location;
    Label *label;
  };
};

typedef struct MemoryLocation MemoryLocation;

enum RALInstruction { LDA, LDI, STA, STI, ADD, SUB, MUL, JMP, JMZ, JMN, JA, HLT };

typedef enum RALInstruction RALInstruction;

typedef struct FunctionGap FunctionGap;

class RALFunction;
typedef struct {
  MemoryLocation *fp;
  MemoryLocation *sp;
  MemoryLocation *scratch;
  MemoryLocation *scratch2;
  MemoryLocation *prev_fp;
  map<string, RALFunction*> functions;
  vector<MemoryLocation *> constants;

  MemoryLocation *last_written_to;
  map<string, list<FunctionGap*> > toCompile;
} Env;

class RALStmt 
{
public:
  RALStmt();
	RALStmt(RALInstruction instruction);
	RALStmt(RALInstruction instruction, void *argument);
  ~RALStmt();

  Label *getLabel();

  RALInstruction getInstruction();
  void setInstruction(RALInstruction instruction);

  void* getArgument();
  void setArgument(void *argument);

  void output();

private:
  Label *label_;
  RALInstruction instruction_;
  void *argument_; 
};

class RALStmtList 
{
public:
  RALStmtList() {};

  /* These should check for NULL and appropriately do nothing - just as a
   * caution... */
  void append(RALStmt *S);  
  void append(RALStmtList *L); 

  void replaceNULLsWith(Label *label);

  void assignLineNumbers();
  Label *getFirstLabel() { return SL_.front()->getLabel(); };
  void peepholeOptimize();
  void output();

protected:
	vector <RALStmt*> SL_;
};

class LDO : public RALStmtList
{
public:
  LDO(MemoryLocation *fp, MemoryLocation *offset, Env &e);

  void setOffset(MemoryLocation* offset, vector<MemoryLocation*> &constants);

private:
  RALStmt *stmtWithOffset;
};

class STO : public RALStmtList
{
public:
  STO(MemoryLocation *fp, MemoryLocation *offset, Env &e);

  void setOffset(MemoryLocation* offset, vector<MemoryLocation*> &constants);

private:
  RALStmt *stmtWithOffset;
};

class RALFunction
{
public:
  RALFunction() { };

  RALStmtList *getStatementList() { return SL_; };
  void setStatementList(RALStmtList *statements);
  
  vector<MemoryLocation*> getActivationRecord() { return activationRecord_; };
  void setActivationRecord(vector<MemoryLocation*> activationRecord)
    { activationRecord_ = activationRecord; };
  
  void link();
  void output();

  Label *getFirstLabel();

  /* These are part of the activation record... they're just here for
   * convenience */
  MemoryLocation *prev_fp;
  MemoryLocation *ret_addr;
  MemoryLocation *ret_value;
  list<MemoryLocation *> parameters;

private:
  RALStmtList *SL_;
  /* All these memory locations are actually just offsets from the fp,
   * but then again, MemoryLocation pointers are abstract to begin with,
   * these just moreso. */
  vector<MemoryLocation *> activationRecord_;
};

class RALProgram 
{
public:
  RALProgram(Env e);

  void link();
  void output();
  void dump();

private:
  Env e_;
  RALStmtList *SL_;
};

struct FunctionGap {
  RALStmt *ADDwithActivationRecordSize;
  LDO *LDOwithReturnValueOffset;
  LDO *LDOwithPrevFPOffset;
  STO *STOwithReturnAddressOffset;
  STO *STOwithPrevFPOffset;
  RALStmt *JMPtoFunction;
  list<STO*> params;
};


#endif
