# CMakeLists para src
add_library(lexer lexer.cpp)
add_library(parser parser.cpp)
add_library(ast ast.cpp)
add_library(symbols symbols.cpp)
add_library(semantic semantic.cpp)
add_library(runtime runtime.cpp)
add_library(interpreter interpreter.cpp)
add_library(adt adt.cpp)

# Exemplos de executáveis
# add_executable(main main.cpp)
# target_link_libraries(main lexer parser ast symbols semantic runtime interpreter adt)
