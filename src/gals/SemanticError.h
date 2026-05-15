#ifndef SEMANTIC_ERROR_H
#define SEMANTIC_ERROR_H

#include "AnalysisError.h"
#include "GalsDef.h"

#include <string>

class _GALS_CLASS SemanticError : public AnalysisError
{
public:

    SemanticError(const std::string &msg, int position = -1)
      : AnalysisError(msg, position) { }
};

class _GALS_CLASS SemanticWarning : public AnalysisError
{
public:

  SemanticWarning(const std::string& msg, int position = -1)
    : AnalysisError(msg, position) {
  }
};

#endif
