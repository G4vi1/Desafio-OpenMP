CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -Wextra -pedantic

FONTE = sequencial.cpp
PROGRAMA = sequencial
DADOS ?= ../dados_exemplo.txt
THREADS ?= 4
MOSTRAR_THREADS ?= 0
FORCAR_PARALELO ?= 0

# MOSTRAR_THREADS=1 ativa a impressao ordenada dos IDs.
OPCAO_THREADS = $(if $(filter 1,$(MOSTRAR_THREADS)),--mostrar-threads,) $(if $(filter 1,$(FORCAR_PARALELO)),--forcar-paralelo,)

.PHONY: all run clean

all: $(PROGRAMA) paralelizado

$(PROGRAMA): $(FONTE)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"

run: $(PROGRAMA)
	@test -f "$(DADOS)" || { echo "Arquivo de dados nao encontrado: $(DADOS)"; exit 1; }
	./$(PROGRAMA) "$(DADOS)"

clean:
	$(RM) $(PROGRAMA) paralelizado

# Compila a versao paralela com suporte a OpenMP.
paralelizado: paralelizado.cpp
	$(CXX) $(CXXFLAGS) -fopenmp "$<" -o "$@"

.PHONY: run-paralelizado
run-paralelizado: paralelizado
	@test -f "$(DADOS)" || { echo "Arquivo de dados nao encontrado: $(DADOS)"; exit 1; }
	@case "$(THREADS)" in ''|*[!0-9]*|0) echo "THREADS deve ser um inteiro positivo."; exit 1;; esac
	@test "$(MOSTRAR_THREADS)" = 0 -o "$(MOSTRAR_THREADS)" = 1 || { echo "MOSTRAR_THREADS deve ser 0 ou 1."; exit 1; }
	OMP_NUM_THREADS=$(THREADS) OMP_DYNAMIC=FALSE ./paralelizado "$(DADOS)" $(OPCAO_THREADS)

# Atalho explicito para a execucao sequencial.
.PHONY: run-sequencial
run-sequencial: run
