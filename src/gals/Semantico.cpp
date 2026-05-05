#include "Semantico.h"
#include "Constants.h"

#include <iostream>

void Semantico::executeAction(int action, const Token *token)
{
    (void)action;
    (void)token;
    // Semantic actions are executed during analysis. No debug output is produced in CLI mode.
}

