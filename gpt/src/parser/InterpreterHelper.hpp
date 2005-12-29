#ifndef INTERPRETERHELPER_HPP
#define INTERPRETERHELPER_HPP

#include "Symbol.hpp"
#include "SymbolTable.hpp"

#include <string>
#include <sstream>
#include <list>
#include <stack>
#include <iostream>

using namespace std;

class ExprValue {
  public:
    void setValue(string str) {
      value = str;
    }
    void setValue(const stringstream& s) {
      value = s.str();
    }

    bool ifTrue() {
      if(type == TIPO_LITERAL) {
        return (value.length() > 0)?true:false;
      } else {
        return (atof(value.c_str()))?true:false;
      }
    }

  string value;
  int type;
};

class Variable {
  public:
  string name;
  int type;

  bool isPrimitive;
  string primitiveValue;

//   list<string> matrixExprValue;
  list<int>    dimensions; //dim configuration
};

class LValue {
  public:
    void addMatrixIndex(ExprValue& e) {
      dims.push_back(e.value);
    }        

  string name;
  list<string> dims;//0,2,3 == X[0][2][3]
};


class Variables {
public:
  void init() { //init glovals
    //currentVars must have all global vars at this point
    globalVars = currentVars;
  }

  void init(const string& scope) {    
    varstates.push(currentVars);
    currentVars.clear();
  }

  void add(Variable& v) {
    currentVars[v.name] = v;
  }

  Variable& get(const string& name) {
    if(currentVars.find(name) == currentVars.end()) {
      return globalVars[name];
    } else {
      return currentVars[name];
    }
  }

  void popState() {    
    currentVars = varstates.top();
    varstates.pop();
  }

private:
  typedef map<string, Variable> VariableState_t;

  stack<VariableState_t> varstates;
  map<string, Variable> currentVars;//map<varname, Variable>
  map<string, Variable> globalVars;
};

class PrivateInterpreter {
  public:
  PrivateInterpreter(SymbolTable& st) : stable(st) {}
  
  void init() {
    list<Symbol> globals = stable.getSymbols(SymbolTable::GlobalScope);

    for(list<Symbol>::iterator it = globals.begin(); it != globals.end(); ++it) {
      Variable v;
      v.name = (*it).lexeme;
      v.type = (*it).type.primitiveType();
      v.isPrimitive = (*it).type.isPrimitive();
      v.dimensions = (*it).type.dimensions();
      variables.add(v);
    }

    variables.init();
  }

  void nextCmd(int line) {
    //conection
  }

  void execAttribution(LValue& lvalue, ExprValue& v) {
    //1: get the symbol
    //3: set the v.value to the variable in the current scope
    Variable& var = variables.get(lvalue.name);

    if(var.isPrimitive) { //primitive 
      var.primitiveValue = v.value;      
      cerr << var.name << ":=" << v.value << endl;
    } else {
      //TODO!
    }   
  }

  void execPasso(LValue& lvalue, int passo) {
    //1: get the symbol
    //3: set the v.value to the variable in the current scope
    Variable& var = variables.get(lvalue.name);

    if(var.isPrimitive) { //primitive
      stringstream s;
      s << (atof(var.primitiveValue.c_str()) + passo);
      var.primitiveValue = s.str();
      cerr << "passo: " << var.name << ":=" << s.str() << endl;
    } else {
      //TODO!
    }
  }

  ExprValue evaluateOu(ExprValue& left, ExprValue& right) {
    //1 || 1
    //"ou" || "eu"
    // "" || 'a'

    ExprValue v;
    v.type = TIPO_LOGICO;

    bool l,r;
    if(left.type == TIPO_LITERAL) {
      l = (left.value.length() != 0)?true:false;
    } else {
      l = atof(left.value.c_str())?true:false;
    }

    if(right.type == TIPO_LITERAL) {
      l = (right.value.length() != 0)?true:false;
    } else {
      l = atof(right.value.c_str())?true:false;
    }

    stringstream s;
    s << (l || r);
    v.setValue(s);
    return v;
  }

