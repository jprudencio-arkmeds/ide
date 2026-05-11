#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "GalsDef.h"

#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

struct _GALS_CLASS Symbol {
    std::string type;
    int         position; // offset no código-fonte
};

class _GALS_CLASS SymbolTable {
public:
    SymbolTable() {
        enterScope(); // escopo global
    }

    void enterScope() {
        m_scopes.push({});
    }

    void exitScope() {
        if (m_scopes.size() > 1)
            m_scopes.pop();
    }

    bool addSymbol(const std::string& name, const std::string& type, int position = -1) {
        auto& top = m_scopes.top();
        if (top.find(name) != top.end())
            return false;
        top[name] = {type, position};
        return true;
    }

    std::optional<Symbol> lookupSymbol(const std::string& name) const {
        // Copia a pilha para percorrê-la sem destruí-la (pilha não tem iterador).
        auto temp = m_scopes;
        while (!temp.empty()) {
            const auto& scope = temp.top();
            auto it = scope.find(name);
            if (it != scope.end())
                return it->second;
            temp.pop();
        }
        return std::nullopt;
    }

    bool existsInCurrentScope(const std::string& name) const {
        const auto& top = m_scopes.top();
        return top.find(name) != top.end();
    }

    int depth() const {
        return static_cast<int>(m_scopes.size());
    }

    std::vector<std::pair<std::string, Symbol>> visibleSymbols() const {
        std::unordered_map<std::string, Symbol> visible;
        auto temp = m_scopes;
        // Percorre do mais externo ao mais interno para que o mais interno vença.
        std::vector<std::unordered_map<std::string, Symbol>> layers;
        while (!temp.empty()) {
            layers.push_back(temp.top());
            temp.pop();
        }
        for (auto it = layers.rbegin(); it != layers.rend(); ++it)
            for (auto& [k, v] : *it)
                visible[k] = v;
        return {visible.begin(), visible.end()};
    }

    void reset() {
        while (!m_scopes.empty())
            m_scopes.pop();
        enterScope();
    }

private:
    std::stack<std::unordered_map<std::string, Symbol>> m_scopes;
};

#endif
