#pragma once
//******************************************************************************
//
// This file is part of the OpenHoldem project
//    Source code:           https://github.com/OpenHoldem/openholdembot/
//    Forums:                http://www.maxinmontreal.com/forums/index.php
//    Licensed under GPL v3: http://www.gnu.org/licenses/gpl.html
//
//******************************************************************************
//
// Purpose: Virtual base class for all symbol-engines
//
//******************************************************************************

#define _ALLOW_KEYWORD_MACROS
#include <afxwin.h>
///#include "..\MemoryManagement_DLL\CSpaceOptimizedGlobalObject.h"

// OH-symbols are lower-case
// OpenPPL-symbols are upper-case
// To be used in EvaluateSymbol()
#define FAST_EXIT_ON_OPENPPL_SYMBOLS(name) if (isupper(name[0])) return false;

class CVirtualSymbolEngine/*#: public CSpaceOptimizedGlobalObject */{
 public:
  CVirtualSymbolEngine() {}
  virtual ~CVirtualSymbolEngine() {}
 public:
  virtual void InitOnStartup() {}
	virtual void UpdateOnConnection() {}
	virtual void UpdateOnHandreset() {}
	virtual void UpdateOnNewRound() {}
	virtual void UpdateOnMyTurn() {}
	virtual void UpdateOnHeartbeat() {}
  virtual void UpdateAfterAutoplayerAction(int autoplayer_action_code) {}
 public:
  void WarnIfSymbolRequiresMyTurn(CString name);
  void WarnIfSymbolIsHoldemOnly(CString name);
 public:
	virtual bool EvaluateSymbol(const CString name, double *result, bool log = false) {
    // We don't provide any symbols
    return false;
  }
 public:  
	// To build a list of identifiers for the editor
	virtual CString SymbolsProvided() {
    // Default for symbol-engines that don't provide any symbols
    return "";
  }
};