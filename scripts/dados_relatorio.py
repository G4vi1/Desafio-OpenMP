"""Leitura, validacao e resumo dos CSVs, sem dependencia de bibliotecas graficas."""
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics

METRICAS = ('leitura_s', 'calculos_s', 'total_s')
CAMPOS_EST = ('media_estatura_cm', 'desvio_estatura_cm', 'cv_estatura_pct',
              'media_peso_kg', 'desvio_peso_kg', 'cv_peso_pct')


def sha256(caminho):
    return hashlib.sha256(Path(caminho).read_bytes()).hexdigest()


def ultimo_csv(pasta):
    candidatos = [p for p in pasta.glob('comparacao_*.csv')
                  if not p.stem.endswith('_estatisticas')]
    if not candidatos:
        raise ValueError('Nenhuma comparacao CSV encontrada. Execute make comparar primeiro.')
    return max(candidatos, key=lambda p: p.stat().st_mtime_ns)


def numero(texto, campo):
    valor = float(texto)
    if not math.isfinite(valor) or valor < 0:
        raise ValueError(f'{campo}: valor invalido {texto!r}')
    return valor


def ler_tempos(caminho):
    grupos = {}
    repeticoes = set()
    with caminho.open(newline='') as arquivo:
        leitor = csv.DictReader(arquivo)
        if not set(('versao', 'repeticao', *METRICAS)).issubset(leitor.fieldnames or []):
            raise ValueError('CSV nao possui as colunas de tempos esperadas.')
        campo_threads = 'threads_pedidas' if 'threads_pedidas' in leitor.fieldnames else 'threads'
        for linha in leitor:
            versao = linha['versao']
            if versao not in ('sequencial', 'paralelizado'):
                raise ValueError(f'Versao desconhecida: {versao}')
            threads = int(linha[campo_threads])
            repeticao = int(linha['repeticao'])
            usadas = int(linha.get('threads_usadas') or threads)
            if threads < 1 or repeticao < 1 or usadas != threads or (versao == 'sequencial' and threads != 1):
                raise ValueError('Quantidade de threads ou repeticao invalida.')
            chave = (versao, threads)
            identificador = (*chave, repeticao)
            if identificador in repeticoes:
                raise ValueError('Repeticao duplicada na mesma configuracao.')
            repeticoes.add(identificador)
            valores = {campo: numero(linha[campo], campo) for campo in METRICAS}
            if valores['total_s'] + 2e-6 < valores['leitura_s'] + valores['calculos_s']:
                raise ValueError('Tempo total menor que leitura + calculos.')
            valores['repeticao'] = repeticao
            grupos.setdefault(chave, []).append(valores)
    if ('sequencial', 1) not in grupos or not any(k[0] == 'paralelizado' for k in grupos):
        raise ValueError('A comparacao precisa conter sequencial e paralelo.')
    conjuntos = [{r['repeticao'] for r in rows} for rows in grupos.values()]
    if any(c != conjuntos[0] for c in conjuntos[1:]):
        raise ValueError('Configuracoes possuem conjuntos diferentes de repeticoes.')
    return dict(sorted(grupos.items(), key=lambda item: (item[0][0] != 'sequencial', item[0][1])))


def quantil(valores, proporcao):
    valores = sorted(valores)
    posicao = (len(valores) - 1) * proporcao
    baixo = int(posicao)
    alto = min(baixo + 1, len(valores) - 1)
    return valores[baixo] + (valores[alto] - valores[baixo]) * (posicao - baixo)


def resumir(grupos):
    base = statistics.median(r['calculos_s'] for r in grupos[('sequencial', 1)])
    resumo = []
    for (versao, threads), linhas in grupos.items():
        r = {'versao': versao, 'threads': threads, 'repeticoes': len(linhas)}
        for metrica in METRICAS:
            valores = [linha[metrica] for linha in linhas]
            r[metrica + '_mediana'] = statistics.median(valores)
            r[metrica + '_q1'] = quantil(valores, .25)
            r[metrica + '_q3'] = quantil(valores, .75)
        tempo = r['calculos_s_mediana']
        r['speedup'] = base / tempo if base > 0 and tempo > 0 else None
        r['eficiencia_pct'] = r['speedup'] / threads * 100 if r['speedup'] is not None else None
        resumo.append(r)
    return resumo


def ler_contexto(caminho):
    arquivo = caminho.with_name(caminho.stem + '_metadados.json')
    if not arquivo.exists():
        return {}, ['Coleta antiga sem metadados de ambiente e estatisticas.']
    contexto = json.loads(arquivo.read_text())
    if contexto['tempos']['sha256'] != sha256(caminho):
        raise ValueError('O CSV de tempos mudou desde a coleta; SHA-256 divergente.')
    return contexto, []


def ler_estatisticas(caminho, grupos, contexto):
    if not contexto:
        return []
    p = caminho.parent / contexto['estatisticas']['arquivo']
    if sha256(p) != contexto['estatisticas']['sha256']:
        raise ValueError('CSV de estatisticas com SHA-256 divergente.')
    linhas = list(csv.DictReader(p.open(newline='')))
    valores = {}
    vistos = set()
    for linha in linhas:
        chave = (linha['versao'], int(linha['threads_pedidas']))
        repeticao = int(linha['repeticao'])
        if chave not in grupos or repeticao not in {r['repeticao'] for r in grupos[chave]}:
            raise ValueError('Estatistica nao corresponde a uma medicao.')
        identidade = (*chave, repeticao)
        if identidade in vistos:
            raise ValueError('Estatistica duplicada.')
        vistos.add(identidade)
        dados = tuple(numero(linha[campo], campo) for campo in CAMPOS_EST)
        if chave in valores and valores[chave] != dados:
            raise ValueError('Estatisticas variam entre repeticoes da mesma configuracao.')
        valores[chave] = dados
    if len(vistos) != sum(len(v) for v in grupos.values()):
        raise ValueError('Faltam estatisticas para algumas medicoes.')
    return [dict(versao=k[0], threads=k[1], **dict(zip(CAMPOS_EST, valores[k]))) for k in grupos]


def gravar_csv(caminho, linhas):
    if not linhas:
        return
    with caminho.open('w', newline='') as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=list(linhas[0]))
        escritor.writeheader()
        escritor.writerows(linhas)
