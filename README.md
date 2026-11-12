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
  comparar.py               # Comparar ate o maximo e salvar tempos e medianas
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
