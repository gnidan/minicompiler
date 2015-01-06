/*
 * file:  ralprogram.cpp 
 * Author: G. Nicholas D'Andrea 
 * Date: 2/17/08
 *
 * Description: Implementation for RALProgram storage and linking
 *
 * Modified: 3/05/08
 * Added function support
 */
#include <list>
#include "programext.h"
#include "ralprogram.h"

using namespace std;

/* Every statement is going to have its own unique Label...
 * this is irrelevant and essentially unordered until we link */
RALStmt::RALStmt()
{
  label_ = new Label();
}

RALStmt::RALStmt(RALInstruction instruction)
{
  label_ = new Label();
  setInstruction(instruction);
}

/* we're using (void*) for the argument because jump statements will address
 * a line label and all other statements will address a memory location...
 * meaning before we dereference the pointer we're going to have to check out
 * which will it be. */
RALStmt::RALStmt(RALInstruction instruction, void *argument)
{
  label_ = new Label();
  setInstruction(instruction);
  setArgument(argument);
}

RALStmt::~RALStmt()
{
  delete label_;
}

Label *RALStmt::getLabel()
{
  return label_;
}

RALInstruction RALStmt::getInstruction()
{
  return instruction_;
}

void RALStmt::setInstruction(RALInstruction instruction)
{
  instruction_ = instruction;
}

void *RALStmt::getArgument()
{
  return argument_;
}

void RALStmt::setArgument(void *argument)
{
  argument_ = argument;
}

void RALStmt::output()
{
  switch(instruction_)
  {
    case LDA:
      cout << "LDA ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case LDI:
      cout << "LDI ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case STA:
      cout << "STA ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case STI:
      cout << "STI ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case ADD:
      cout << "ADD ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case SUB:
      cout << "SUB ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case MUL:
      cout << "MUL ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case JMP:
      cout << "JMP ";
      cout << ((Label*)argument_)->line;
      break;
    case JMZ:
      cout << "JMZ ";
      cout << ((Label*)argument_)->line;
      break;
    case JMN:
      cout << "JMN ";
      cout << ((Label*)argument_)->line;
      break;
    case JA:
      cout << "JA ";
      cout << ((MemoryLocation*)argument_)->address;
      break;
    case HLT:
      cout << "HLT";
      break;
  }

  cout << endl;
}

void RALStmtList::append(RALStmt *S)
{
  SL_.push_back(S);
}

void RALStmtList::append(RALStmtList *L)
{
  SL_.insert(SL_.end(), L->SL_.begin(), L->SL_.end());
}

void RALStmtList::replaceNULLsWith(Label *label)
{
  vector<RALStmt*>::iterator it;
  for(it = SL_.begin(); it != SL_.end(); it++)
  {
    if((*it)->getArgument() == NULL)
      (*it)->setArgument(label);
  }
}

void RALStmtList::assignLineNumbers()
{
  int i;
  for(i = 0; i < SL_.size(); i++)
    /* remember it's (i+1) because lines start at 1 in RAL */
    SL_[i]->getLabel()->line = i + 1;
}

void RALStmtList::output()
{
  vector<RALStmt*>::iterator it;
  for(it = SL_.begin(); it != SL_.end(); it++)
    (*it)->output();
}

void RALStmtList::peepholeOptimize()
{
  vector<RALStmt*>::iterator it;
  for(it = SL_.begin() + 1; it != SL_.end(); it++)
  {
    vector<RALStmt*>::iterator prev = it - 1;
    if((*it)->getInstruction() == LDA && (*prev)->getInstruction() == STA &&
        (*it)->getArgument() == (*prev)->getArgument())
      SL_.erase(it);
  }
}

LDO::LDO(MemoryLocation *fp, MemoryLocation *offset, Env &e)
{
  SL_.push_back(new RALStmt(LDA, fp));

  if(offset != NULL)
    stmtWithOffset =
      new RALStmt(ADD, getConstant(e.constants, offset));
  else
    stmtWithOffset = new RALStmt(ADD, NULL);

  SL_.push_back(stmtWithOffset);
  SL_.push_back(new RALStmt(STA, e.scratch));
  SL_.push_back(new RALStmt(LDI, e.scratch));
}

void LDO::setOffset(MemoryLocation *offset, vector<MemoryLocation*> &constants)
{
  stmtWithOffset->setArgument(getConstant(constants, offset));
}

void STO::setOffset(MemoryLocation *offset, vector<MemoryLocation*> &constants)
{
  stmtWithOffset->setArgument(getConstant(constants, offset));
}