  ExprValue evaluateE(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;

    bool l,r;
    if(left.type == TIPO_LITERAL) {
      l = (left.value.length() != 0)?true:false;
    } else {
      l = atof(left.value.c_str())?true:false;
    }

    if(right.type == TIPO_LITERAL) {
      l = (right.value.length() != 0)?true:false;
    } else {
      l = atof(right.value.c_str())?true:false;
    }


    stringstream s;
    s << (l && r);
    v.setValue(s);
    return v;
  }

  ExprValue  evaluateBitOu(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_INTEIRO;

    stringstream s;
    s << (atoi(left.value.c_str()) | atoi(right.value.c_str()));
    v.setValue(s);
    return v;
  }

  ExprValue evaluateBitXou(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_INTEIRO;

    stringstream s;
    s << (atoi(left.value.c_str()) ^ atoi(right.value.c_str()));
    v.setValue(s);
    return v;
  }

  ExprValue evaluateBitE(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_INTEIRO;

    stringstream s;
    s << (atoi(left.value.c_str()) & atoi(right.value.c_str()));
    v.setValue(s);
    return v;
  }

  ExprValue evaluateIgual(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;

    bool res;
   
    if((left.type == TIPO_LITERAL) || (right.type == TIPO_LITERAL)) {
      res = left.value == right.value;
    } else {
      if((left.type == TIPO_REAL) || (right.type == TIPO_REAL)) {
        res = atof(left.value.c_str()) == atof(right.value.c_str());//"0" == "0.000"
      } else { //caractere, inteiro, logico
        res = atoi(left.value.c_str()) == atoi(right.value.c_str());
      }
    }

    stringstream s;
    s << res;
    v.setValue(s);
    return v;
  }

  ExprValue evaluateDif(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;

    bool res;

    if((left.type == TIPO_LITERAL) || (right.type == TIPO_LITERAL)) {
      res = left.value != right.value;
    } else {
      if((left.type == TIPO_REAL) || (right.type == TIPO_REAL)) {
        res = atof(left.value.c_str()) != atof(right.value.c_str());//"0" == "0.000"
      } else { //caractere, inteiro, logico
        res = atoi(left.value.c_str()) != atoi(right.value.c_str());
      }
    }

    stringstream s;
    s << res;
    v.setValue(s);
    return v;
  }

  ExprValue evaluateMaior(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;
    
    bool res;

    if((left.type == TIPO_LITERAL) || (right.type == TIPO_LITERAL)) {
      res = left.value.length() > right.value.length();
    } else {
      if((left.type == TIPO_REAL) || (right.type == TIPO_REAL)) {
        res = atof(left.value.c_str()) > atof(right.value.c_str());//"0" == "0.000"
      } else { //caractere, inteiro, logico
        res = atoi(left.value.c_str()) > atoi(right.value.c_str());
      }
    }

    stringstream s;
    s << res;
    v.setValue(s);
    return v;
  }

  ExprValue evaluateMaiorEq(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;

    bool res;

    if((left.type == TIPO_LITERAL) || (right.type == TIPO_LITERAL)) {
      res = left.value.length() >= right.value.length();
    } else {
      if((left.type == TIPO_REAL) || (right.type == TIPO_REAL)) {
        res = atof(left.value.c_str()) >= atof(right.value.c_str());//"0" == "0.000"
      } else { //caractere, inteiro, logico
        res = atoi(left.value.c_str()) >= atoi(right.value.c_str());
      }
    }

    stringstream s;
    s << res;
    v.setValue(s);
    return v;
  }

