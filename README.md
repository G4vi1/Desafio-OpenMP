# Desafio OpenMP — sequencial otimizado e paralelo

Calcula média agrupada, desvio padrão populacional e coeficiente de variação
(CV) de estaturas e pesos de exatamente 1.000 pessoas. As duas versões usam
`double`, os mesmos dados, as mesmas regras de classe e a mesma leitura validada.

## Teste rápido

Na pasta principal do projeto:

```bash
cd ~/Downloads/Desafio-OpenMP
make -C codigos
make -C codigos run-sequencial
make -C codigos run-paralelizado THREADS=4
```

O padrão é `dados_exemplo.txt`, com 1.000 registros **fictícios**. O comando
`run-paralelizado` agora executa OpenMP de verdade por padrão. Para usar
explicitamente o modo automático anterior, que seleciona o sequencial nesta
entrada pequena:

```bash
make -C codigos run-paralelizado FORCAR_PARALELO=0
```

Se já estiver dentro de `codigos`, retire `-C codigos` dos comandos.

## Executar com Docker

O Docker instala GCC, Make e Python 3 em Ubuntu 24.04. Os executáveis são
compilados dentro do container com o mesmo `-O3` das duas versões. Não é
necessário instalar as ferramentas de C++ diretamente no computador.

Execute os comandos seguintes na **pasta principal**:

```bash
cd ~/Downloads/Desafio-OpenMP

# Construir a imagem com as permissoes do seu usuario Linux:
docker build --build-arg USER_UID="$(id -u)" --build-arg USER_GID="$(id -g)" -t desafio-openmp .

# Preparar a pasta em que os resultados serao salvos:
mkdir -p resultados
```

### Comparação completa

```bash
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados desafio-openmp
```

Sem comando adicional, compara o sequencial e OpenMP com 1, 2, 4, 8, 16… até
o máximo detectado dentro do container, com 30 repetições por configuração.
Mostra o resumo no terminal e salva CSV/TXT na pasta `resultados` do computador.

Para mudar as repetições ou limitar o máximo:

```bash
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp python3 scripts/comparar.py --repeticoes 40 --max-threads 8
```

Remova `--max-threads 8` para testar até o máximo disponível. A montagem mantém
os resultados no computador mesmo após o container ser removido por `--rm`.

### Execuções individuais

```bash
# Sequencial, salvando um TXT:
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp bash scripts/sequencial.sh

# Paralelo com 4 threads, salvando um TXT:
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp bash scripts/paralelizado.sh dados_exemplo.txt 4

# Paralelo sem tabelas ou IDs, mostrando apenas resumo e metricas:
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp make -C codigos run-paralelizado THREADS=4 TABELA=0 MOSTRAR_THREADS=0
```

### Usar seus dados

Coloque `dados.txt` na pasta principal e monte o arquivo adicionalmente:

```bash
docker run --rm \
  --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  --mount type=bind,src="$(pwd)/dados.txt",dst=/lab/dados.txt,readonly \
  desafio-openmp python3 scripts/comparar.py dados.txt --repeticoes 40
```

O arquivo precisa existir e obedecer ao formato de 1.000 pessoas descrito abaixo.
Para abrir um terminal dentro do ambiente:

```bash
docker run --rm -it --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados desafio-openmp bash
```

Dentro dele, a pasta principal é `/lab`. Você pode executar `make -C codigos
run-sequencial` ou `make -C codigos run-paralelizado THREADS=4`.

A cada início o ambiente recompila antes de executar o comando escolhido. Isso
recria `resultados/paralelizado` mesmo quando a montagem oculta o executável da
imagem e evita reutilizar binários do computador. A compilação fica fora dos
tempos medidos. O código vem da imagem: após alterar fontes, scripts ou dados de
exemplo, repita o `docker build`.

Se o Docker exibir erro de acesso a `/var/run/docker.sock`, use `sudo docker`
no lugar de `docker`, mantendo `USER_UID` e `USER_GID` com os valores do seu
usuário. Se o Docker não estiver ativo, inicie o serviço antes de executar.
O número de threads disponível depende dos processadores visíveis no container
ou na máquina virtual do Docker; use esse mesmo ambiente para comparar as versões.

