/***************************************************************************
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "X86.hpp"
#include "Display.hpp"

#include <stdlib.h>

X86SubProgram::X86SubProgram() 
  : SizeofDWord(sizeof(int)), //4
    _total_locals(0),
    _param_offset(0), 
    _local_offset(0) {
  
}

X86SubProgram::X86SubProgram(const X86SubProgram& other) 
    : SizeofDWord(sizeof(int)), 
    _total_locals(other._total_locals),
    _param_offset(other._param_offset),
    _local_offset(other._local_offset),
    _name(other._name),
    _params(other._params),
    _locals(other._locals) {

  _head << other._head.str();
  _txt  << other._txt.str();
  _init << other._init.str();
}

X86SubProgram::~X86SubProgram() {
}

void X86SubProgram::writeTEXT(const string& str) {
  _txt << str << endl;
}

void X86SubProgram::init(const string& name, int total_params, int total_locals) {
  _name = name;
  if(total_params) {
    _param_offset = SizeofDWord + (total_params * SizeofDWord);
  }
  if(total_locals) {    
    _local_offset = total_locals * SizeofDWord;
  }

  _total_locals  = total_locals - total_params;
}

string X86SubProgram::name() {
  return _name;
}

void X86SubProgram::declareLocal(const string& local_var, int type) {
  _head << "%define " << local_var << " [ebp-" << _local_offset << "]" << endl;

  _init << "mov dword " << local_var << ", 0" << endl;

  _local_offset += SizeofDWord;
}

void X86SubProgram::declareParam(const string& param, int type) {

  _head << "%define " << param << " [ebp+" << _param_offset << "]" << endl;

  _param_offset -= SizeofDWord;
}

void X86SubProgram::writeMatrixInitCode(int decl_type, const string& varname, int type, int size) {
  if(decl_type == X86::VAR_GLOBAL) {
    _init << "addarg " << size << " * SIZEOF_DWORD" << endl;
    _init << "call __malloc" << endl;
    _init << "clargs 1" << endl;
    _init << "mov [" << varname << "], eax" << endl;
    _init << "addarg dword [" << varname << "]" << endl;
    _init << "addarg " << size << " * SIZEOF_DWORD" << endl;
    _init << "call matrix_init__" << endl;
    _init << "clargs 2" << endl;
  } else if(decl_type == X86::VAR_LOCAL) {
    _init << "addarg " << size << " * SIZEOF_DWORD" << endl;
    _init << "call __malloc" << endl;
    _init << "clargs 1" << endl;
    _init << "mov dword " << varname << ", eax" << endl;
    _init << "addarg dword " << varname << endl;
    _init << "addarg " << size << " * SIZEOF_DWORD" << endl;
    _init << "call matrix_init__" << endl;
    _init << "clargs 2" << endl;  
  } else {
    _init << "addarg " << varname << endl;
    _init << "addarg " << size << endl;
    _init << "addarg " << (type == TIPO_LITERAL) << endl;
    _init << "call __clone_matrix" << endl;
    _init << "clargs 3" << endl;
    _init << "mov [" << varname << "], eax" << endl;
  }
}

string X86SubProgram::source() {
  stringstream s;
  s << name() << ":" << endl;
  s << _head.str();

  if(name() != X86::EntryPoint) {
    s << "begin " << (_total_locals *  SizeofDWord)<< endl;
  }

  s << _init.str();

  s << _txt.str();
 
  return s.str();
}






////////--------------------------------------------------------




string X86::EntryPoint = "_start";

X86::X86(SymbolTable& st) 
 : _stable(st) {

}

X86::~X86() {
}

void X86::init(const string& name) {

    _head << "; algoritmo: " << name << "\n\n";

    #include <asm_tmpl.h>

    _bss << "section .bss\n"
           "    __mem    resb  __MEMORY_SIZE\n\n";

    _data << "section .data\n"
            "              _data_no equ $\n"
            "    __mem_ptr         dd 0\n"
            "    __aux             dd 0\n"
            "    __str_true        db 'verdadeiro',0\n"
            "    __str_false       db 'falso',0\n"
            "    __str_no_mem_left db 'N�o foi poss�vel alocar mem�ria.',0\n\n";


  createScope(SymbolTable::GlobalScope);
}

void X86::writeBSS(const string& str) {
  _bss << str << endl;
}

void X86::writeDATA(const string& str) {
  _data << str << endl;
}

void X86::writeTEXT(const string& str) {
  _subprograms[_currentScope].writeTEXT(str);;

}

void X86::createScope(const string& scope) {
  _currentScope = scope;
  if(scope == SymbolTable::GlobalScope) {
    _subprograms[scope].init(X86::EntryPoint);
  } else {
    Symbol symb = _stable.getSymbol(SymbolTable::GlobalScope, scope, true);
    _subprograms[scope].init(scope, 
      symb.param.symbolList().size(), _stable.getSymbols(scope).size());
  }
}

string X86::currentScope() {
  return _currentScope;
}

void X86::writeExit() {
  writeTEXT("exit 0");
}

void X86::declarePrimitive(int decl_type, const string& name, int type) {
  stringstream s;
  if(decl_type == VAR_GLOBAL) {
    //all sizes have 32 bits (double word)
    switch(type) {
      case TIPO_INTEIRO:
      case TIPO_REAL:
      case TIPO_CARACTERE:
      case TIPO_LITERAL:
      case TIPO_LOGICO:
        s << name << " dd 0";
        writeDATA(s.str());
        break;
      default:
        Display::self()->showError("Erro interno: tipo nao suportado (X86::declarePrimitive).");
        exit(1);
    }
  } else if(decl_type == VAR_PARAM) {
    _subprograms[currentScope()].declareParam(name, type);
  } else if(decl_type == VAR_LOCAL) {
    _subprograms[currentScope()].declareLocal(name, type);
  } else {
    Display::self()->showError("Erro interno: X86::declarePrimitive).");
    exit(1);
  }
}

void X86::declareMatrix(int decl_type, int type, string name, list<string> dims) {
  int size = 1;
  for(list<string>::iterator it = dims.begin(); it != dims.end(); it++) {
    size *= atoi((*it).c_str());
  }

  if(decl_type == VAR_GLOBAL) {
    declarePrimitive(VAR_GLOBAL, name, TIPO_INTEIRO);//*TIPO_INTEIRO (pointer)        
  } else if(decl_type == VAR_PARAM) {
    _subprograms[currentScope()].declareParam(name, type);
  } else if(decl_type == VAR_LOCAL) {
    _subprograms[currentScope()].declareLocal(name, type);
  } else {
    Display::self()->showError("Erro interno: X86::declareMatrix).");
    exit(1);
  }

  _subprograms[currentScope()].writeMatrixInitCode(decl_type, name, type, size);
}


string X86::toNasmString(string str) {
  string ret;
  for(unsigned int i = 0; i < str.length(); i++) {
    if(str[i] == '\\') {
      i++;
      switch(str[i]) {
        case '0':
          ret += "',0,'";
          break;
        case 'n':
          ret += "',10,'";
          break;
        case 'r':
          ret += "',13,'";
          break;
        default:
          ret += str[i];
      }
    } else {
      ret += str[i];
    }
  }
  return ret;
}

string X86::addGlobalLiteral(string str) {
  stringstream s;
  string lb = createLabel(false, "str");
  s << lb << " db '" << toNasmString(str) << "',0";
  writeDATA(s.str());
  return lb;
}

string X86::translateFuncLeia(const string& id, int type) {
  switch(type) {
    case TIPO_REAL:
      return "__leia_real";
    case TIPO_LITERAL:
      return "__leia_literal";
    case TIPO_CARACTERE:
      return "__leia_caractere";
    case TIPO_LOGICO:
      return "__leia_logico";
    case TIPO_INTEIRO:
    default:
      return "__leia_inteiro";
  }
}

string X86::translateFuncImprima(const string& id, int type) {
  switch(type) {
    case TIPO_REAL:
      return "__imprima_real";
    case TIPO_LITERAL:
      return "__imprima_literal";
    case TIPO_CARACTERE:
      return "__imprima_caractere";
    case TIPO_LOGICO:
      return "__imprima_logico";
    case TIPO_INTEIRO:
      return "__imprima_inteiro";
    default:
      Display::self()->showError("Erro interno: tipo nao suportado (x86::translateFuncImprima).");
      exit(1);
  }
}



string X86::createLabel(bool local, string tmpl) {
  static int c = 0;
  stringstream s;
  if(local) {
    s << ".___" << tmpl << "_" << c;
  } else {
    s << "___" << tmpl << "_" << c;
  }
  c++;
  return s.str();
}


string X86::source() {
  stringstream str;

  //.data footer
  writeDATA("datasize   equ     $ - _data_no");

  str << _head.str()
      << _bss.str()
      << _data.str();

  //.text header
  str << "section .text" << endl;
  str << "_start_no equ $" << endl;

  string sss;
  for(map<string, X86SubProgram>::iterator it = _subprograms.begin(); it != _subprograms.end(); ++it) {
    sss = it->second.source();
    str << it->second.source();
  }

  str  << _lib.str();
  
  return str.str();
}

void X86::writeCast(int e1, int e2) {
  //casts
  if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) {
    //int to float
    writeTEXT("mov dword [__aux], eax");
    writeTEXT("fild dword [__aux]");
    writeTEXT("fstp dword [__aux]");
    writeTEXT("mov eax, dword [__aux]");
  } else if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) {
    writeTEXT("mov dword [__aux], eax");
    writeTEXT("fld dword [__aux]");
    writeTEXT("fistp dword [__aux]");
    writeTEXT("mov eax, dword [__aux]");
  }
}

void X86::writeAttribution(int e1, int e2, pair<pair<int, bool>, string>& lv) {
  writeTEXT("pop eax");
  writeTEXT("pop ecx");

  writeCast(e1, e2);

  stringstream s;  
  if(_stable.getSymbol(currentScope(), lv.second, true).scope != SymbolTable::GlobalScope) {
    if(lv.first.second) { //if is primitive
      s << "lea edx, " << lv.second;
    } else {
      s << "mov edx, dword [" << lv.second << "]";
    }
  } else {
    if(lv.first.second) { //if is primitive
      s << "mov edx, dword " << lv.second;
    } else {
      s << "mov edx, dword [" << lv.second << "]";
    }
  }
  writeTEXT(s.str());
  s.str("");
  s << "lea edx, [edx + ecx * SIZEOF_DWORD]";
  writeTEXT(s.str());
  s.str("");
  s << "mov [edx], eax";
  writeTEXT(s.str());
}


void X86::writeOuExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("cmp eax, 0");
  writeTEXT("setne al");
  writeTEXT("and eax, 0xff");
  writeTEXT("cmp ebx, 0");
  writeTEXT("setne bl");
  writeTEXT("and ebx, 0xff");
  writeTEXT("or al, bl");

  writeTEXT("push eax");
}

void X86::writeEExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("cmp eax, 0");        
  writeTEXT("setne al");
  writeTEXT("and eax, 0xff");
  writeTEXT("cmp ebx, 0");
  writeTEXT("setne bl");
  writeTEXT("and ebx, 0xff");
  writeTEXT("and al, bl");

  writeTEXT("push eax");
}

void X86::writeBitOuExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("or eax, ebx");

  writeTEXT("push eax");
}

void X86::writeBitXouExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("xor eax, ebx");

  writeTEXT("push eax");
}

void X86::writeBitEExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("and eax, ebx");

  writeTEXT("push eax");
}

void X86::writeIgualExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("cmp eax, ebx");
  writeTEXT("sete al");
  writeTEXT("and eax, 0xff");

  writeTEXT("push eax");
}

void X86::writeDiferenteExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("cmp eax, ebx");
  writeTEXT("setne al");
  writeTEXT("and eax, 0xff");
  
  writeTEXT("push eax");
}

void X86::writeMaiorExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  //fcomp assumes ST0 is left-hand operand aways
  //flags after comp:
  //5.0 @ 4 : ax -> 0
  //5.0 @ 5 : ax -> 0x4000
  //4.0 @ 6 : ax -> 0x100
  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0");
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0x100");
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fcomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0");
    }
    writeTEXT("sete al");
    writeTEXT("and eax, 0xff");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT("setg al");
    writeTEXT("and eax, 0xff");
  }
  
  writeTEXT("push eax");
}

void X86::writeMenorExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0x100");
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0");
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fcomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0x100");
    }
    writeTEXT("sete al");
    writeTEXT("and eax, 0xff");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT("setl al");
    writeTEXT("and eax, 0xff");
  }

  writeTEXT("push eax");
}

void X86::writeMaiorEqExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0");
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0x100");
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fcomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0");
    }
    writeTEXT("sete bl");
    writeTEXT("and ebx, 0xff");
    
    writeTEXT("cmp ax, 0x4000");
    writeTEXT("sete al");
    writeTEXT("and eax, 0xff");
    writeTEXT("or eax, ebx");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT("setge al");
    writeTEXT("and eax, 0xff");
  }

  writeTEXT("push eax");
}

void X86::writeMenorEqExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0x100");
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      writeTEXT("ficomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0");
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fcomp dword [__aux]");
      writeTEXT("fstsw ax");
      writeTEXT("cmp ax, 0x100");
    }
    writeTEXT("sete bl");
    writeTEXT("and ebx, 0xff");
    
    writeTEXT("cmp ax, 0x4000");
    writeTEXT("sete al");
    writeTEXT("and eax, 0xff");
    writeTEXT("or eax, ebx");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT("setle al");
    writeTEXT("and eax, 0xff");
  }

  writeTEXT("push eax");
}

void X86::writeMaisExpr(int e1, int e2) { 
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    string addpop;
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      addpop = "fiadd dword [__aux]";
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      addpop = "fiadd dword [__aux]";
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      addpop = "fadd dword [__aux]";
    }
      writeTEXT(addpop);
      writeTEXT("fstp dword [__aux]");
      writeTEXT("mov eax, dword [__aux]");
  } else {
    writeTEXT("add eax, ebx");    
  }

  writeTEXT("push eax");
}

void X86::writeMenosExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    string subop;
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      subop = "fisub dword [__aux]";
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      subop = "fisub dword [__aux]";
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      subop = "fsub dword [__aux]";
    }
      writeTEXT(subop);
      writeTEXT("fstp dword [__aux]");
      writeTEXT("mov eax, dword [__aux]");
  } else {
    writeTEXT("sub eax, ebx");
  }

  writeTEXT("push eax");
}

void X86::writeDivExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    string divpop;
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      divpop = "fidiv dword [__aux]";
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      divpop = "fidivr dword [__aux]";
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      divpop = "fdiv dword [__aux]";
    }
      writeTEXT(divpop);
      writeTEXT("fstp dword [__aux]");
      writeTEXT("mov eax, dword [__aux]");
  } else {
    writeTEXT("xor edx, edx");
    writeTEXT("idiv ebx");
  }

  writeTEXT("push eax");
}

void X86::writeMultipExpr(int e1, int e2) {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  if((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    string mulpop;
    writeTEXT("fninit");
    if((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) { //float/integer
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      mulpop = "fimul dword [__aux]";
    } else if((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) { //integer/float
      writeTEXT("mov [__aux], ebx");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], eax");
      mulpop = "fimul dword [__aux]";
    } else { //float/float
      writeTEXT("mov [__aux], eax");
      writeTEXT("fld dword [__aux]");
      writeTEXT("mov [__aux], ebx");
      mulpop = "fmul dword [__aux]";
    }
      writeTEXT(mulpop);
      writeTEXT("fstp dword [__aux]");
      writeTEXT("mov eax, dword [__aux]");
  } else {
    writeTEXT("imul eax, ebx");
  }

  writeTEXT("push eax");
}

void X86::writeModExpr() {
  writeTEXT("pop ebx");
  writeTEXT("pop eax");

  writeTEXT("xor edx, edx");
  writeTEXT("idiv ebx");        
  writeTEXT("mov eax, edx");

  writeTEXT("push eax");
}

void X86::writeUnaryNeg(int etype) {
  writeTEXT("pop eax");

  stringstream s;
  if(etype == TIPO_REAL) {
    s << "or eax ,0x80000000";
  } else {
    s << "neg eax";          
  }
  writeTEXT(s.str());

  writeTEXT("push eax");
}

void X86::writeUnaryNot() {
  writeTEXT("pop eax");

  writeTEXT("mov ebx, eax");
  writeTEXT("xor eax, eax");
  writeTEXT("cmp ebx, 0");
  writeTEXT("sete al");

  writeTEXT("push eax");
}

void X86::writeUnaryBitNotExpr() {
  writeTEXT("pop eax");
  writeTEXT("not eax");

  writeTEXT("push eax");
}


void X86::writeLiteralExpr(const string& src) {
  stringstream s;
  s << "push " << src;
  writeTEXT(s.str());
}

void X86::writeLValueExpr(pair< pair<int, bool>, string>& lv) {
  stringstream s;

  if(_stable.getSymbol(currentScope(), lv.second, true).scope != SymbolTable::GlobalScope) {
    if(lv.first.second) { //if is primitive
      s << "lea edx, " << lv.second;
    } else {
      s << "mov edx, dword [" << lv.second << "]";
    }
  } else {
    if(lv.first.second) { //if is primitive
      s << "mov edx, dword " << lv.second;
    } else {
      s << "mov edx, dword [" << lv.second << "]";
    }
  }

  writeTEXT(s.str());
  s.str("");
  s << "lea edx, [edx + ecx * SIZEOF_DWORD]";
  writeTEXT(s.str());
  s.str("");
  s << "push dword [edx]";  
  writeTEXT(s.str());
  //s << "push dword [" << src << " + ecx*4]"; 
  //writeTEXT(s.str());
}

string X86::toChar(const string& str) {
  if(str[0] != '\\') {
    return str;
  } else {
    string ret;
    switch(str[1]) {
      case '0':        
        ret = "0";
        break;
      case 'n':
        ret = "10";
        break;
      case 'r':
        ret += "13";
        break;
      default:
        ret = str[1];
    }
    return ret;
  }
}

string X86::toReal(const string& str) {
  stringstream s;
  //get the content of a float variable to integer.
  float fvalue; //sizeof(float) should be 4
  long  *fvaluep; //sizeof(long) should be 4
  fvalue = atof(str.c_str()); 
  fvaluep = (long*) &fvalue; 
  s << *fvaluep; 
  return s.str();
} 
