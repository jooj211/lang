## Instâncias de Teste da Linguagem Lang

Este repositório contém instâncias de teste para o compilador da Linguagem Lang, desenvolvida em 2025.  
Os testes estão organizados em diretórios correspondentes a diferentes etapas do compilador.

### Análise Sintática

O diretório `sintaxe` contém programas escritos em Lang para testar o analisador sintático (parser).  
Esses programas podem estar sintaticamente corretos ou incorretos, de acordo com o subdiretório em que se encontram. No entanto, eles podem não ser válidos em outras etapas do compilador (como semântica ou tipagem).

Este diretório está subdividido em:
- `certo`: programas que devem ser aceitos pelo parser.
- `errado`: programas que devem ser rejeitados pelo parser.

Um parser será considerado correto se aceitar todos os programas em `certo` e rejeitar todos os programas em `errado`.

### Interpretadores / Compiladores

O diretório `semantica` contém programas que devem ser executados corretamente por um interpretador ou compilador, de acordo com a semântica da linguagem Lang.  
Embora alguns desses programas possam não ser bem tipados, eles ainda devem funcionar conforme a definição semântica da linguagem.

Subdiretórios:
- `full`: programas que utilizam todos os recursos da linguagem (funções, vetores, tipos de dados e alocação dinâmica).
- `function`: programas que utilizam funções e recursos básicos.
- `simple`: programas simples, com apenas o procedimento principal, desvios condicionais e estruturas de repetição.

### Sistema de Tipos

O diretório `types` contém programas projetados para testar o sistema de tipos da linguagem.  
Ele possui a mesma subdivisão de `semantica`:
- `simple`
- `function`
- `full`  
Todos os programas nesses subdiretórios devem ser aceitos pelo sistema de tipos.

Além disso, há um subdiretório `errado`, que contém apenas programas que devem ser rejeitados pelo sistema de tipos.
