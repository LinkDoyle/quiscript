antlr_target(XMLGrammarLexer grammar/XMLLexer.g4
             LEXER PACKAGE antlrxmlparser)
antlr_target(XMLGrammarParser grammar/XMLParser.g4
             PARSER LISTENER VISITOR
             PACKAGE antlrxmlparser
             DEPENDS_ANTLR XMLGrammarLexer)

include_directories(${ANTLR4_INCLUDE_DIRS})
include_directories(${ANTLR_XMLGrammarLexer_OUTPUT_DIR})
include_directories(${ANTLR_XMLGrammarParser_OUTPUT_DIR})
source_group("antlr4cpp" FILES
  ${ANTLR_XMLGrammarLexer_CXX_OUTPUTS}
  ${ANTLR_XMLGrammarParser_CXX_OUTPUTS}
)
 
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_LIST
  "main.cpp"
)

add_executable(antlr4_helloworld ${SOURCE_LIST}
  ${ANTLR_XMLGrammarLexer_CXX_OUTPUTS}
  ${ANTLR_XMLGrammarParser_CXX_OUTPUTS}
)
set_target_properties(antlr4_helloworld PROPERTIES FOLDER "Demo")
target_link_libraries(antlr4_helloworld antlr4_static)
