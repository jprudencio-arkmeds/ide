#ifndef ANALYSIS_ERROR_H
#define ANALYSIS_ERROR_H

#include "GalsDef.h"

#include <string>

class _GALS_CLASS AnalysisError
{
public:

    AnalysisError(const std::string &msg, int position = -1)
      : message(msg), position(position) { }

    const char *getMessage() const { return message.c_str(); }
    int getPosition() const { return position; }

private:
    std::string message;
    int position;
};

#endif
