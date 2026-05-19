#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "GalsDef.h"

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class Modality { VARIABLE, ARRAY, PARAMETER, FUNCTION };

struct _GALS_CLASS Symbol {
    std::string type;
    Modality    modality      = Modality::VARIABLE;
    int         position      = -1;
    int         scopeDepth    = 0;
    bool        isInitialized = false;
    bool        isFunction    = false;
    bool        isUsed        = false;
};

struct _GALS_CLASS SymbolRecord {
    std::string name;
    std::shared_ptr<Symbol> symbol;
};

class _GALS_CLASS SymbolTable {
public:
    SymbolTable() {
        enterScope();
    }

    void enterScope() {
        m_scopes.push({});
    }

    void exitScope() {
        if (m_scopes.size() > 1)
            m_scopes.pop();
    }

    std::shared_ptr<Symbol> addSymbol(const std::string& name, const std::string& type,
                                      Modality modality, int position) {
        auto& top = m_scopes.top();
        if (top.find(name) != top.end())
            return nullptr;
        bool isFn = (modality == Modality::FUNCTION);
        auto sym = std::make_shared<Symbol>(Symbol{
            type, modality, position, depth(), false, isFn, false
        });
        top[name] = sym;
        m_allSymbols.push_back({name, sym});
        return sym;
    }

    std::shared_ptr<Symbol> lookupSymbol(const std::string& name) const {
        auto temp = m_scopes;
        while (!temp.empty()) {
            const auto& scope = temp.top();
            auto it = scope.find(name);
            if (it != scope.end())
                return it->second;
            temp.pop();
        }
        return nullptr;
    }

    bool existsInCurrentScope(const std::string& name) const {
        return m_scopes.top().find(name) != m_scopes.top().end();
    }

    int depth() const {
        return static_cast<int>(m_scopes.size());
    }

    std::unordered_map<std::string, std::shared_ptr<Symbol>> currentScopeSymbols() const {
        return m_scopes.top();
    }

    const std::vector<SymbolRecord>& allSymbols() const {
        return m_allSymbols;
    }

    std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> visibleSymbols() const {
        std::unordered_map<std::string, std::shared_ptr<Symbol>> visible;
        auto temp = m_scopes;
        std::vector<std::unordered_map<std::string, std::shared_ptr<Symbol>>> layers;
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
        m_allSymbols.clear();
        enterScope();
    }

private:
    std::stack<std::unordered_map<std::string, std::shared_ptr<Symbol>>> m_scopes;
    std::vector<SymbolRecord> m_allSymbols;
};

#endif
