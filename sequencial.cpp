/*
 * Desafio final: coeficiente de variacao para dados agrupados.
 * Versao inteiramente sequencial, sem OpenMP.
 *
 * Compilar: g++ -std=c++14 -O2 -Wall -Wextra -pedantic sequencial.cpp -o sequencial
 * Executar: ./sequencial dados.txt
 * Entrada: exatamente 1000 linhas, cada uma com estatura (cm) e peso (kg).
 * Use ponto como separador decimal, por exemplo: 172.5 68.2
 */

 /*
 
 Explicações prévias:

 Devido o uso de 1000 valores de estatura e peso foi decidido o uso
 do tipo long double para evitar problemas de precisão.

 
 
 
 
 
 
 */

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Quantidade exigida pelo enunciado.
constexpr std::size_t TOTAL_PESSOAS = 1000;

struct Pessoa {
    long double estatura;
    long double peso;
};

struct Estatisticas {
    // Numero da classe -> quantidade de pessoas.
    std::map<long double, std::size_t> frequencias;
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

// variavel seleciona Pessoa::estatura ou Pessoa::peso.
Estatisticas calcular(const std::vector<Pessoa>& pessoas, // Dados das pessoas; recebe sem copiar e sem permitir alteracoes.
                     long double Pessoa::*variavel,  // Campo que sera analisado: Pessoa::estatura ou Pessoa::peso.
                     long double largura // Tamanho de cada classe: 8 cm para estatura ou 4 kg para peso.
                    ) {

    // Inicializa o resultado com a largura da classe.
    Estatisticas resultado{};
    resultado.largura = largura;

    // Agrupa os valores em intervalos [inferior, superior).
    for (const Pessoa& pessoa : pessoas) {

        /*
            pessoa.*variavel: acessa a estatura ou o peso, conforme o campo escolhido.
            largura: divide o valor pelo tamanho do intervalo.
            std::floor(...): arredonda o resultado para baixo.
            classe: guarda o número da classe.
            const: impede alterar esse número depois.

            exemplo: 170 / 8 = 21,25
            floor(21,25) = 21
            Classe 21 → de 168 cm até menos de 176 cm.
        */

        const long double classe = std::floor((pessoa.*variavel) / largura);
        ++resultado.frequencias[classe];
    }

    // Media ponderada pelos pontos medios das classes.

    /*
    
    pessoas.size() retorna a quantidade de pessoas.
    static_cast<long double>(...) converte essa quantidade para long double.
    n guarda o total, que será usado depois para dividir a soma.
    const impede alterar n.

    */
    const long double n = static_cast<long double>(pessoas.size());

    //Inicializa a soma ponderada com zero.
    long double soma_ponderada = 0;

    /*  
        Itera sobre cada classe e sua frequência.
        Cria um acumulador, inicialmente zero, para
        somar ponto médio × frequência.
    */

    for (const auto& classe : resultado.frequencias) {

        /* 
            Calcula o centro do intervalo. 
            Exemplo: para a classe 21, com largura 8 cm:

            Intervalo: [168, 176)
            Ponto médio: (21 + 0,5) × 8 = 172 cm
        */

        const long double ponto_medio = (classe.first + 0.5L) * largura;
        soma_ponderada += ponto_medio * classe.second;
    }

    // Calcula a média dividindo a soma ponderada pelo total de pessoas.

    resultado.media = soma_ponderada / n;

    // Desvio padrao populacional: divide por N.
    long double soma_desvios_quadrados = 0;

    // Percorre as classes para calcular a dispersao em torno da media.

    for (const auto& classe : resultado.frequencias) {

        // Calcula o centro do intervalo.

        const long double ponto_medio = (classe.first + 0.5L) * largura;

        // Calcula quanto o ponto medio se afasta da media geral.

        const long double diferenca = ponto_medio - resultado.media;

        // Acumula a diferenca ao quadrado, ponderada pela frequencia.

        soma_desvios_quadrados += classe.second * diferenca * diferenca;
    }

    // Calcula o desvio padrao populacional.

    resultado.desvio_padrao = std::sqrt(soma_desvios_quadrados / n);

    // Dispersao em porcentagem da media.
    resultado.coeficiente_variacao = (resultado.desvio_padrao / resultado.media) * 100;

    // Rejeita resultados infinitos ou invalidos.
    if (!std::isfinite(resultado.media) ||
        !std::isfinite(resultado.desvio_padrao) ||
        !std::isfinite(resultado.coeficiente_variacao)) {
        throw std::runtime_error("Valores grandes demais para calcular as estatisticas.");
    }

    return resultado;
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
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " dados.txt\n";
        return 1;
    }

    try {
        const std::vector<Pessoa> pessoas = ler_dados(argv[1]);

        // Mede agrupamento e calculos, sem leitura nem impressao.
        const auto inicio = std::chrono::steady_clock::now();
        const Estatisticas estaturas = calcular(pessoas, &Pessoa::estatura, 8);
        const Estatisticas pesos = calcular(pessoas, &Pessoa::peso, 4);
        const auto fim = std::chrono::steady_clock::now();
        const std::chrono::duration<double> tempo = fim - inicio;

        std::cout << std::fixed << std::setprecision(6)
                  << "Analise sequencial de " << pessoas.size() << " pessoas\n"
                  << "Calculos baseados nos pontos medios das classes.\n";
        mostrar("Estatura", "cm", estaturas);
        mostrar("Peso", "kg", pesos);
        std::cout << "\nTempo dos calculos (s): " << tempo.count() << '\n';
    } catch (const std::exception& erro) {
        std::cerr << "Erro: " << erro.what() << '\n';
        return 1;
    }
    return 0;
}
