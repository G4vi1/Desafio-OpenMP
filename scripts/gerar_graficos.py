#!/usr/bin/env python3
"""Gera figuras PNG/SVG e tabelas para o futuro relatorio, sem refazer medicoes."""
import argparse
import csv
import json
import math
from pathlib import Path
import shutil
import statistics

from dados_relatorio import (gravar_csv, ler_contexto, ler_estatisticas,
                             ler_tempos, resumir, sha256, ultimo_csv)


def rotulo(chave):
    return 'Sequencial' if chave[0] == 'sequencial' else f'OpenMP\n{chave[1]} thread' + ('s' if chave[1] != 1 else '')


def main():
    raiz = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path, nargs='?', help='padrao: ultima coleta CSV')
    args = parser.parse_args()
    origem = args.csv.resolve() if args.csv else ultimo_csv(raiz / 'resultados')
    grupos = ler_tempos(origem)
    resumo = resumir(grupos)
    contexto, avisos = ler_contexto(origem)
    estatisticas = ler_estatisticas(origem, grupos, contexto)
    if not estatisticas:
        avisos.append('CVs nao registrados nesta coleta: grafico de CV omitido, sem inferir valores atuais.')
    if min(len(v) for v in grupos.values()) < 10:
        avisos.append('Menos de 10 repeticoes por configuracao: use mais amostras na analise final.')
    if any(r['calculos_s'] <= 0 for linhas in grupos.values() for r in linhas):
        avisos.append('Ha tempos arredondados para zero; speedup pode ser indefinido e a escala de tempo sera linear.')

    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        raise SystemExit('Instale as dependencias graficas (veja README) ou use a imagem Docker atualizada.')

    pasta = raiz / 'resultados' / 'graficos' / origem.stem
    pasta.mkdir(parents=True, exist_ok=True)
    chaves = list(grupos)
    rotulos = [rotulo(c) for c in chaves]
    cores = ['#334155' if k[0] == 'sequencial' else '#0072B2' for k in chaves]
    n = len(next(iter(grupos.values())))
    fonte = origem.name
    plt.rcParams.update({'font.size': 11, 'axes.titlesize': 17,
                         'axes.labelsize': 12, 'figure.facecolor': 'white',
                         'axes.spines.top': False, 'axes.spines.right': False,
                         'svg.fonttype': 'none'})
    figuras = []

    def nova(titulo, subtitulo):
        fig, ax = plt.subplots(figsize=(11, 6.4))
        fig.suptitle(titulo, x=.08, y=.96, ha='left', fontweight='bold')
        ax.set_title(subtitulo, loc='left', fontsize=10, color='#475569', pad=14)
        ax.set_axisbelow(True)
        ax.grid(axis='y', alpha=.18)
        fig.text(.08, .025, f'Fonte: {fonte} | n = {n} por configuração',
                 fontsize=8, color='#475569')
        return fig, ax

    def salvar(fig, nome, legenda):
        fig.tight_layout(rect=(.025, .055, .99, .92))
        for extensao in ('png', 'svg'):
            fig.savefig(pasta / f'{nome}.{extensao}', dpi=300, bbox_inches='tight', facecolor='white')
        plt.close(fig)
        figuras.append((nome, legenda))

    def tempos(metrica, nome, titulo):
        valores = [r[metrica + '_mediana'] * 1e6 for r in resumo]
        fig, ax = nova(titulo, 'Mediana e intervalo interquartil (P25–P75); menor é melhor')
        ax.bar(range(len(chaves)), valores, color=cores, width=.62)
        # Log so quando todos os quartis sao positivos, sem inventar tempos.
        if all(r[metrica + '_q1'] > 0 for r in resumo):
            ax.set_yscale('log')
            ax.set_ylabel('Tempo (µs) — escala logarítmica')
            ax.set_ylim(bottom=min(valores) * .45)
        else:
            ax.set_ylabel('Tempo (µs)')
        for i, r in enumerate(resumo):
            mediana = valores[i]
            ax.errorbar(i, mediana,
                        yerr=[[max(0, mediana-r[metrica+'_q1']*1e6)],
                              [max(0, r[metrica+'_q3']*1e6-mediana)]],
                        fmt='none', color='#111827', capsize=5, linewidth=1.2)
            ax.annotate(f'{mediana:,.1f}'.replace(',', '_').replace('.', ',').replace('_', '.'), (i, r[metrica+'_q3']*1e6),
                        xytext=(0, 8), textcoords='offset points', ha='center', fontsize=10)
        ax.set_xticks(range(len(chaves)))
        ax.set_xticklabels(rotulos)
        maior_quartil = max(r[metrica + '_q3'] * 1e6 for r in resumo)
        if ax.get_yscale() == 'log':
            menor_quartil = min(r[metrica + '_q1'] * 1e6 for r in resumo)
            ax.set_ylim(menor_quartil * .6, maior_quartil * 1.9)
        else:
            ax.set_ylim(0, max(maior_quartil * 1.25, 1))
        salvar(fig, nome, 'Barras: medianas; hastes: quartis 25 e 75, não intervalos de confiança. '
               'Microsegundos; a escala logarítmica, quando usada, está identificada no eixo.')

    tempos('calculos_s', '01_tempo_calculos', 'Tempo dos cálculos: sequencial × OpenMP')
    tempos('total_s', '02_tempo_total', 'Tempo total: leitura, cálculos e saída')

    paralelos = [r for r in resumo if r['versao'] == 'paralelizado']
    threads = [r['threads'] for r in paralelos]
    fig, ax = nova('Speedup dos cálculos', 'Razão entre medianas: tempo sequencial / tempo paralelo')
    speedups = [r['speedup'] if r['speedup'] is not None else math.nan for r in paralelos]
    ax.plot(threads, speedups, 'o-', color='#0072B2', linewidth=2, label='Medido')
    ax.plot(threads, threads, '--', color='#94A3B8', label='Ideal: speedup = threads')
    ax.axhline(1, color='#D55E00', linestyle=':', label='Sem ganho: speedup = 1')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.set_xticks(threads)
    ax.set_xticklabels([str(t) for t in threads])
    ax.set_xlabel('Threads OpenMP efetivamente usadas')
    ax.set_ylabel('Speedup — escala logarítmica')
    for t, v in zip(threads, speedups):
        if math.isfinite(v) and v > 0:
            ax.annotate(f'{v:.3g}×', (t, v), xytext=(0, -18), textcoords='offset points', ha='center')
    ax.margins(y=.25)
    ax.legend(loc='upper left', fontsize=9)
    salvar(fig, '03_speedup', 'Speedup acima de 1 indica ganho; abaixo de 1 indica perda. '
           'A curva ideal é apenas uma referência, não uma previsão de desempenho.')

    fig, ax = nova('Eficiência paralela', 'Eficiência = speedup / número de threads × 100%')
    eficiencias = [r['eficiencia_pct'] if r['eficiencia_pct'] is not None else math.nan for r in paralelos]
    ax.plot(threads, eficiencias, 'o-', color='#009E73', linewidth=2, label='Eficiência medida')
    ax.axhline(100, color='#94A3B8', linestyle='--', label='Referência ideal: 100%')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.set_xticks(threads)
    ax.set_xticklabels([str(t) for t in threads])
    ax.set_xlabel('Threads OpenMP efetivamente usadas')
    ax.set_ylabel('Eficiência (%) — escala logarítmica')
    for t, v in zip(threads, eficiencias):
        if math.isfinite(v) and v > 0:
            ax.annotate(f'{v:.3g}%', (t, v), xytext=(0, -18), textcoords='offset points', ha='center')
    ax.margins(y=.25)
    ax.legend(loc='upper right', fontsize=9)
    salvar(fig, '04_eficiencia', 'Eficiência calculada a partir das medianas dos cálculos. '
           'Baixa eficiência revela custo de coordenação e/ou trabalho insuficiente por thread.')

    fig, ax = nova('Variação entre execuções', 'Cálculos: mediana, quartis, dispersão e valores extremos')
    amostras = [[r['calculos_s'] * 1e6 for r in grupos[k]] for k in chaves]
    caixas = ax.boxplot(amostras, patch_artist=True, widths=.55,
                       medianprops={'color': '#D55E00', 'linewidth': 2},
                       flierprops={'marker': '.', 'markersize': 4, 'alpha': .55})
    for caixa, cor in zip(caixas['boxes'], cores):
        caixa.set_facecolor(cor)
        caixa.set_alpha(.5)
    if all(v > 0 for amostra in amostras for v in amostra):
        ax.set_yscale('log')
        ax.set_ylabel('Tempo dos cálculos (µs) — escala logarítmica')
    else:
        ax.set_ylabel('Tempo dos cálculos (µs)')
    ax.set_xticks(range(1, len(chaves)+1))
    ax.set_xticklabels(rotulos)
    salvar(fig, '05_variacao_execucoes', 'Caixas: P25–P75; linha: mediana; hastes: até 1,5 intervalo '
           'interquartil; pontos: valores além das hastes. Nenhuma medição é removida.')

    fig, ax = nova('Onde o tempo total é gasto?', 'Participação no tempo médio total; a escala de cada barra é 100%')
    bases = [0.] * len(chaves)
    componentes = ('Leitura e validação', 'Cálculos', 'Saída e demais operações')
    for j, (nome, cor) in enumerate(zip(componentes, ('#56B4E9', '#D55E00', '#94A3B8'))):
        alturas = []
        for chave in chaves:
            linhas = grupos[chave]
            leitura = statistics.mean(r['leitura_s'] for r in linhas)
            calculos = statistics.mean(r['calculos_s'] for r in linhas)
            total = statistics.mean(r['total_s'] for r in linhas)
            residuo = max(0, total - leitura - calculos)
            componentes_s = (leitura, calculos, residuo)
            soma = sum(componentes_s)
            alturas.append(componentes_s[j] / soma * 100 if soma else 0)
        ax.bar(range(len(chaves)), alturas, bottom=bases, color=cor, width=.62, label=nome)
        bases = [a+b for a, b in zip(bases, alturas)]
    ax.set_xticks(range(len(chaves)))
    ax.set_xticklabels(rotulos)
    ax.set_ylabel('Participação no tempo médio total (%)')
    ax.set_ylim(0, 116)
    ax.legend(loc='upper center', ncol=3, fontsize=9)
    salvar(fig, '06_composicao_tempo', 'Participações derivadas das médias dos componentes. O restante '
           'é total − leitura − cálculos e inclui saída e outras operações, não somente impressão. '
           'Não é uma medição por fase das diretivas OpenMP.')

    if estatisticas:
        fig, ax = nova('Coeficientes de variação obtidos', 'Resultados estatísticos da mesma coleta; não são métricas de velocidade')
        for deslocamento, campo, nome, cor in [(-.19, 'cv_estatura_pct', 'Estatura', '#0072B2'),
                                                (.19, 'cv_peso_pct', 'Peso', '#E69F00')]:
            valores = [r[campo] for r in estatisticas]
            barras = ax.bar([i+deslocamento for i in range(len(chaves))], valores,
                            width=.36, color=cor, label=nome)
            ax.bar_label(barras, labels=[f'{v:.2f}%' for v in valores], padding=4, fontsize=9)
        ax.set_xticks(range(len(chaves)))
        ax.set_xticklabels(rotulos)
        ax.set_ylabel('Coeficiente de variação (%)')
        ax.margins(y=.25)
        ax.legend(loc='upper left', ncol=2)
        salvar(fig, '07_coeficientes_variacao', 'CV de estatura e peso registrado junto das medições. '
               'Valores preservam as duas casas decimais exibidas pelo programa. Igualdade visual '
               'não comprova igualdade em toda a precisão interna.')

    gravar_csv(pasta / 'resumo_desempenho.csv', resumo)
    gravar_csv(pasta / 'estatisticas_obtidas.csv', estatisticas)
    # Guarda as entradas exatas usadas, sem juntar coletas de codigos diferentes.
    shutil.copyfile(origem, pasta / 'tempos_originais.csv')
    fontes = contexto.get('fontes', {})
    for nome, informacoes in fontes.items():
        snapshot = origem.parent / informacoes['snapshot']
        if sha256(snapshot) != informacoes['sha256']:
            raise ValueError('Snapshot de codigo alterado: ' + nome)
        destino = pasta / 'fontes' / nome
        destino.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(snapshot, destino)

    manifesto = {'matplotlib': matplotlib.__version__, 'origem': str(origem), 'sha256_tempos': sha256(origem),
                 'metadados_coleta': contexto, 'avisos': avisos,
                 'figuras': [{'arquivo': nome, 'legenda': legenda} for nome, legenda in figuras]}
    (pasta / 'manifesto_graficos.json').write_text(json.dumps(manifesto, ensure_ascii=False, indent=2)+'\n')
    linhas = ['# Material para o futuro relatório', '',
              f'Fonte única: `{fonte}`. Não foram misturadas coletas diferentes.', '',
              '## 1. Código-fonte e diretivas', '',
              'Os snapshots da coleta, quando disponíveis, estão em `fontes/codigos/`. '
              'Inclua `sequencial.cpp`, `paralelizado.cpp`, `comum.hpp` e `calculo_sequencial.hpp` '
              'na entrega do código completo.', '',
              '## 2. O que foi paralelizado e por quê', '',
              'Uma equipe processa as duas variáveis. O primeiro `omp for schedule(static)` '
              'distribui a contagem entre vetores privados, evitando disputa por contador. '
              'Os próximos laços combinam as frequências por classe e usam `reduction(+:...)` '
              'para somar os produtos ponderados. `single` publica as médias e sua barreira '
              'garante a disponibilidade antes dos desvios. Outras reduções combinam os '
              'desvios quadrados. `nowait` nos primeiros laços de estatura permite adiantar '
              'o peso; as barreiras posteriores preservam as dependências. Preparação, leitura '
              'e impressão continuam sequenciais. Confira essa descrição contra os snapshots '
              'antes de incluí-la no PDF.', '',
              '## 3. Resultados estatísticos', '']
    if estatisticas:
        linhas += ['| Configuração | Média estatura (cm) | Desvio (cm) | CV estatura | Média peso (kg) | Desvio (kg) | CV peso |',
                   '|---|---:|---:|---:|---:|---:|---:|']
        for e in estatisticas:
            linhas.append(f"| {rotulo((e['versao'],e['threads'])).replace(chr(10),' ')} | "
                          f"{e['media_estatura_cm']:.2f} | {e['desvio_estatura_cm']:.2f} | {e['cv_estatura_pct']:.2f}% | "
                          f"{e['media_peso_kg']:.2f} | {e['desvio_peso_kg']:.2f} | {e['cv_peso_pct']:.2f}% |")
    else:
        linhas.append('Esta coleta não registra os CVs. Faça uma nova comparação para documentá-los.')
    linhas += ['', '## 4. Análise de desempenho', '',
               '| Configuração | Mediana cálculos (s) | Mediana total (s) | Speedup | Eficiência |',
               '|---|---:|---:|---:|---:|']
    for r in resumo:
        ganho = f"{r['speedup']:.4g}" if r['speedup'] is not None else 'N/D'
        eficiencia = f"{r['eficiencia_pct']:.4g}%" if r['eficiencia_pct'] is not None else 'N/D'
        linhas.append(f"| {rotulo((r['versao'],r['threads'])).replace(chr(10),' ')} | "
                      f"{r['calculos_s_mediana']:.6f} | {r['total_s_mediana']:.6f} | {ganho} | {eficiencia} |")
    linhas += ['', 'Compare com o sequencial otimizado: speedup menor que 1 é perda. '
               'Processos novos incluem a criação inicial da equipe; não confunda isso com '
               'medição de uma equipe aquecida. As hastes dos gráficos são quartis, não '
               'intervalos de confiança. Não atribua diferenças da leitura à aceleração '
               'dos cálculos. Tempos do programa foram arredondados a microssegundos.', '',
               '## Figuras e legendas sugeridas', '']
    for nome, legenda in figuras:
        linhas += [f'### {nome}', '', f'![{nome}]({nome}.png)', '', legenda, '']
    linhas += ['## Proveniência e limitações', '',
               f"Entrada registrada: `{contexto.get('entrada',{}).get('arquivo','não registrada')}`.",
               f"Quantidade de pessoas: {contexto.get('entrada',{}).get('pessoas','não registrada')}.",
               'Consulte o JSON para sistema, processadores, compilador, hashes e método da coleta. '
               'O arquivo `dados_exemplo.txt` fornecido pelo projeto contém dados fictícios.', '']
    linhas += ['- ' + aviso for aviso in avisos]
    (pasta / 'guia_relatorio.md').write_text('\n'.join(linhas)+'\n')
    for aviso in avisos:
        print('Aviso:', aviso)
    print(f'{len(figuras)} graficos PNG (300 dpi) e SVG gerados em: {pasta}')
    print('Tabelas, snapshots e guia de redacao salvos na mesma pasta. Nenhum PDF final foi criado.')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, OSError) as erro:
        raise SystemExit(f'Erro ao gerar graficos: {erro}')
