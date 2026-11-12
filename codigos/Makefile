CXX = g++
CXXFLAGS = -std=c++14 -O3 -Wall -Wextra -pedantic
DADOS ?= ../dados_exemplo.txt
THREADS ?= 4
MOSTRAR_THREADS ?= 0
FORCAR_PARALELO ?= 1
TABELA ?= 1
REPETICOES ?= 30
MAX_THREADS ?=
OPCAO_MAX = $(if $(MAX_THREADS),--max-threads "$(MAX_THREADS)",)

OPCAO_TABELA = $(if $(filter 0,$(TABELA)),--sem-tabela,)
OPCOES_OMP = $(if $(filter 1,$(MOSTRAR_THREADS)),--mostrar-threads,) $(if $(filter 0,$(FORCAR_PARALELO)),--automatico,)
CABECALHOS = comum.hpp calculo_sequencial.hpp

.PHONY: paralelizado all run run-sequencial run-paralelizado comparar clean validar-opcoes
all: sequencial paralelizado

sequencial: sequencial.cpp $(CABECALHOS)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"

# O codigo-fonte permanece aqui; o executavel fica em resultados/.
paralelizado: ../resultados/paralelizado

../resultados/paralelizado: paralelizado.cpp $(CABECALHOS)
	mkdir -p ../resultados
	$(CXX) $(CXXFLAGS) -fopenmp "$<" -o "$@"

validar-opcoes:
	@test -f "$(DADOS)" || { echo "Arquivo de dados nao encontrado: $(DADOS)"; exit 1; }
	@case "$(THREADS)" in ''|*[!0-9]*|0) echo "THREADS deve ser um inteiro positivo."; exit 1;; esac
	@case "$(TABELA):$(MOSTRAR_THREADS):$(FORCAR_PARALELO)" in [01]:[01]:[01]) ;; *) echo "TABELA, MOSTRAR_THREADS e FORCAR_PARALELO devem ser 0 ou 1."; exit 1;; esac

run: run-sequencial

run-sequencial: sequencial validar-opcoes
	./sequencial "$(DADOS)" $(OPCAO_TABELA)

run-paralelizado: paralelizado validar-opcoes
	OMP_NUM_THREADS=$(THREADS) OMP_DYNAMIC=FALSE ../resultados/paralelizado "$(DADOS)" $(OPCAO_TABELA) $(OPCOES_OMP)

# Compara de verdade: OpenMP ativo, mesmos dados, mesma saida e sem tabelas.
comparar: all validar-opcoes
	python3 ../scripts/comparar.py "$(DADOS)" --repeticoes "$(REPETICOES)" $(OPCAO_MAX)

clean:
	$(RM) sequencial ../resultados/paralelizado
