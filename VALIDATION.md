# Revisão técnica desta versão

## Verificações executadas

| Verificação | Resultado no Linux |
| --- | --- |
| Compilação C++20/JUCE, VST3 e Standalone | Passou |
| 480 configurações de DSP: 4 taxas × 2 releases × 5 bits × 3 dithers × 4 shapings | Passou |
| Teto de amostras antes/depois de quantização e durante automação | Passou |
| Grade de quantização inclusive com dither desligado | Passou |
| Latência de 1,5 ms e ausência de saída antecipada | Passou |
| Senoide em ganho unitário, erro limitado à quantização | Passou |
| Vínculo estéreo e preservação da relação entre canais | Passou |
| Continuidade do ataque ao detectar transiente futuro | Passou |
| Recuperação de release manual rápido/lento | Passou |
| Teste estatístico de viés de dither de baixo nível | Passou |
| 100.000 amostras com automação aleatória de todos os controles de DSP | Passou |
| Restauração dos 25 presets, valores, nomes e índices de programa | Passou |
| Áudio de cada preset: saída finita e dentro do ceiling | Passou |
| Undo/Redo de presets e identidade do programa | Passou |
| Buffers de 0, 1, 17, 64, 511 e 1024 amostras | Passou |
| NaN/Inf de entrada: saída e medidores finitos | Passou |
| AddressSanitizer + UndefinedBehaviorSanitizer no núcleo DSP | Passou, sem diagnósticos |
| Renderização do painel JUCE, textura e controles | Inspecionada |
| pluginval 1.0.4, nível 5, VST3 Linux carregado por host externo | Passou; testes de janela omitidos por execução sem desktop |

Comandos: `ctest --test-dir build --output-on-failure`; para os sanitizadores, `g++ -std=c++20 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer -ISource Tests/DspTests.cpp -o dsp-sanitized && ./dsp-sanitized`. Os resultados não certificam todos os hosts ou dispositivos.

O workflow também compila com Visual Studio 2022 em Windows x64, executa as duas suítes de testes e verifica os arquivos do pacote. A publicação do ZIP ocorre somente após o sucesso dessas etapas; o job associado a cada release é a evidência para o binário Windows correspondente. O pipeline também carrega o VST3 Windows no pluginval 1.0.4, nível 5, com os testes de interface habilitados. O log `validation-windows.log` acompanha o ZIP quando essa etapa passa. No Linux, o pluginval encontrou exatamente um plugin e executou os testes de processamento/automação em 44,1/48/96 kHz e blocos de 64/128/256/512/1024 amostras. O validador VST3 separado da Steinberg não foi executado; auval não se aplica a este alvo Linux VST3.

## Falhas corrigidas nesta revisão

- A escolha de bits era ignorada com dither None. Agora a quantização é independente do dither.
- A quantização/shaping podia terminar no teto fora da grade de bits. O limite final usa o maior nível representável que não ultrapassa o ceiling.
- O ganho podia cair instantaneamente quando um transiente entrava no buffer. O ataque agora antecipa o momento de reprodução de cada pico.
- A suavização anterior não tinha duração fixa. Threshold/Ceiling usam 10 ms e Release 5 ms; o limite de segurança continua imediato ao reduzir o ceiling.
- O decaimento dos medidores dependia do tamanho do buffer e podia congelar com o transporte parado. O processador usa decaimento por tempo e a interface trata a interrupção de blocos.
- Entradas não finitas podiam contaminar os medidores. Elas são saneadas antes da medição e do DSP.
- A identidade de um preset podia divergir após restauração/Undo/A-B. O índice acompanha as mudanças do estado salvo; o seletor indica presets de fábrica modificados com asterisco.

## Limites que permanecem

O painel e os controles seguem a referência fornecida; o processamento é uma implementação original. O manual não publica o algoritmo ou os coeficientes de ARC/IDR. Portanto, não foi demonstrada equivalência sonora ou numérica ao Waves L2. O shaping desta implementação usa realimentação de erro de 1ª/2ª/3ª ordem; os nomes Moderate, Normal e Ultra não significam que as curvas originais foram reproduzidas.

O ceiling é de amostras. Não há oversampling/detector de true peak; reconstrução D/A e conversão de sample rate podem gerar picos entre amostras, conforme a discussão do capítulo 7 do manual. O A/B restaura parâmetros com suavização, sem garantia de troca sample-accurate. Os testes não equivalem a certificação de tempo real, cobertura exaustiva de falhas, avaliação auditiva, comparação com uma instância original ou validação em uma DAW real.

Os presets de instrumentos são pontos de partida originais e não aplicam EQ, correção de afinação ou modelagem da voz. O nível da gravação determina a redução de ganho. A revisão elimina as falhas reproduzidas; não constitui garantia de funcionamento perfeito em todo cenário.
