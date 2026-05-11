#ifndef SYNTATIC_ERROR_H
#define SYNTATIC_ERROR_H

#include "AnalysisError.h"
#include "GalsDef.h"

#include <string>

class _GALS_CLASS SyntacticError : public AnalysisError
{
public:

    SyntacticError(const std::string &msg, int position = -1)
      : AnalysisError(msg, position) { }
};

#endif
