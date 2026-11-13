/* Versao sequencial otimizada: nenhuma diretiva ou dependencia de OpenMP.
 * Compilar: make sequencial
 * Executar: ./sequencial ../dados_exemplo.txt [--sem-tabela]
 * Leitura, classes e saida compartilhadas estao em comum.hpp.
 */
#include "calculo_sequencial.hpp"


int main(int argc, char* argv[]) {
    // Exige o arquivo de entrada e aceita ocultar as tabelas de frequencia.
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--sem-tabela")) {
        std::cerr << "Uso: " << argv[0] << " dados.txt [--sem-tabela]\n";
        return 1;
    }
    try {
        // A leitura inclui a validacao dos 1.000 pares de estatura e peso.
        const double inicio_total = agora();
        const std::vector<Pessoa> pessoas = ler_dados(argv[1]);
        const double fim_leitura = agora();
        const double inicio = agora();
        const Resultado r = calcular_sequencial(pessoas);
        const double fim = agora();

        std::cout << std::fixed << std::setprecision(2)
                  << "Analise sequencial de " << pessoas.size() << " pessoas\n";
        mostrar("Estatura", "cm", r.estaturas, argc == 2);
        mostrar("Peso", "kg", r.pesos, argc == 2);
        // O tempo total inclui a saida dos resultados ate o flush do terminal.
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