  ExprValue evaluateMenor(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;

    bool res;

    if((left.type == TIPO_LITERAL) || (right.type == TIPO_LITERAL)) {
      res = left.value.length() < right.value.length();
    } else {
      if((left.type == TIPO_REAL) || (right.type == TIPO_REAL)) {
        res = atof(left.value.c_str()) < atof(right.value.c_str());//"0" == "0.000"
      } else { //caractere, inteiro, logico
        res = atoi(left.value.c_str()) < atoi(right.value.c_str());
      }
    }

    stringstream s;
    s << res;
    v.setValue(s);
    return v;
  }

  ExprValue evaluateMenorEq(ExprValue& left, ExprValue& right) {
    ExprValue v;
    v.type = TIPO_LOGICO;

    bool res;

    if((left.type == TIPO_LITERAL) || (right.type == TIPO_LITERAL)) {
      res = left.value.length() <= right.value.length();
    } else {
      if((left.type == TIPO_REAL) || (right.type == TIPO_REAL)) {
        res = atof(left.value.c_str()) <= atof(right.value.c_str());//"0" == "0.000"
      } else { //caractere, inteiro, logico
        res = atoi(left.value.c_str()) <= atoi(right.value.c_str());
      }
    }

    stringstream s;
    s << res;
    v.setValue(s);
    return v;
  }

  ExprValue evaluateMais(ExprValue& left, ExprValue& right) {
    ExprValue v;

    stringstream s;
    if((left.type == TIPO_REAL) || (left.type == TIPO_REAL)) {
      s << (atof(left.value.c_str()) + atof(right.value.c_str()));
      v.type = TIPO_REAL;
    } else {
      s << (atoi(left.value.c_str()) + atoi(right.value.c_str()));
      v.type = TIPO_INTEIRO;
    }

    v.setValue(s);
    return v;
  }
    
  ExprValue evaluateMenos(ExprValue& left, ExprValue& right) {
    ExprValue v;

    stringstream s;
    if((left.type == TIPO_REAL) || (left.type == TIPO_REAL)) {
      s << (atof(left.value.c_str()) - atof(right.value.c_str()));
      v.type = TIPO_REAL;
    } else {
      s << (atoi(left.value.c_str()) - atoi(right.value.c_str()));
      v.type = TIPO_INTEIRO;
    }

    v.setValue(s);
    return v;
  }


  ExprValue evaluateDiv(ExprValue& left, ExprValue& right) {
    ExprValue v;

    stringstream s;
    if((left.type == TIPO_REAL) || (left.type == TIPO_REAL)) {
      s << (atof(left.value.c_str()) / atof(right.value.c_str()));
      v.type = TIPO_REAL;
    } else {
      s << (atoi(left.value.c_str()) / atoi(right.value.c_str()));
      v.type = TIPO_INTEIRO;
    }

    v.setValue(s);
    return v;
  }

  ExprValue evaluateMultip(ExprValue& left, ExprValue& right) {
    ExprValue v;

    stringstream s;
    if((left.type == TIPO_REAL) || (left.type == TIPO_REAL)) {
      s << (atof(left.value.c_str()) * atof(right.value.c_str()));
      v.type = TIPO_REAL;
    } else {
      s << (atoi(left.value.c_str()) * atoi(right.value.c_str()));
      v.type = TIPO_INTEIRO;
    }

    v.setValue(s);
    return v;
  }

  ExprValue evaluateMod(ExprValue& left, ExprValue& right) {
    ExprValue v;

    stringstream s;

    s << (atoi(left.value.c_str()) % atoi(right.value.c_str()));
    v.type = TIPO_INTEIRO;

    v.setValue(s);
    return v;
  }

 
  ExprValue evaluateUnNeg(ExprValue& v) {
    stringstream s;
    s << -(atof(v.value.c_str()));
        
    v.setValue(s);
    return v;
  }

  ExprValue evaluateUnPos(ExprValue& v) {
    stringstream s;
    s << +(atof(v.value.c_str()));
    v.setValue(s);
    return v;
  }

