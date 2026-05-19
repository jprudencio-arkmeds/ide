#include "TypeOperationsTable.h"

#include <algorithm>

bool TypeOperationsTable::isCompatible(const std::string& left, const std::string& right, Operator op) {
  try {
    Type leftType = stringToType(left);
    Type rightType = stringToType(right);
    auto leftIt = compatibleAtribuitionTypeTable.find(leftType);
    if (leftIt == compatibleAtribuitionTypeTable.end())
      return false;
    auto rightIt = leftIt->second.find(rightType);
    if (rightIt == leftIt->second.end())
      return false;
    const std::vector<Operator>& operators = rightIt->second;
    return std::find(operators.begin(), operators.end(), op) != operators.end();
  }
  catch (const std::invalid_argument&) {
    return false;
  }
}

Type TypeOperationsTable::stringToType(const std::string& str) {
  if (str == "int") return INT;
  if (str == "float") return FLOAT;
  if (str == "double") return DOUBLE;
  if (str == "char") return CHAR;
  if (str == "void") return VOID;
  if (str == "string") return STRING;
  throw std::invalid_argument("Invalid type string: " + str);
}
