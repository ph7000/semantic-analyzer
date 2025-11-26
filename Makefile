CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -g -I.

PARSER = bison
PARSERFLAGS = -Wall -Werror -d
LEXER = flex
LEXERFLAGS =

TARGET = semanticanalyzer

PARSER_SRC = parser.tab.cpp
PARSER_HDR = parser.tab.hpp
LEXER_SRC = lex.yy.c

OBJS = main.o scanner.o parser.o astnode.o semantic_analyzer.o

all: $(TARGET)

debug: PARSERFLAGS += -t -v
debug: LEXERFLAGS += -d
debug: CXXFLAGS += -DYYDEBUG=1
debug: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

parser.tab.cpp parser.tab.hpp: parser.y
	$(PARSER) $(PARSERFLAGS) -o $(PARSER_SRC) parser.y

lex.yy.c: lexer.l parser.tab.hpp
	$(LEXER) $(LEXERFLAGS) -o $(LEXER_SRC) lexer.l

parser.o: parser.tab.cpp parser.tab.hpp astnode.hpp data_type.hpp exception.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(PARSER_SRC)

scanner.o: lex.yy.c parser.tab.hpp exception.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(LEXER_SRC)

astnode.o: astnode.cpp astnode.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ astnode.cpp

semantic_analyzer.o: semantic_analyzer.cpp semantic_analyzer.hpp astnode.hpp exception.hpp data_type.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ semantic_analyzer.cpp

main.o: main.cpp astnode.hpp parser.tab.hpp exception.hpp semantic_analyzer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ main.cpp

clean:
	rm -f $(TARGET) $(PARSER_SRC) $(PARSER_HDR) $(LEXER_SRC) *.o parser.output
	rm -rf test/result

.PHONY: all debug clean help