/*
 * Desafio final: coeficiente de variacao para dados agrupados.
 * Versao paralelizada com OpenMP.
 *
 * Compilar: g++ -std=c++14 -O2 -Wall -Wextra -pedantic -fopenmp paralelizado.cpp -o paralelizado
 * Executar: ./paralelizado dados.txt
 * Entrada: exatamente 1000 linhas, cada uma com estatura (cm) e peso (kg).
 * Use ponto como separador decimal, por exemplo: 172.5 68.2
 */

 /*
 
 Explicações prévias:

 Devido o uso de 1000 valores de estatura e peso foi decidido o uso
 do tipo long double para evitar problemas de precisão.
 */

#include <time.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <omp.h>
#include <utility>
#include <algorithm>

// Quantidade exigida pelo enunciado.
// Relogio monotonicamente crescente, em segundos.
static double agora() {
    timespec t{};
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        throw std::runtime_error("Nao foi possivel consultar o relogio.");
    }
    return t.tv_sec + t.tv_nsec * 1e-9;
}

constexpr std::size_t TOTAL_PESSOAS = 1000;

struct Pessoa {
    long double estatura;
    long double peso;
};

struct Estatisticas {
    // Numero da classe -> quantidade de pessoas.
    std::map<long double, std::size_t> frequencias;
    int threads_frequencias = 0;
    int threads_media = 0;
    int threads_desvio = 0;
    long double largura;
    long double media;
    long double desvio_padrao;
    long double coeficiente_variacao;
};

// Leitura e validacao das 1000 pessoas.
std::vector<Pessoa> ler_dados(const std::string& caminho) {
    std::ifstream arquivo(caminho);

    if (!arquivo) {
        throw std::runtime_error("Nao foi possivel abrir: " + caminho);
    }

    std::vector<Pessoa> pessoas;

    pessoas.reserve(TOTAL_PESSOAS);

    std::string linha;

    std::size_t numero_linha = 0;

    while (std::getline(arquivo, linha)) {
        ++numero_linha;

        std::istringstream entrada(linha);

        Pessoa pessoa{};

        std::string extra;

        // Cada linha deve conter apenas estatura e peso positivos.
        if (!(entrada >> pessoa.estatura >> pessoa.peso) || (entrada >> extra) ||
            !std::isfinite(pessoa.estatura) || !std::isfinite(pessoa.peso) ||
            pessoa.estatura <= 0 || pessoa.peso <= 0) {
            throw std::runtime_error("Linha " + std::to_string(numero_linha) +
                                     ": informe dois numeros positivos e finitos.");
        }

        if (pessoas.size() == TOTAL_PESSOAS) {
            throw std::runtime_error("O arquivo possui mais de 1000 pessoas.");
        }

        pessoas.push_back(pessoa);
    }

    if (arquivo.bad()) {
        throw std::runtime_error("Falha durante a leitura do arquivo.");
    }

    if (pessoas.size() != TOTAL_PESSOAS) {
        throw std::runtime_error("Esperadas 1000 pessoas; recebidas " +
                                 std::to_string(pessoas.size()) + ".");
    }

    return pessoas;
}

