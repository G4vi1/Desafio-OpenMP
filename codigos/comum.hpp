#ifndef DESAFIO_COMUM_HPP
#define DESAFIO_COMUM_HPP

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <time.h>
#include <vector>

constexpr std::size_t TOTAL_PESSOAS = 1000;

struct Pessoa {
    double estatura;
    double peso;
};

struct Classe {
    double numero;
    std::size_t frequencia = 0;
};

struct Estatisticas {
    double largura;
    double primeira;
    bool densa;
    std::vector<Classe> classes;
    double media = 0;
    double desvio_padrao = 0;
    double coeficiente_variacao = 0;
};

struct Resultado {
    Estatisticas estaturas;
    Estatisticas pesos;
    int threads_usadas = 1;
};

inline double agora() {
    timespec t{};
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        throw std::runtime_error("Nao foi possivel consultar o relogio.");
    }
    return t.tv_sec + t.tv_nsec * 1e-9;
}

// Converte diretamente o texto, sem criar um istringstream por linha.
inline std::vector<Pessoa> ler_dados(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if (!arquivo) throw std::runtime_error("Nao foi possivel abrir: " + caminho);

    std::vector<Pessoa> pessoas;
    pessoas.reserve(TOTAL_PESSOAS);
    std::string linha;
    std::size_t numero_linha = 0;
    while (std::getline(arquivo, linha)) {
        ++numero_linha;
        const char* inicio = linha.c_str();
        char* fim_estatura = nullptr;
        char* fim_peso = nullptr;
        errno = 0;
        const double estatura = std::strtod(inicio, &fim_estatura);
        const bool estatura_valida = fim_estatura != inicio && errno != ERANGE;
        const bool separador = std::isspace(static_cast<unsigned char>(*fim_estatura));
        errno = 0;
        const double peso = std::strtod(fim_estatura, &fim_peso);
        const bool peso_valido = fim_peso != fim_estatura && errno != ERANGE;
        while (std::isspace(static_cast<unsigned char>(*fim_peso))) ++fim_peso;

        if (!estatura_valida || !separador || !peso_valido ||
            fim_peso != inicio + linha.size() || !std::isfinite(estatura) ||
            !std::isfinite(peso) || estatura <= 0 || peso <= 0) {
            throw std::runtime_error("Linha " + std::to_string(numero_linha) +
                                     ": informe dois numeros positivos e finitos.");
        }
        if (pessoas.size() == TOTAL_PESSOAS) {
            throw std::runtime_error("O arquivo possui mais de 1000 pessoas.");
        }
        pessoas.push_back({estatura, peso});
    }
    if (arquivo.bad()) throw std::runtime_error("Falha durante a leitura do arquivo.");
    if (pessoas.size() != TOTAL_PESSOAS) {
        throw std::runtime_error("Esperadas 1000 pessoas; recebidas " +
                                 std::to_string(pessoas.size()) + ".");
    }
    return pessoas;
}

// Prepara contadores contiguos, com os mesmos intervalos das versoes anteriores.
inline Estatisticas preparar_classes(const std::vector<Pessoa>& pessoas,
                                     double Pessoa::*campo, double largura,
                                     double minimo, double maximo) {
    Estatisticas e{};
    e.largura = largura;
    e.primeira = std::floor(minimo / largura);
    const double faixa = std::floor(maximo / largura) - e.primeira + 1;
    e.densa = std::isfinite(faixa) && faixa <= 65536;
    if (e.densa) {
        e.classes.resize(static_cast<std::size_t>(faixa));
        for (std::size_t i = 0; i < e.classes.size(); ++i) {
            e.classes[i].numero = e.primeira + i;
        }
    } else {
        // Faixa muito esparsa: guarda somente as classes existentes.
        std::vector<double> numeros;
        numeros.reserve(pessoas.size());
        for (const Pessoa& p : pessoas) numeros.push_back(std::floor((p.*campo) / largura));
        std::sort(numeros.begin(), numeros.end());
        numeros.erase(std::unique(numeros.begin(), numeros.end()), numeros.end());
        e.classes.reserve(numeros.size());
        for (double numero : numeros) e.classes.push_back({numero, 0});
    }
    return e;
}

inline Resultado preparar(const std::vector<Pessoa>& pessoas) {
    if (pessoas.empty()) throw std::runtime_error("Nao ha pessoas para analisar.");
    double min_e = pessoas[0].estatura, max_e = min_e;
    double min_p = pessoas[0].peso, max_p = min_p;
    // Uma passagem para os limites das duas variaveis.
    for (const Pessoa& p : pessoas) {
        min_e = std::min(min_e, p.estatura);
        max_e = std::max(max_e, p.estatura);
        min_p = std::min(min_p, p.peso);
        max_p = std::max(max_p, p.peso);
    }
    Resultado r{};
    r.estaturas = preparar_classes(pessoas, &Pessoa::estatura, 8, min_e, max_e);
    r.pesos = preparar_classes(pessoas, &Pessoa::peso, 4, min_p, max_p);
    return r;
}

inline std::size_t indice(const Estatisticas& e, double valor) {
    const double numero = std::floor(valor / e.largura);
    if (e.densa) return static_cast<std::size_t>(numero - e.primeira);
    const auto posicao = std::lower_bound(e.classes.begin(), e.classes.end(), numero,
        [](const Classe& classe, double chave) { return classe.numero < chave; });
    return static_cast<std::size_t>(posicao - e.classes.begin());
}

inline double ponto_medio(const Estatisticas& e, std::size_t i) {
    return (e.classes[i].numero + 0.5) * e.largura;
}

inline void finalizar(Estatisticas& e, double soma_quadrados, double n) {
    e.desvio_padrao = std::sqrt(soma_quadrados / n);
    e.coeficiente_variacao = e.desvio_padrao / e.media * 100;
    if (!std::isfinite(e.media) || !std::isfinite(e.desvio_padrao) ||
        !std::isfinite(e.coeficiente_variacao)) {
        throw std::runtime_error("Valores grandes demais para calcular as estatisticas.");
    }
}

inline void mostrar(const char* nome, const char* unidade, const Estatisticas& e,
                    bool mostrar_classes) {
    std::cout << '\n' << nome << " - classes de " << e.largura << ' ' << unidade << '\n';
    if (mostrar_classes) {
        std::cout << "Intervalo [inferior, superior) | Ponto medio | Frequencia\n";
        for (std::size_t i = 0; i < e.classes.size(); ++i) {
            const Classe& c = e.classes[i];
            if (c.frequencia == 0) continue;
            std::cout << '[' << c.numero * e.largura << ", "
                      << (c.numero + 1) * e.largura << ") | "
                      << ponto_medio(e, i) << " | " << c.frequencia << '\n';
        }
    }
    std::cout << "Media agrupada: " << e.media << ' ' << unidade << '\n'
              << "Desvio padrao populacional: " << e.desvio_padrao << ' ' << unidade << '\n'
              << "Coeficiente de variacao: " << e.coeficiente_variacao << "%\n";
}

#endif
