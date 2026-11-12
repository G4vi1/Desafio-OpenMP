#!/usr/bin/env python3
"""Compara sequencial e OpenMP em potencias de dois ate o limite disponivel."""
import argparse
import csv
from datetime import datetime
import os
from pathlib import Path
import random
import re
import statistics
import subprocess
import tempfile


def configuracoes(maximo):
    valores = []
    threads = 1
    while threads <= maximo:
        valores.append(threads)
        threads *= 2
    if valores[-1] != maximo:
        valores.append(maximo)
    return valores


def maximo_disponivel():
    try:
        processadores = len(os.sched_getaffinity(0))
    except AttributeError:
        processadores = os.cpu_count() or 1
    limite_omp = os.environ.get('OMP_THREAD_LIMIT', '')
    if limite_omp.isdigit() and int(limite_omp) > 0:
        processadores = min(processadores, int(limite_omp))
    return max(1, processadores)


def executar(executavel, dados, threads):
    ambiente = dict(os.environ, OMP_NUM_THREADS=str(threads), OMP_DYNAMIC='FALSE')
    processo = subprocess.run([str(executavel), str(dados), '--sem-tabela'],
                              env=ambiente, text=True, capture_output=True, timeout=60)
    if processo.returncode:
        raise RuntimeError(processo.stderr or processo.stdout)
    return processo.stdout


def main():
    raiz = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dados', type=Path, nargs='?', default=raiz / 'dados_exemplo.txt')
    parser.add_argument('--repeticoes', type=int, default=30)
    parser.add_argument('--max-threads', '--threads', dest='max_threads', type=int,
                        help='limita a sequencia; padrao: maximo disponivel')
    args = parser.parse_args()
    disponivel = maximo_disponivel()
    maximo = args.max_threads if args.max_threads is not None else disponivel
    if args.repeticoes < 3:
        parser.error('use pelo menos 3 repeticoes')
    if not 1 <= maximo <= disponivel:
        parser.error(f'o limite deve estar entre 1 e {disponivel} threads')
    dados = args.dados.resolve()
    if not dados.is_file():
        parser.error(f'arquivo nao encontrado: {dados}')
    subprocess.run(['make', '-C', str(raiz / 'codigos'), 'all'], check=True)

    variantes = [('sequencial', 1)] + [('paralelizado', t) for t in configuracoes(maximo)]
    print('Threads disponiveis:', disponivel)
    print('Configuracoes: sequencial; OpenMP com', ', '.join(str(t) for _, t in variantes[1:]))
    print('Repeticoes por configuracao:', args.repeticoes, flush=True)

    # Referencia sempre sequencial, independentemente da ordem dos testes.
    saida_base = executar(raiz / 'codigos/sequencial', dados, 1)
    padrao_resultado = r'^(?:Media agrupada|Desvio padrao populacional|Coeficiente de variacao):.*$'
    referencia = re.findall(padrao_resultado, saida_base, re.M)
    if len(referencia) != 6:
        raise RuntimeError('Saida sequencial incompleta.')
    medicoes = []
    gerador = random.Random(42)
    for repeticao in range(args.repeticoes):
        ordem = variantes.copy()
        gerador.shuffle(ordem)
        for programa, threads in ordem:
            executavel = raiz / ('codigos/sequencial' if programa == 'sequencial' else 'resultados/paralelizado')
            saida = executar(executavel, dados, threads)
            if re.findall(padrao_resultado, saida, re.M) != referencia:
                raise RuntimeError('Estatisticas exibidas divergentes; comparacao interrompida.')
            usadas = 1
            if programa == 'paralelizado':
                equipe = re.search(r'Threads efetivamente usadas .*: (\d+)/(\d+)/(\d+)', saida)
                if not equipe or tuple(map(int, equipe.groups())) != (threads,) * 3:
                    raise RuntimeError(f'A equipe real difere das {threads} threads solicitadas. Confira os limites OpenMP.')
                usadas = int(equipe[1])
            tempos = []
            for padrao in ('Tempo de leitura', 'Tempo dos calculos', 'Tempo total de execucao'):
                valor = re.search(padrao + r' \(s\): ([0-9.]+)', saida)
                if not valor:
                    raise RuntimeError(f'Metrica ausente: {padrao}')
                tempos.append(float(valor[1]))
            medicoes.append([repeticao + 1, programa, threads, usadas, *tempos])

    pasta = raiz / 'resultados'
    pasta.mkdir(exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w', prefix='comparacao_' + datetime.now().strftime('%Y%m%d_%H%M%S_'),
                                     suffix='.csv', dir=pasta, delete=False, newline='') as arquivo:
        caminho = Path(arquivo.name)
        tabela = csv.writer(arquivo)
        tabela.writerow(['repeticao', 'versao', 'threads_pedidas', 'threads_usadas', 'leitura_s', 'calculos_s', 'total_s'])
        tabela.writerows(medicoes)
    base = statistics.median(m[5] for m in medicoes if m[1] == 'sequencial')
    linhas = [f'Dados: {dados}', f'Threads disponiveis: {disponivel}',
              f'Repeticoes por configuracao: {args.repeticoes}',
              'Processos novos; ordem embaralhada; tabelas e IDs desativados; OpenMP ativo.',
              'Tempos em segundos (medianas). Total inclui leitura e impressao do resumo.',
              'Versao | Threads | Leitura (s) | Calculos (s) | Total (s) | Speedup | Eficiencia (%)']
    for programa, threads in variantes:
        grupo = [m for m in medicoes if m[1:3] == [programa, threads]]
        leitura, calculos, total = [statistics.median(m[i] for m in grupo) for i in (4, 5, 6)]
        speedup = base / calculos if calculos > 0 else None
        ganho = f'{speedup:.3f}' if speedup is not None else 'N/D'
        eficiencia = f'{speedup / threads * 100:.2f}' if speedup is not None else 'N/D'
        linhas.append(f'{programa} | {threads} | {leitura:.6f} | {calculos:.6f} | {total:.6f} | {ganho} | {eficiencia}')
    linhas.append('Tempos arredondados a microssegundos; N/D indica tempo abaixo da precisao exibida.')
    relatorio = caminho.with_suffix('.txt')
    relatorio.write_text('\n'.join(linhas) + '\n')
    print('\n'.join(linhas))
    print(f'\nTempos brutos: {caminho}\nResumo: {relatorio}')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.SubprocessError) as erro:
        raise SystemExit(f'Erro na comparacao: {erro}')