Referências: [execução de containers](https://docs.docker.com/engine/containers/run/)
e [montagem de pastas](https://docs.docker.com/engine/storage/bind-mounts/).

## Entrada de dados

Cada linha deve conter estatura em cm e peso em kg, separados por espaço ou
TAB. Use ponto decimal, sem cabeçalho ou colunas extras. Por exemplo:

```text
170.50 68.20
182.00 84.70
159.30 55.00
```

Essas linhas apenas ilustram o formato: o arquivo completo precisa de 1.000
registros positivos e finitos. Linhas inválidas, vazias, quantidade incorreta
e dados extras são rejeitados.

Para seus próprios dados, coloque `dados.txt` na pasta principal e execute:

```bash
make -C codigos run-sequencial DADOS=../dados.txt
make -C codigos run-paralelizado THREADS=4 DADOS=../dados.txt
```

`DADOS` é interpretado a partir de `codigos` nos comandos do Makefile.

## Métricas sem imprimir as tabelas

```bash
make -C codigos run-sequencial TABELA=0
make -C codigos run-paralelizado THREADS=4 TABELA=0
```

`TABELA=0` oculta os intervalos, mas mantém médias, desvios, CVs e tempos.
Para mostrar também os IDs participantes da equipe:

```bash
make -C codigos run-paralelizado THREADS=4 MOSTRAR_THREADS=1
```

Os IDs são listados após os cálculos, fora da medição dessa etapa. As métricas
OpenMP informam processadores lógicos, máximo configurado, ajuste dinâmico e
tamanho real da equipe. O Makefile desativa o ajuste dinâmico nas execuções.

## Script da versão paralela e TXT

Na pasta principal:

```bash
# Dados de exemplo, 4 threads:
bash scripts/paralelizado.sh

# Dados de exemplo, 8 threads:
bash scripts/paralelizado.sh dados_exemplo.txt 8

# Dados proprios, 4 threads:
bash scripts/paralelizado.sh dados.txt 4

# Sem tabela, com IDs participantes:
TABELA=0 MOSTRAR_THREADS=1 bash scripts/paralelizado.sh dados_exemplo.txt 4
```

Na pasta `codigos`, use:

```bash
bash ../scripts/paralelizado.sh ../dados_exemplo.txt 4
```

O primeiro argumento é o arquivo e o segundo é a quantidade de threads. Sem
argumentos usa o exemplo e 4 threads (ou `OMP_NUM_THREADS`, se estiver definida).
O caminho dos dados é relativo à pasta atual. O script compila, executa OpenMP
com ajuste dinâmico desativado, mostra a saída e salva um TXT exclusivo com
nome `resultados/paralelizado_4threads_DATA_HORA_IDENTIFICADOR.txt`.

O **executável** agora fica em `resultados/paralelizado`. O código-fonte continua
em `codigos/paralelizado.cpp`, acompanhado pelo Git. Por exemplo, na pasta
principal, uma execução direta é:

```bash
OMP_NUM_THREADS=4 OMP_DYNAMIC=FALSE ./resultados/paralelizado dados_exemplo.txt
```

## Comparação repetida até o máximo disponível

Na pasta principal:

```bash
make -C codigos comparar REPETICOES=40
```

Ou chame o Python diretamente (ele também compila antes dos testes):

```bash
python3 scripts/comparar.py --repeticoes 40
python3 scripts/comparar.py dados.txt --repeticoes 40
```

Na pasta `codigos`:

```bash
make comparar REPETICOES=40
```

Requer Python 3. Compara o sequencial com OpenMP em **1, 2, 4, 8, 16, 32…**
threads até o máximo disponível. Nesta máquina, com 16 processadores lógicos,
a sequência é **sequencial, paralelo 1, 2, 4, 8 e 16 threads**. Se o máximo não
for potência de dois, ele também entra: para 12, testa 1, 2, 4, 8 e 12.

O máximo considera a afinidade de CPU do processo (ou a contagem de CPUs do
sistema quando a afinidade não está disponível) e `OMP_THREAD_LIMIT`, se
definido. É um teto de recursos lógicos, não uma promessa de ganho de velocidade.
Cada execução confere a quantidade real de threads usada nas três etapas.

Para limitar os testes abaixo do máximo:

```bash
make -C codigos comparar REPETICOES=40 MAX_THREADS=8
python3 scripts/comparar.py --repeticoes 40 --max-threads 8
```

`THREADS=4` continua controlando uma execução individual pelo Makefile;
`MAX_THREADS=8` limita a sequência da comparação. São parâmetros diferentes.

A comparação realiza cada configuração 40 vezes, embaralha a ordem e mantém
tabelas e IDs desativados. Confere as seis estatísticas exibidas contra uma
execução sequencial de referência e interrompe se elas divergirem. O paralelo
sempre usa OpenMP de verdade, sem o modo automático sequencial.

Em `resultados/`, salva arquivos exclusivos:

- `comparacao_*.csv`: tempos de leitura, cálculos e total de cada execução,
  versão, threads solicitadas e efetivamente usadas.
- `comparacao_*.txt`: medianas em segundos, speedup e eficiência.

**Speedup = tempo sequencial / tempo paralelo.** Acima de 1 indica ganho;
abaixo de 1 indica perda. **Eficiência = speedup / threads × 100%.** Os tempos
são apresentados com seis casas decimais; um cálculo arredondado para zero
produz `N/D` no speedup, evitando divisão por zero.

São processos novos: o tempo paralelo inclui a primeira criação da equipe.
O script não mede apenas uma equipe já aquecida. Com apenas 1.000 pessoas,
o custo do OpenMP pode superar o ganho, mesmo usando mais threads. Compare as
medianas dos cálculos; não atribua diferenças isoladas da leitura ao OpenMP.

## Gráficos para o futuro relatório PDF

Os scripts `scripts/dados_relatorio.py` e `scripts/gerar_graficos.py` analisam
uma coleta de cada vez. Não misturam CSVs de algoritmos, máquinas ou execuções
diferentes. Geram figuras estáticas em **PNG com 300 dpi** e **SVG vetorial**,
adequadas para inserir em um relatório. Ainda não geram o PDF final.

### Usar Docker (Matplotlib já incluído)

Reconstrua a imagem para incluir os scripts e a dependência nova:

```bash
docker build --build-arg USER_UID="$(id -u)" --build-arg USER_GID="$(id -g)" -t desafio-openmp .
```

Para coletar novas medições e depois gerar todos os gráficos, na pasta principal:

```bash
mkdir -p resultados
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp bash -c 'python3 scripts/comparar.py --repeticoes 40 && python3 scripts/gerar_graficos.py'
```

Para apenas gerar os gráficos da coleta mais recente, sem medir novamente:

```bash
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp python3 scripts/gerar_graficos.py
```

Para selecionar uma coleta específica, substitua o nome no exemplo:

```bash
docker run --rm --mount type=bind,src="$(pwd)/resultados",dst=/lab/resultados \
  desafio-openmp python3 scripts/gerar_graficos.py resultados/comparacao_NOME.csv
```

A inicialização do container recompila antes do comando, mas o gerador de
figuras não executa os programas nem substitui medições históricas. Ele usa os
CSV e snapshots registrados na coleta selecionada.

### Executar diretamente no Linux

Instale as dependências em um ambiente Python separado:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-graficos.txt

make -C codigos comparar REPETICOES=40
make -C codigos graficos
```

Se o módulo `venv` não estiver instalado no seu sistema, use o Docker acima
ou instale o pacote `python3-venv` da sua distribuição. Com o ambiente ativo,
também é possível selecionar um CSV:

```bash
python scripts/gerar_graficos.py resultados/comparacao_NOME.csv
# Equivalente pelo Makefile; CSV e relativo a codigos:
make -C codigos graficos CSV=../resultados/comparacao_NOME.csv
```

### Figuras geradas e relação com os critérios da entrega

| Arquivo | O que mostra | Uso no relatório |
|---|---|---|
| `01_tempo_calculos` | Medianas e intervalo P25–P75 dos cálculos | Comparar o algoritmo sequencial com OpenMP |
| `02_tempo_total` | Medianas e intervalo P25–P75 do total | Mostrar o efeito sobre o programa completo |
| `03_speedup` | Razão das medianas, referência 1 e ideal por threads | Identificar ganhos ou perdas reais |
| `04_eficiencia` | Speedup dividido pelas threads, em porcentagem | Discutir escalabilidade e custo de coordenação |
| `05_variacao_execucoes` | Boxplots com todas as medições e valores extremos | Mostrar estabilidade e variação entre testes |
| `06_composicao_tempo` | Participação de leitura, cálculo e restante no tempo médio | Discutir onde o tempo é gasto |
| `07_coeficientes_variacao` | CV de estatura e peso por configuração | Apresentar os resultados estatísticos obtidos |

Cada nome possui um `.png` e um `.svg`. Os eixos indicam quando a escala é
logarítmica: ela permite enxergar tanto o sequencial de poucos microssegundos
quanto execuções paralelas muito mais lentas. As hastes dos gráficos de tempo
são **quartis**, não intervalos de confiança. Nenhum tempo extremo é removido.
A referência ideal de speedup é apenas uma comparação visual, não uma promessa.

Os arquivos ficam em:

```text
resultados/graficos/comparacao_NOME/
  01_tempo_calculos.png
  01_tempo_calculos.svg
  ...
  07_coeficientes_variacao.png
  07_coeficientes_variacao.svg
  resumo_desempenho.csv
  estatisticas_obtidas.csv
  tempos_originais.csv
  manifesto_graficos.json
  guia_relatorio.md
  fontes/codigos/...
```

`guia_relatorio.md` organiza os materiais nos quatro critérios solicitados:

1. **Código-fonte completo:** snapshots dos `.cpp`, `.hpp` e Makefile usados na
   coleta, preservados em `fontes/codigos/`.
2. **Partes paralelizadas e justificativa:** descrição da equipe única, contagens
   privadas, `omp for`, reduções, `single`, `nowait` e dependências entre fases.
   Revise a descrição com os snapshots antes de finalizar o PDF.
3. **Resultados obtidos:** tabela com médias, desvios e CVs e o gráfico dos CVs.
4. **Análise de desempenho:** medianas, quartis, speedup, eficiência e legendas
   para interpretar as figuras, inclusive quando o paralelo perde desempenho.

O gráfico de composição usa **médias** dos componentes para que as parcelas
somem o total médio. O restante é `total − leitura − cálculos`: inclui saída e
outras operações, não é tempo medido exclusivamente da impressão nem de cada
fase OpenMP. Speedup e eficiência usam **medianas dos cálculos**. Gráficos não
substituem a análise e a explicação do código no texto do relatório.

### Dados e rastreabilidade

As novas comparações recompilam as duas versões com o mesmo compilador e
registram, além do CSV de tempos e resumo TXT:

- `comparacao_NOME_estatisticas.csv`: as seis estatísticas de cada execução.
- `comparacao_NOME_metadados.json`: sistema, CPU, compilador, configuração,
  método, tamanho e hash da entrada e hashes dos dados e fontes.
- `comparacao_NOME_fontes/`: cópia dos fontes da coleta para a entrega futura.

O gerador verifica os hashes e a correspondência entre repetições. Os CVs
possuem a precisão exibida pelo programa (duas casas decimais); sua igualdade
não prova igualdade em toda a precisão interna. Os tempos são arredondados a
microssegundos. O gerador não transforma essas medições em precisão adicional.
O exemplo fornecido continua sendo uma entrada de **dados fictícios**.

CSVs antigos sem estatísticas/metadados ainda geram os seis gráficos de
desempenho, com aviso. O gráfico dos CVs é omitido: o script não usa valores da
entrada atual para preencher uma coleta antiga. Para obtê-lo, faça uma nova
comparação. Uma coleta de teste com poucas repetições gera aviso; use 30 ou 40
repetições para a análise, sem interpretar isso como garantia estatística.

Para verificar as fórmulas e a validação dos dados:

```bash
python3 -m unittest discover -s tests -v
```

Referências de exportação e gráficos: [Matplotlib — savefig](https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.savefig.html)
e [Matplotlib — boxplot](https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.boxplot.html).

## O que foi otimizado

### Base sequencial

- Converte as linhas diretamente com `strtod`, evitando criar um
  `istringstream` por pessoa, mantendo as validações.
- Conta estatura e peso na mesma passagem.
- Usa vetores contíguos em vez de atualizações de `std::map`.
- Não cria um vetor temporário com a classe de cada pessoa.
- Usa uma passagem conjunta para encontrar os limites das duas variáveis.
- Compila com `-O3`, sem `-ffast-math`.

As classes começam em múltiplos de 8 cm e 4 kg e seguem `[inferior, superior)`.
Por exemplo, 170 cm pertence a `[168, 176)`, cujo ponto médio é 172 cm. Classes
vazias não são impressas. Faixas muito esparsas usam apenas os intervalos
ocupados, evitando alocação proporcional à distância entre os extremos.

### OpenMP

Uma única equipe executa as etapas das **duas variáveis**:

1. `omp for schedule(static)` distribui as pessoas e cada thread conta em
   vetores privados, sem `critical` ou `atomic`.
2. Dois `omp for` combinam as contagens por classe e fazem `reduction` das
   somas ponderadas. Cada contador final tem um único escritor.
3. `single` calcula as médias; a barreira implícita as disponibiliza à equipe.
4. Dois `omp for reduction` somam os desvios quadrados.

Os primeiros laços de estatura nas etapas 2 e 4 usam `nowait`, permitindo
adiantar o trabalho de peso. As esperas posteriores garantem a conclusão das
etapas antes de seus resultados serem usados. A preparação dos limites, a
alocação, a leitura, as raízes quadradas finais e a impressão são sequenciais.

O algoritmo mantém o cálculo em duas passagens (média e depois desvios) para
evitar a perda de precisão da fórmula baseada na diferença entre somas grandes.
As reduções podem mudar a ordem das adições em ponto flutuante, produzindo
pequenas diferenças nas últimas casas.

## Significado dos tempos

Todos são medidos com `clock_gettime(CLOCK_MONOTONIC)`:

- **Leitura:** abrir, ler, converter e validar a entrada.
- **Cálculos:** preparação, alocação dos contadores, contagem, média e desvio.
  No paralelo inclui criar a equipe, sincronizar e combinar resultados.
- **Total:** leitura, cálculos e impressão dos resultados; exclui compilação,
  inicialização do processo e impressão das próprias métricas. Na versão OpenMP,
  a apresentação inicial do ambiente ocorre antes deste cronômetro.

As estatísticas usam duas casas decimais. Tempos usam seis casas decimais, como na formatação original. Isso limita
a apresentação a microssegundos; não representa a resolução real do relógio.

## Gerar um TXT da versão sequencial

Na pasta principal:

```bash
bash scripts/sequencial.sh
# Ou, com dados próprios:
bash scripts/sequencial.sh dados.txt
```

Se estiver em `codigos`, use `bash ../scripts/sequencial.sh`.
O script mostra o resultado e salva um `.txt` exclusivo em `resultados/`.
O caminho informado ao script é relativo à pasta atual do terminal.

## Organização

```text
codigos/
  comum.hpp                 # Estruturas, leitura, classes e impressão
  calculo_sequencial.hpp    # Algoritmo sequencial usado como referência
  sequencial.cpp            # Execução e métricas sequenciais
  paralelizado.cpp          # Etapas OpenMP e métricas da equipe
  Makefile
scripts/
  sequencial.sh             # Executar e salvar a saída sequencial
  paralelizado.sh           # Executar OpenMP e salvar a saida
  comparar.py               # Coletar tempos, estatisticas, ambiente e fontes
  dados_relatorio.py        # Validar CSVs e calcular resumos
  gerar_graficos.py         # Exportar PNG/SVG e materiais para o relatorio
  docker-entrypoint.sh       # Preparar a execucao dentro do container
resultados/                 # Executavel paralelizado e relatorios; ignorados pelo Git
dados_exemplo.txt
```

O sequencial não depende de OpenMP. Compartilhar as rotinas comuns impede que
mudanças na leitura ou nos intervalos tornem a comparação entre versões injusta.

## Problemas comuns

- **Sem regra para o alvo:** na pasta principal use `make -C codigos ...`.
- **Nada a ser feito:** o executável está atualizado; use um alvo `run-...`.
- **Dados não encontrados:** confira o caminho; o arquivo fornecido se chama
  `dados_exemplo.txt`, não `dados.txt`.
- **Erro de linha ou quantidade:** confira as regras de entrada acima.
- **Reconstruir:** execute `make -C codigos clean` e `make -C codigos`.

Os executáveis e `resultados/` estão no `.gitignore`. Limpar com `make clean`
não remove dados de entrada nem relatórios.

O `make` gera `codigos/sequencial` e `resultados/paralelizado`. O `make clean`
remove apenas esses executáveis, preservando os relatórios TXT/CSV. Se você
apagar toda a pasta `resultados`, o próximo `make` a recria antes de compilar.
