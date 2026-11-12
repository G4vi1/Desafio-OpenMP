"""Testes das formulas e validacao dos arquivos usados nos graficos."""
import csv
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from dados_relatorio import ler_contexto, ler_tempos, quantil, resumir, sha256, ultimo_csv


class DadosRelatorioTest(unittest.TestCase):
    def setUp(self):
        self.temporario = tempfile.TemporaryDirectory()
        self.pasta = Path(self.temporario.name)
        self.csv = self.pasta / 'comparacao_teste.csv'

    def tearDown(self):
        self.temporario.cleanup()

    def escrever(self, linhas):
        with self.csv.open('w', newline='') as arquivo:
            gravador = csv.writer(arquivo)
            gravador.writerow(['repeticao', 'versao', 'threads_pedidas', 'threads_usadas',
                               'leitura_s', 'calculos_s', 'total_s'])
            gravador.writerows(linhas)

    def dados(self):
        return [[i, v, t, t, .001, tempo, .001 + tempo + .0001]
                for i in (1, 2, 3) for v, t, tempo in
                [('sequencial', 1, .0002), ('paralelizado', 2, .0001)]]

    def test_speedup_e_eficiencia(self):
        self.escrever(self.dados())
        resumo = resumir(ler_tempos(self.csv))
        self.assertEqual(resumo[0]['speedup'], 1)
        self.assertEqual(resumo[1]['speedup'], 2)
        self.assertEqual(resumo[1]['eficiencia_pct'], 100)

    def test_quartis_interpolados(self):
        self.assertEqual(quantil([4, 1, 3, 2], .25), 1.75)
        self.assertEqual(quantil([4, 1, 3, 2], .75), 3.25)

    def test_zero_nao_produz_speedup_infinito(self):
        linhas = self.dados()
        for l in linhas:
            if l[1] == 'paralelizado': l[5] = 0
        self.escrever(linhas)
        self.assertIsNone(resumir(ler_tempos(self.csv))[1]['speedup'])

    def test_rejeita_threads_divergentes(self):
        linhas = self.dados()
        linhas[1][3] = 1
        self.escrever(linhas)
        with self.assertRaises(ValueError): ler_tempos(self.csv)

    def test_rejeita_nan_e_repeticao_duplicada(self):
        for alteracao in ('nan', 'duplicada'):
            linhas = self.dados()
            if alteracao == 'nan': linhas[0][5] = 'nan'
            else: linhas.append(linhas[0])
            self.escrever(linhas)
            with self.assertRaises(ValueError): ler_tempos(self.csv)

    def test_rejeita_coleta_incompleta(self):
        self.escrever(self.dados()[:-1])
        with self.assertRaises(ValueError): ler_tempos(self.csv)

    def test_detecta_csv_alterado(self):
        self.escrever(self.dados())
        metadados = self.csv.with_name(self.csv.stem + '_metadados.json')
        metadados.write_text(json.dumps({'tempos': {'sha256': sha256(self.csv)}}))
        ler_contexto(self.csv)
        self.csv.write_text(self.csv.read_text() + '\n')
        with self.assertRaises(ValueError): ler_contexto(self.csv)

    def test_ultimo_csv_ignora_estatisticas(self):
        self.escrever(self.dados())
        (self.pasta / 'comparacao_teste_estatisticas.csv').write_text('nao e tempo')
        self.assertEqual(ultimo_csv(self.pasta), self.csv)


if __name__ == '__main__':
    unittest.main()
