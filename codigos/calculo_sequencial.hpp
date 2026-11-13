#ifndef DESAFIO_CALCULO_SEQUENCIAL_HPP
#define DESAFIO_CALCULO_SEQUENCIAL_HPP

#include "comum.hpp"

inline Resultado calcular_sequencial(const std::vector<Pessoa>& pessoas) {
    // Prepara classes de 8 cm para estatura e de 4 kg para peso.
    Resultado r = preparar(pessoas);
    Estatisticas& e = r.estaturas;
    Estatisticas& p = r.pesos;

    // Conta as duas variaveis em uma passagem, sem mapas ou vetores temporarios.
    for (const Pessoa& pessoa : pessoas) {
        ++e.classes[indice(e, pessoa.estatura)].frequencia;
        ++p.classes[indice(p, pessoa.peso)].frequencia;
    }

    // Cada ponto medio contribui com um peso igual a frequencia da classe.
    double soma_e = 0, soma_p = 0;
    for (std::size_t i = 0; i < e.classes.size(); ++i) {
        soma_e += ponto_medio(e, i) * e.classes[i].frequencia;
    }
    for (std::size_t i = 0; i < p.classes.size(); ++i) {
        soma_p += ponto_medio(p, i) * p.classes[i].frequencia;
    }
    const double n = static_cast<double>(pessoas.size());
    e.media = soma_e / n;
    p.media = soma_p / n;

    // Com as medias prontas, soma os desvios quadraticos ponderados.
    // Calcular os desvios diretamente evita subtrair somas muito grandes.
    double desvios_e = 0, desvios_p = 0;
    for (std::size_t i = 0; i < e.classes.size(); ++i) {
        const double diferenca = ponto_medio(e, i) - e.media;
        desvios_e += e.classes[i].frequencia * diferenca * diferenca;
    }
    for (std::size_t i = 0; i < p.classes.size(); ++i) {
        const double diferenca = ponto_medio(p, i) - p.media;
        desvios_p += p.classes[i].frequencia * diferenca * diferenca;
    }
    // Calcula o desvio padrao populacional e o CV de cada variavel.
    finalizar(e, desvios_e, n);
    finalizar(p, desvios_p, n);
    return r;
}

#endif
