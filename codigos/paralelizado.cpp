/* OpenMP sobre a mesma base otimizada do sequencial.
 * Compilar: make paralelizado
 * Executar: OMP_NUM_THREADS=4 ./paralelizado ../dados_exemplo.txt --sem-tabela
 */
#include "calculo_sequencial.hpp"
#include <omp.h>

Resultado calcular_paralelo(const std::vector<Pessoa>& pessoas) {
    Resultado r = preparar(pessoas);
    Estatisticas& e = r.estaturas;
    Estatisticas& p = r.pesos;
    const std::size_t ne = e.classes.size(), np = p.classes.size();
    const double n = static_cast<double>(pessoas.size());
    const int limite = std::max(1, std::min(omp_get_max_threads(),
                                           static_cast<int>(pessoas.size())));

    // Vetores separados por thread: nenhuma disputa na contagem.
    // Aloca antes da equipe para propagar normalmente erros de memoria.
    std::vector<std::vector<std::size_t>> locais_e(limite, std::vector<std::size_t>(ne, 0));
    std::vector<std::vector<std::size_t>> locais_p(limite, std::vector<std::size_t>(np, 0));
    double soma_e = 0, soma_p = 0, desvios_e = 0, desvios_p = 0;

    // A mesma equipe executa todas as etapas, para as duas variaveis.
    #pragma omp parallel num_threads(limite)
    {
        const int id = omp_get_thread_num();
        #pragma omp single nowait
        r.threads_usadas = omp_get_num_threads();

        #pragma omp for schedule(static)
        for (std::size_t i = 0; i < pessoas.size(); ++i) {
            ++locais_e[id][indice(e, pessoas[i].estatura)];
            ++locais_p[id][indice(p, pessoas[i].peso)];
        }

        // Cada classe tem um unico escritor; reduction combina as somas privadas.
        // nowait permite adiantar o peso sem esperar todas as classes de estatura.
        #pragma omp for reduction(+:soma_e) schedule(static) nowait
        for (std::size_t j = 0; j < ne; ++j) {
            std::size_t frequencia = 0;
            for (int t = 0; t < r.threads_usadas; ++t) frequencia += locais_e[t][j];
            e.classes[j].frequencia = frequencia;
            soma_e += ponto_medio(e, j) * frequencia;
        }
        #pragma omp for reduction(+:soma_p) schedule(static)
        for (std::size_t j = 0; j < np; ++j) {
            std::size_t frequencia = 0;
            for (int t = 0; t < r.threads_usadas; ++t) frequencia += locais_p[t][j];
            p.classes[j].frequencia = frequencia;
            soma_p += ponto_medio(p, j) * frequencia;
        }

        // As reducoes terminaram; uma thread publica as medias para toda a equipe.
        #pragma omp single
        {
            e.media = soma_e / n;
            p.media = soma_p / n;
        }

        #pragma omp for reduction(+:desvios_e) schedule(static) nowait
        for (std::size_t j = 0; j < ne; ++j) {
            const double diferenca = ponto_medio(e, j) - e.media;
            desvios_e += e.classes[j].frequencia * diferenca * diferenca;
        }
        #pragma omp for reduction(+:desvios_p) schedule(static)
        for (std::size_t j = 0; j < np; ++j) {
            const double diferenca = ponto_medio(p, j) - p.media;
            desvios_p += p.classes[j].frequencia * diferenca * diferenca;
        }
    }
    finalizar(e, desvios_e, n);
    finalizar(p, desvios_p, n);
    return r;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0]
                  << " dados.txt [--sem-tabela] [--mostrar-threads] [--automatico]\n";
        return 1;
    }
    bool mostrar_tabela = true, mostrar_threads = false, automatico = false;
    for (int i = 2; i < argc; ++i) {
        const std::string opcao = argv[i];
        if (opcao == "--sem-tabela") mostrar_tabela = false;
        else if (opcao == "--mostrar-threads") mostrar_threads = true;
        else if (opcao == "--automatico") automatico = true;
        else if (opcao == "--forcar-paralelo") automatico = false;
        else {
            std::cerr << "Opcao desconhecida: " << opcao << '\n';
            return 1;
        }
    }
    try {
        std::cout << "Processadores logicos disponiveis: " << omp_get_num_procs() << '\n'
                  << "Maximo de threads configurado: " << omp_get_max_threads() << '\n'
                  << "Ajuste dinamico de threads: " << (omp_get_dynamic() ? "ativo" : "inativo") << '\n';
        const double inicio_total = agora();
        const std::vector<Pessoa> pessoas = ler_dados(argv[1]);
        const double fim_leitura = agora();
        const bool usar_paralelo = !automatico || pessoas.size() >= 10000;
        const double inicio = agora();
        const Resultado r = usar_paralelo ? calcular_paralelo(pessoas) : calcular_sequencial(pessoas);
        const double fim = agora();

        std::cout << std::fixed << std::setprecision(2)
                  << "Analise " << (usar_paralelo ? "paralela" : "automatica sequencial")
                  << " de " << pessoas.size() << " pessoas\n"
                  << "Threads efetivamente usadas (frequencias/media/desvio): "
                  << r.threads_usadas << '/' << r.threads_usadas << '/' << r.threads_usadas << '\n';
        if (mostrar_threads) {
            for (int id = 0; id < r.threads_usadas; ++id) {
                std::cout << "Thread participante: " << id << '\n';
            }
        }
        mostrar("Estatura", "cm", r.estaturas, mostrar_tabela);
        mostrar("Peso", "kg", r.pesos, mostrar_tabela);
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