// Vetores privados evitam disputar um contador a cada pessoa.
Estatisticas calcular(const std::vector<Pessoa>& pessoas,
                     long double Pessoa::*variavel, long double largura,
                     bool forcar_paralelo) {
    Estatisticas resultado{};
    resultado.largura = largura;
    resultado.threads_frequencias = 1;
    resultado.threads_media = 1;
    resultado.threads_desvio = 1;

    // Calcula a classe uma unica vez por pessoa e identifica a faixa ocupada.
    std::vector<long double> classes(pessoas.size());
    long double menor = std::floor((pessoas.front().*variavel) / largura);
    long double maior = menor;
    for (std::size_t i = 0; i < pessoas.size(); ++i) {
        classes[i] = std::floor((pessoas[i].*variavel) / largura);
        menor = std::min(menor, classes[i]);
        maior = std::max(maior, classes[i]);
    }

    const long double faixa = maior - menor + 1;
    // Evita alocacao excessiva para dados muito esparsos ou extremos.
    if (!std::isfinite(faixa) || faixa > 65536) {
        for (long double classe : classes) {
            ++resultado.frequencias[classe];
        }
    } else {
        const std::size_t quantidade = static_cast<std::size_t>(faixa);
        std::vector<std::size_t> frequencias(quantidade, 0);

        // Para 1000 pessoas, iniciar equipes costuma custar mais que contar.
        const bool usar_paralelo = forcar_paralelo || pessoas.size() >= 10000;
        if (usar_paralelo) {
            const int limite = std::max(1, std::min(
                omp_get_max_threads(), static_cast<int>(pessoas.size())));
            // Cada vetor possui sua propria alocacao, sem contadores compartilhados.
            std::vector<std::vector<std::size_t>> locais(
                limite, std::vector<std::size_t>(quantidade, 0));

            // Uma unica equipe por variavel, em vez de tres equipes.
            #pragma omp parallel num_threads(limite)
            {
                const int id = omp_get_thread_num();
                #pragma omp single nowait
                resultado.threads_frequencias = omp_get_num_threads();

                #pragma omp for schedule(static)
                for (std::size_t i = 0; i < classes.size(); ++i) {
                    const std::size_t indice = static_cast<std::size_t>(classes[i] - menor);
                    ++locais[id][indice];
                }
            }

            // Poucas classes: combinar sequencialmente evita novas barreiras.
            for (int id = 0; id < resultado.threads_frequencias; ++id) {
                for (std::size_t j = 0; j < quantidade; ++j) {
                    frequencias[j] += locais[id][j];
                }
            }
        } else {
            for (long double classe : classes) {
                ++frequencias[static_cast<std::size_t>(classe - menor)];
            }
        }

        // Mantem o formato de saida anterior, sem usar mapa no laco principal.
        for (std::size_t j = 0; j < quantidade; ++j) {
            if (frequencias[j] != 0) {
                resultado.frequencias[menor + j] = frequencias[j];
            }
        }
    }

    // Ha poucas classes: as duas somas sao mais baratas sem equipes adicionais.
    const long double n = static_cast<long double>(pessoas.size());
    long double soma_ponderada = 0;
    for (const auto& classe : resultado.frequencias) {
        const long double ponto_medio = (classe.first + 0.5L) * largura;
        soma_ponderada += ponto_medio * classe.second;
    }
    resultado.media = soma_ponderada / n;

    // Mantem duas passagens para evitar cancelamento numerico na variancia.
    long double soma_desvios_quadrados = 0;
    for (const auto& classe : resultado.frequencias) {
        const long double ponto_medio = (classe.first + 0.5L) * largura;
        const long double diferenca = ponto_medio - resultado.media;
        soma_desvios_quadrados += classe.second * diferenca * diferenca;
    }
    resultado.desvio_padrao = std::sqrt(soma_desvios_quadrados / n);
    resultado.coeficiente_variacao = resultado.desvio_padrao / resultado.media * 100;
    if (!std::isfinite(resultado.media) || !std::isfinite(resultado.desvio_padrao) ||
        !std::isfinite(resultado.coeficiente_variacao)) {
        throw std::runtime_error("Valores grandes demais para calcular as estatisticas.");
    }
    return resultado;
}

// Diagnostico fora do tempo dos calculos; nao cria uma equipe adicional.
void mostrar_ids(const char* variavel, int threads) {
    for (int id = 0; id < threads; ++id) {
        std::cout << "Thread participante (" << variavel << "): " << id << '\n';
    }
}

// Imprime os resultados de forma organizada.

