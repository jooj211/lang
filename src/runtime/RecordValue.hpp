#ifndef RECORD_VALUE_HPP
#define RECORD_VALUE_HPP

#include "Value.hpp"
#include <string>
#include <map>

// Representa uma instância de um tipo 'data' (um registro).
// É essencialmente um mapa que associa nomes de campos a outros valores.
class RecordValue : public Value {
public:
    // O mapa que armazena os campos do registro, ex: "x" -> IntValue*
    std::map<std::string, Value*> fields;

    // O nome do tipo do registro (ex: "Point") para referência futura.
    std::string type_name;

    explicit RecordValue(const std::string& type) : type_name(type) {}

    // O destrutor do RecordValue não deleta os ponteiros dos campos,
    // pois a 'value_pool' do interpretador já cuida disso.
    ~RecordValue() {}

    void print() const override {
        // Imprime o nome do tipo e o endereço do registro,
        // como é comum em muitas linguagens.
        // O `this` é um ponteiro, então o convertemos para um tipo que pode ser impresso.
        std::cout << type_name << "@" << (void*)this;
    }
};

#endif