  ExprValue evaluateUnNot(ExprValue& v) {
    v.type = TIPO_LOGICO;

    if(v.type == TIPO_LITERAL) {
      v.value = (v.value.length()>0)?"0":"1";
    } else {
      v.value = atof(v.value.c_str())?"0":"1";
    }

    return v;
  }

  ExprValue evaluateUnBNot(ExprValue& v) {
    stringstream s;
    s << ~(atoi(v.value.c_str()));
    v.setValue(s);
    return v;
  }


  ExprValue getLValueValue(LValue& l) {
    ExprValue value;

    Variable var = variables.get(l.name);
    value.type = var.type;
    if(var.isPrimitive) {
      value.value = var.primitiveValue;
    } //TODO: else if matrix!
    return value;
  }

  bool execLowerEq(LValue& lv, ExprValue& ate) {
    Variable var = variables.get(lv.name);
    //TODO: matrix

    //var.getValue(lv);//lv has information about the indexes uded
    return atoi(var.primitiveValue.c_str()) <= atoi(ate.value.c_str());
  }

  bool execBiggerEq(LValue& lv, ExprValue& ate) {
    Variable var = variables.get(lv.name);

    return atoi(var.primitiveValue.c_str()) >= atoi(ate.value.c_str());
  }


  void initFCall(const string& fname, list<ExprValue>& args) {
    variables.init(fname);

    //setup local vars

    list<Symbol> globals = stable.getSymbols(fname);

    for(list<Symbol>::iterator it = globals.begin(); it != globals.end(); ++it) {
      Variable v;
      v.name = (*it).lexeme;
      v.type = (*it).type.primitiveType();
      v.isPrimitive = (*it).type.isPrimitive();
      v.dimensions =  (*it).type.dimensions();
      variables.add(v);
    }

    //init params
    Symbol func = stable.getSymbol(SymbolTable::GlobalScope,fname);
    list< pair<string,SymbolType> >& params = func.param.symbolList();

    list< pair<string,SymbolType> >::iterator pit = params.begin();
    list<ExprValue>::iterator ait = args.begin();

    while((ait != args.end()) && (pit != params.end())) {
      Symbol pv = stable.getSymbol(fname, (*pit).first);
      Variable& var = variables.get(pv.lexeme);
      //TODO! matrix!
      var.primitiveValue = (*ait).value;
      
      ++ait;
      ++pit;
    }
  }
  
  void finishLastFCall() {
    variables.popState();
  }

  bool isBuiltInFunction(const string& fname) {
    return stable.getSymbol(SymbolTable::GlobalScope, fname).isBuiltin;    
  }

  ExprValue execBuiltInFunction(const string& fname, list<ExprValue>& args) {
    ExprValue v;
    if(fname == "leia") {
      return executeLeia();
    } else if(fname == "imprima") {
      executeImprima(args);
      return v;//empty value
    }
  }

  private:
    ExprValue executeLeia() {
      ExprValue ret;
      //ret.type = 
      cin >> ret.value;
      return ret;
    }

    void executeImprima(list<ExprValue>& args) {      
      for(list<ExprValue>::iterator it = args.begin(); it != args.end(); ++it) {
        switch((*it).type) {
          case TIPO_INTEIRO:
            cout << (int) atoi((*it).value.c_str());
            break;
          case TIPO_REAL:
            cout << (float) atof((*it).value.c_str());
            break;
          case TIPO_CARACTERE:
            cout << (char) atoi((*it).value.c_str());
            break;
          case TIPO_LOGICO:
            if(atoi((*it).value.c_str())) {
              cout << "verdadeiro";
            } else {
              cout << "falso";
            }
            break;
          case TIPO_LITERAL:
            cout << (*it).value;
            break;
          default:
            cout << (*it).value;
        }        
      }
      cout << endl;
      cout.flush();
    }

    Variables variables;
//     typedef map<string, Variable> VariableState_t;
//     stack<VariableState_t> varstates;
//     map<string, Variable> currentVars;//map<varname, Variable>
    SymbolTable& stable;
};

#endif
