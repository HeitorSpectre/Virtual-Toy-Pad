# Virtual Toy Pad — Release 1.0

**By HeitorSpectre**

Payload ELF para executar LEGO Dimensions (versao PS4) no PS5 com jailbreak, usando uma Toy Pad virtual, menus de personagens/veiculos e selecao pelo DualSense, sem exigir a base fisica.

## Compatibilidade

- Jogo: LEGO Dimensions, **CUSA00935, atualizacao 1.23** (offsets especificos desta versao).
- PS5 com jailbreak e ELF Loader funcional (por exemplo, o ELFLDR/etaHEN).
- A reproducao em outras regioes/versoes do jogo nao foi validada.

## Instalar

1. Com o jogo fechado, carregue `ldinject.elf` pelo ELF Loader do PS5.
2. Abra LEGO Dimensions e espere a Toy Pad virtual ser detectada.
3. Use **L3 + R3** para abrir/fechar o menu. Selecione usando o direcional ou analogico esquerdo.
4. **X** seleciona; **O** volta; **L1/R1** alterna personagens/veiculos; **L2/R2** troca paginas.
5. Pressionar **Quadrado** junto com direcional no seletor permite ajuste fino das molduras. Quadrado + Triangulo altera os modos de borda.

Os dados do payload ficam em `/data/ldmod` no PS5. Nao ha necessidade de acessar PSN ou atualizar o firmware para usar este payload.

## Compilar o codigo-fonte

No WSL/Linux, instale `clang`, `ld.lld`, `make`, `gcc`, `xxd`, Python 3, `pyelftools` e o [ps5-payload-sdk](https://github.com/ps5-payload-dev/sdk).
Configure `PS5_PAYLOAD_SDK` com a pasta do SDK (padrao: `$HOME/ps5-payload-sdk`) e execute:

```bash
bash build.sh
```

O resultado sera `build/ldinject.elf`. O build gera `ldhook.bin` e `blob.h` automaticamente. Eles nao precisam estar versionados.

## Conteudo

O repositorio guarda somente codigo-fonte, Makefiles, tabelas de figuras e documentacao essencial. Nao inclui jogos, dumps, BIOS, EBOOT, conteudos da PSN, credenciais nem logs de diagnostico.

## Direitos e avisos

Projeto independente, nao oficial, desenvolvido por **HeitorSpectre**. LEGO Dimensions, LEGO e marcas associadas pertencem aos respectivos titulares. O usuario deve obter e executar legalmente o proprio jogo.

O codigo de `src/injector/pt.c` e `pt.h` contem trabalho de **John Törnblom (2024)**, licenciado em **GPL-3.0-or-later**; seus avisos originais foram preservados. Consulte `COPYING` para os termos aplicaveis. A distribuicao binaria inclui o codigo-fonte correspondente em `source-code.zip`.