STO::STO(MemoryLocation *fp, MemoryLocation *offset, Env &e)
{
  SL_.push_back(new RALStmt(STA, e.scratch2));
  SL_.push_back(new RALStmt(LDA, fp));

  if(offset != NULL)
    stmtWithOffset =
      new RALStmt(ADD, getConstant(e.constants, offset));
  else
    stmtWithOffset = new RALStmt(ADD, NULL);

  SL_.push_back(stmtWithOffset);
  SL_.push_back(new RALStmt(STA, e.scratch));
  SL_.push_back(new RALStmt(LDA, e.scratch2));
  SL_.push_back(new RALStmt(STI, e.scratch));
}

RALProgram::RALProgram(Env e)
{
  e_ = e;
  SL_ = new RALStmtList();

  /* Store the return address to halt the program, then
   * Jump to the main function */
  
  RALStmt *hlt = new RALStmt(HLT, NULL);

  SL_->append(
      new RALStmt(LDA, getConstant(e_.constants, hlt->getLabel()))
      );
  STO *sto = new STO(e.fp, NULL, e);
  SL_->append( sto );

  SL_->append(
      new RALStmt(JMP, e_.functions[""]->getFirstLabel())
      );

  SL_->append(hlt);
  
  /* Append all the functions */
  map<string, RALFunction*>::iterator it;
  for(it = e_.functions.begin(); it != e_.functions.end(); it++)
  {
    SL_->append( it->second->getStatementList() );
  }

  SL_->assignLineNumbers();

  sto->setOffset(e_.functions[""]->ret_addr, e_.constants);
  link();

  e_.fp->value = e_.constants.back()->address + 1;

  e_.sp->value = e_.constants.back()->address + 
                 e_.functions[""]->getActivationRecord().size();
}

/* To link the program we need to have all the memory locations assigned 
 * addresses, and all the labels assigned line numbers. */
void RALProgram::link()
{
  int cur_addr = 1;
  e_.fp->address = cur_addr++;
  e_.sp->address = cur_addr++;
  e_.scratch->address = cur_addr++;
  e_.scratch2->address = cur_addr++;
  e_.prev_fp->address = cur_addr++;

  vector<MemoryLocation*>::iterator it;
  for(it = e_.constants.begin(); it != e_.constants.end(); it++)
    (*it)->address = cur_addr++;
}

void RALProgram::output()
{
  SL_->output();
}

void RALProgram::dump()
{
  cout << e_.fp->address << " " << e_.fp->value << endl;
  cout << e_.sp->address << " " << e_.sp->value << endl;
  cout << e_.scratch->address << " " << e_.scratch->value << endl;
  cout << e_.scratch2->address << " " << e_.scratch2->value << endl;
  cout << e_.prev_fp->address << " " << e_.prev_fp->value << endl;

  vector<MemoryLocation*>::iterator it;
  for(it = e_.constants.begin(); it != e_.constants.end(); it++)
    if((*it)->type == RETURN_ADDRESS)
      cout << (*it)->address << " " << (*it)->label->line << endl;
    else if((*it)->type == CONST)
      cout << (*it)->address << " " << (*it)->value << endl;
    else if((*it)->type == POINTER)
      cout << (*it)->address << " " << (*it)->location->address << endl;
}

void RALFunction::setStatementList(RALStmtList *statements)
{
  SL_ = statements;
}

void RALFunction::link()
{
  vector<MemoryLocation*> params, vars, temps, specials;
  int num_params = 0, num_vars = 0, num_temps = 0, num_specials = 0;

  vector<MemoryLocation*>::iterator it;
  for(it = activationRecord_.begin(); it != activationRecord_.end(); it++)
  {
    switch((*it)->type)
    {
      case VARIABLE:
        num_vars++;
        break;
      case PARAMETER:
        num_params++;
        break;
      case TEMPORARY:
        num_temps++;
        break;
      default:
        num_specials++;
    }
  }

  int params_pointer = 0,
      vars_pointer = num_params,
      temps_pointer = num_params + num_vars,
      specials_pointer = num_params + num_vars + num_temps;

  for(it = activationRecord_.begin(); it != activationRecord_.end(); it++)
  {
    switch((*it)->type)
    {
      case VARIABLE:
        (*it)->address = vars_pointer++;
        break;
      case PARAMETER:
        (*it)->address = params_pointer++;
        break;
      case TEMPORARY:
        (*it)->address = temps_pointer++;
        break;
      default:
        (*it)->address = specials_pointer++;
    }
  }

}

void RALFunction::output() { };

Label *RALFunction::getFirstLabel()
{
  return SL_->getFirstLabel();
}

