#include "TypeOperationsTable.h"

#include <algorithm>

namespace {
  bool isNumeric(Type type) {
    return type == INT || type == FLOAT || type == DOUBLE || type == CHAR;
  }

  bool isRelationalOrLogical(Operator op) {
    const int id = static_cast<int>(op);
    return id == t_KEY_GREATER || id == t_KEY_LESS ||
           id == t_KEY_GREATER_EQUAL || id == t_KEY_LESS_EQUAL ||
           id == t_KEY_EQUAL || id == t_KEY_NOT_EQUAL ||
           id == t_KEY_AND || id == t_KEY_OR;
  }

  bool isBitwiseOrShift(Operator op) {
    const int id = static_cast<int>(op);
    return id == t_KEY_BIT_AND || id == t_KEY_BIT_OR || id == t_KEY_BIT_XOR ||
           id == t_KEY_SHIFT_LEFT || id == t_KEY_SHIFT_RIGHT;
  }
}

bool TypeOperationsTable::isCompatible(const std::string& left, const std::string& right, Operator op) {
  try {
    Type leftType = stringToType(left);
    Type rightType = stringToType(right);

    if (isRelationalOrLogical(op)) {
      if (op == static_cast<Operator>(t_KEY_EQUAL) || op == static_cast<Operator>(t_KEY_NOT_EQUAL)) {
        return leftType == rightType || (isNumeric(leftType) && isNumeric(rightType));
      }
      return isNumeric(leftType) && isNumeric(rightType);
    }

    if (isBitwiseOrShift(op)) {
      return (leftType == INT || leftType == CHAR) && (rightType == INT || rightType == CHAR);
    }

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
