#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Informações associadas a um símbolo declarado.
 */
struct Symbol {
    std::string type;
    int         position; // offset no código-fonte
};

/**
 * Tabela de símbolos com escopo baseada em pilha de hash tables.
 *
 * Cada escopo léxico (bloco {}) é uma std::unordered_map separada empilhada
 * em std::stack. A busca percorre da tabela do topo (mais interno) até a base
 * (escopo global), implementando naturalmente o sombreamento de variáveis.
 */
class SymbolTable {
public:
    SymbolTable() {
        enterScope(); // escopo global
    }

    /** Inicia novo escopo — chamado ao encontrar '{'. */
    void enterScope() {
        m_scopes.push({});
    }

    /**
     * Encerra o escopo atual — chamado ao encontrar '}'.
     * Protege contra remoção do escopo global.
     */
    void exitScope() {
        if (m_scopes.size() > 1)
            m_scopes.pop();
    }

    /**
     * Declara um símbolo no escopo corrente.
     * @return false se o nome já existe neste escopo (redeclaração).
     */
    bool addSymbol(const std::string& name, const std::string& type, int position = -1) {
        auto& top = m_scopes.top();
        if (top.find(name) != top.end())
            return false;
        top[name] = {type, position};
        return true;
    }

    /**
     * Procura símbolo do escopo mais interno para o global.
     * @return Symbol se encontrado; std::nullopt caso contrário.
     */
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

    /** Verifica existência no escopo corrente apenas (para checar redeclaração). */
    bool existsInCurrentScope(const std::string& name) const {
        const auto& top = m_scopes.top();
        return top.find(name) != top.end();
    }

    /** Profundidade de escopo atual (1 = global). */
    int depth() const {
        return static_cast<int>(m_scopes.size());
    }

    /** Retorna todos os símbolos visíveis no escopo corrente (para depuração). */
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

    /** Reinicia a tabela para uma nova compilação. */
    void reset() {
        while (!m_scopes.empty())
            m_scopes.pop();
        enterScope();
    }

private:
    std::stack<std::unordered_map<std::string, Symbol>> m_scopes;
};

#endif