void mostrar(const std::string& nome, // Nome da variavel
             const std::string& unidade, // Unidade de medida da variavel
             const Estatisticas& resultado // Resultado do calculo das estatisticas
            ) {

    // Imprime o nome da variavel, a largura da classe e o cabecalho da tabela.

    std::cout << "\n" << nome << " - classes de " << resultado.largura
              << " " << unidade << "\n"
              << "Intervalo [inferior, superior) | Ponto medio | Frequencia\n";
    
    // Equipes reais de cada etapa; podem variar com ajuste dinamico ativo.
    std::cout << "Threads efetivamente usadas (frequencias/media/desvio): "
              << resultado.threads_frequencias << "/" << resultado.threads_media
              << "/" << resultado.threads_desvio << '\n';

    // Itera sobre cada classe e sua frequencia, imprimindo os resultados.

    for (const auto& classe : resultado.frequencias) {

        /* 
            Essas três linhas calculam o início, o fim 
            e o centro do intervalo de uma classe.
        */

        const long double inferior = classe.first * resultado.largura;
        const long double superior = (classe.first + 1) * resultado.largura;
        const long double ponto_medio = (classe.first + 0.5L) * resultado.largura;
        std::cout << "[" << inferior << ", " << superior << ") | "
                  << ponto_medio << " | " << classe.second << '\n';
    }
    std::cout << "Media agrupada: " << resultado.media << " " << unidade << '\n'
              << "Desvio padrao populacional: " << resultado.desvio_padrao
              << " " << unidade << '\n'
              << "Coeficiente de variacao: " << resultado.coeficiente_variacao
              << "%\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0]
                  << " dados.txt [--mostrar-threads] [--forcar-paralelo]\n";
        return 1;
    }
    bool mostrar_threads = false;
    bool forcar_paralelo = false;
    for (int i = 2; i < argc; ++i) {
        const std::string opcao = argv[i];
        if (opcao == "--mostrar-threads") mostrar_threads = true;
        else if (opcao == "--forcar-paralelo") forcar_paralelo = true;
        else {
            std::cerr << "Opcao desconhecida: " << opcao << '\n';
            return 1;
        }
    }
    // Recursos disponiveis e configuracao do ambiente OpenMP.
    const int processadores = omp_get_num_procs();
    const int max_threads = omp_get_max_threads();
    std::cout << "Processadores logicos disponiveis: " << processadores << '\n'
              << "Maximo de threads configurado: " << max_threads << '\n'
              << "Ajuste dinamico de threads: " << (omp_get_dynamic() ? "ativo" : "inativo") << '\n';
    std::cout << "Modo: " << (forcar_paralelo ? "paralelo solicitado" : "automatico; paralelo a partir de 10000 pessoas") << '\n';

    try {
        const double inicio_total = agora();
        const std::vector<Pessoa> pessoas = ler_dados(argv[1]);
        const double fim_leitura = agora();

        // Mede agrupamento e calculos, sem leitura nem impressao.
        const double inicio = agora();
        const Estatisticas estaturas = calcular(pessoas, &Pessoa::estatura, 8, forcar_paralelo);
        const Estatisticas pesos = calcular(pessoas, &Pessoa::peso, 4, forcar_paralelo);
        const double fim = agora();

        if (mostrar_threads) {
            mostrar_ids("estatura", estaturas.threads_frequencias);
            mostrar_ids("peso", pesos.threads_frequencias);
        }

        std::cout << std::fixed << std::setprecision(2)
                  << "Analise paralelizada de " << pessoas.size() << " pessoas\n"
                  << "Calculos baseados nos pontos medios das classes.\n";
        mostrar("Estatura", "cm", estaturas);
        mostrar("Peso", "kg", pesos);

        // Inclui a escrita dos resultados; exclui a impressao das metricas abaixo.
        std::cout.flush();
        const double fim_total = agora();

        std::cout << std::setprecision(6)
                  << "\nTempo de leitura (s): " << fim_leitura - inicio_total << '\n'
                  << "Tempo dos calculos (s): " << fim - inicio << '\n'
                  << "Tempo total de execucao (s): " << fim_total - inicio_total << '\n';
    } catch (const std::exception& erro) {
        std::cerr << "Erro: " << erro.what() << '\n';
        return 1;
    }
    return 0;
}
