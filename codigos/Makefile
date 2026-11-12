CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -Wextra -pedantic

FONTE = sequencial.cpp
PROGRAMA = sequencial
DADOS ?= ../dados.txt

.PHONY: all run clean

all: $(PROGRAMA)

$(PROGRAMA): $(FONTE)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"

run: $(PROGRAMA)
	@test -f "$(DADOS)" || { echo "Arquivo de dados nao encontrado: $(DADOS)"; exit 1; }
	./$(PROGRAMA) "$(DADOS)"

clean:
	$(RM) $(PROGRAMA)
