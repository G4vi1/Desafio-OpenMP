# Desafio OpenMP — versão sequencial

Programa em C++ que lê estatura e peso de 1.000 pessoas e calcula as frequências por classe, a média agrupada, o desvio padrão populacional e o coeficiente de variação (CV).

Esta versão é sequencial, sem diretivas OpenMP. As estaturas são agrupadas em intervalos de 8 cm e os pesos em intervalos de 4 kg. Os cálculos usam os pontos médios das classes.

## 1. Preparar o ambiente

Para executar diretamente no Linux, tenha Bash, `g++` e `make` disponíveis. Confira:

```bash
g++ --version
make --version
```

Entre na pasta principal do projeto:

```bash
cd ~/Downloads/Desafio-OpenMP
```

Se o projeto estiver em outro local, ajuste esse caminho. Os blocos abaixo indicam de qual pasta devem ser executados.

## 2. Teste rápido com os dados de exemplo

Na pasta principal:

```bash
bash scripts/sequencial.sh
```

O script compila o código, executa com `dados_exemplo.txt`, mostra os resultados no terminal e salva um `.txt` em `resultados/`.

O arquivo de exemplo contém **1.000 registros fictícios**. Ele serve para testar o programa, não representa uma coleta real.

Se seu terminal já estiver dentro de `codigos`, use este comando no lugar do anterior:

```bash
bash ../scripts/sequencial.sh
```

Não é necessário executar os dois blocos: são alternativas conforme sua pasta atual.

## 3. Testar com seus próprios dados

Crie `dados.txt` na pasta principal. Cada linha deve conter a estatura em centímetros e o peso em quilogramas, separados por espaço:

```text
170.50 68.20
182.00 84.70
159.30 55.00
```

Essas três linhas ilustram o formato. O arquivo completo precisa ter **exatamente 1.000 linhas**, sem cabeçalho, linhas vazias ou colunas extras. Use ponto como separador decimal. Os valores devem ser positivos e finitos.

Confira a quantidade de registros:

```bash
awk 'END { print NR }' dados.txt
```

Na pasta principal, execute:

```bash
bash scripts/sequencial.sh dados.txt
```

Se estiver em `codigos`, execute:

```bash
bash ../scripts/sequencial.sh ../dados.txt
```

Quando você informa um caminho, ele é interpretado a partir da pasta atual do terminal. Sem argumento, o script localiza automaticamente o `dados_exemplo.txt` da pasta principal.

## 4. Compilar e executar pelo Makefile

Entre em `codigos` a partir da pasta principal:

```bash
cd codigos
make
make run DADOS=../dados_exemplo.txt
```

Para usar seus dados:

```bash
make run DADOS=../dados.txt
```

O Makefile compila e executa, mas não salva automaticamente um relatório. Para gerar o `.txt`, use o script da seção 2 ou 3.

Para remover o executável e recompilar:

```bash
make clean
make
```

Os arquivos de dados e os relatórios são preservados por `make clean`.

## 5. Onde consultar os resultados

Cada execução bem-sucedida do script gera um arquivo com nome semelhante a:

```text
resultados/sequencial_20261112_160850_E3YyZO.txt
```

O nome inclui data, hora e um identificador para não sobrescrever execuções anteriores. Ao terminar, o script informa o caminho completo do relatório.

Na pasta principal, liste os relatórios do mais recente para o mais antigo:

```bash
ls -lt resultados/*.txt
```

Abra o arquivo desejado no VS Code ou leia pelo terminal, substituindo o nome abaixo pelo que foi gerado:

```bash
cat resultados/NOME_DO_ARQUIVO.txt
```

O relatório contém:

- Intervalos, pontos médios e frequências das classes ocupadas.
- Média agrupada e desvio padrão populacional de estatura e peso.
- Coeficiente de variação em porcentagem.
- Tempo de leitura, tempo dos cálculos e tempo total em segundos.

Os valores estatísticos usam duas casas decimais; os tempos usam seis. O tempo dos cálculos inclui agrupamento e estatísticas, sem leitura nem impressão. O total inclui leitura e impressão dos resultados, mas exclui compilação, inicialização do processo e impressão das próprias métricas. Os tempos variam entre execuções.

Para o `dados_exemplo.txt` fornecido, os resultados esperados são:

| Variável | Média | Desvio padrão populacional | CV |
|---|---:|---:|---:|
| Estatura | 172.68 cm | 15.64 cm | 9.06% |
| Peso | 82.86 kg | 21.40 kg | 25.83% |

## 6. Organização do projeto

```text
Desafio-OpenMP/
├── .gitignore
├── Dockerfile
├── README.md
├── dados_exemplo.txt
├── codigos/
│   ├── Makefile
│   └── sequencial.cpp
├── scripts/
│   └── sequencial.sh
└── resultados/             # Relatórios gerados pelo script
```

O executável é gerado em `codigos/sequencial`. O `.gitignore` exclui os executáveis e o conteúdo de `resultados/` do versionamento.

## 7. Problemas comuns

**“Nenhum arquivo make encontrado”**: execute `make` dentro de `codigos`. Na pasta principal, use `make -C codigos` ou o script.

**“Nada a ser feito para all”**: o executável está atualizado. Execute `make run DADOS=../dados_exemplo.txt` dentro de `codigos` para rodar.

**“Arquivo de dados nao encontrado”**: confira o nome e o caminho do arquivo. `../dados.txt` significa um arquivo na pasta acima da atual. O projeto fornece `dados_exemplo.txt`; `dados.txt` precisa ser criado por você.

**“Esperadas 1000 pessoas” ou erro de linha**: confira a quantidade e o formato dos registros conforme a seção 3.

**“Permission denied” ao chamar o script diretamente**: use `bash scripts/sequencial.sh` na pasta principal ou dê permissão com `chmod +x scripts/sequencial.sh`.
