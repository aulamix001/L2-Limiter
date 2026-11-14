# L2 Limiter — painel de hardware e 25 presets

Plugin C++20/JUCE 7.0.9 com alvo VST3 e aplicativo Standalone. AU também é habilitado no macOS. Projeto independente da Aulamix; não contém código ou algoritmos proprietários da Waves.

## Compilar no Windows

Instale Visual Studio 2022 com **Desktop development with C++**, Windows SDK e CMake. No Developer PowerShell, na pasta deste repositório:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

O primeiro configure baixa JUCE do GitHub, fixado no commit da versão 7.0.9. A saída é a pasta `build/L2Limiter_artefacts/Release/VST3/L2 Limiter.vst3`. Copie a **pasta inteira** para `C:\Program Files\Common Files\VST3` e mande sua DAW procurar plugins novamente. O Standalone fica na pasta vizinha `Standalone`.

O workflow `.github/workflows/build.yml` compila e testa em Windows 2022/Visual Studio 2022 após push ou execução manual e publica um ZIP de desenvolvimento no GitHub Releases. O projeto também é compilado e testado neste ambiente Linux. A validação auditiva em uma DAW continua necessária.

## Linux

Neste ambiente de nuvem Debian 13 sem acesso de administrador, execute `bash scripts/setup-cloud-linux.sh`. O script usa pacotes APT verificados e instala as dependências em `/workspace/tooling`. Em tarefas futuras, carregue `source /workspace/tooling/activate.sh` antes de compilar.

Com privilégios de administrador em uma máquina Debian/Ubuntu convencional:

```sh
sudo apt-get install cmake ninja-build g++ pkg-config libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Para testar somente o DSP, sem JUCE ou bibliotecas gráficas:

```sh
cmake -S . -B build-dsp -DL2_BUILD_PLUGIN=OFF
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

## O que foi implementado

- Threshold e Out Ceiling de −30 a 0 dB, Release de 0,01 a 1000 ms, ARC ligado por padrão. Essas faixas são as adotadas por esta implementação; as imagens do manual não estabelecem integralmente todos os limites e defaults.
- Lookahead fixo de 1,5 ms, link estéreo para detecção, redução antecipada de ganho, compensação de ganho associada ao Threshold, teto de amostras e cálculo interno double. A latência é informada ao host.
- Release manual ou adaptativo baseado em pico/RMS, com suavização de threshold e ceiling.
- Quantização selecionável 24/22/20/18/16 bits, dither None/Type1/Type2 e shaping None/Moderate/Normal/Ultra.
- Painel azul texturizado com disposição de controles baseada na referência, sombras projetadas e faders em relevo, displays editáveis, ARC, seletores, medidores L/R de pico e RMS, peak hold de 3 segundos e reset por clique no painel, indicador de clip e A/B. O botão Link acompanha arrastes dos sliders Threshold/Ceiling.
- Estado dos parâmetros via APVTS, automação pelo host, presets XML `.l2p`, Undo/Redo e 25 presets de fábrica disponíveis também como programas do host. Shift-clique em Undo refaz a última ação; Ctrl/Cmd+Z e Ctrl/Cmd+Y também funcionam.
- Testes do núcleo DSP e de integração do processador JUCE, incluindo restauração de estado.

## Limites desta versão

É uma primeira implementação funcional, **não uma reprodução sonora certificada do Waves L2**. As imagens do manual descrevem os resultados de ARC/IDR, mas não fornecem os algoritmos ou coeficientes internos. O release adaptativo, os dois geradores de dither e o shaping de realimentação de 1ª/2ª/3ª ordem são próprios e provisórios. Os nomes Moderate/Normal/Ultra não significam que as curvas originais tenham sido reproduzidas. MBIT+ não é usado.

O teto é garantido para amostras, inclusive após quantização. Não há oversampling ou garantia de true peak: picos entre amostras podem ultrapassar o teto, como discutido no capítulo 7 do manual. O clip final é uma proteção adicional desta implementação.

Ainda faltam: curvas psicoacústicas de shaping validadas, medições espectrais e comparações auditivas com referência, limite exato de 32 transações de histórico, MIDI learn, validação em DAW. A/B restaura o estado no thread da interface e não tem garantia de troca sample-accurate. Input Gain/Auto Input e CLAP do texto preliminar não foram incluídos nesta etapa focada nos controles do manual e VST3.

A compilação Linux não produz um VST3 Windows. Os testes de compilação e processamento do Windows são executados pelo workflow. A validação auditiva e de integração numa DAW Windows continua necessária antes de tratar esse binário como produto final. JUCE tem termos de licença próprios que precisam ser observados para distribuição.

## Presets de fábrica

As 25 opções incluem Default; Female Vocal (Natural, Pop Forward, Soft Ballad); Male Vocal (Natural, Deep & Warm, Pop Forward); Backing Vocals; Bass (Electric Round, Pick Attack, Synth & Sub); Acoustic Gentle; Acoustic Guitar (Fingerstyle, Strumming); Nylon Guitar; Piano (Natural, Pop Presence); Electric Piano; Electric Guitar Clean; Drums; Strings; Pads; Podcast; Mix Bus; CD Master 16 Bit.

São ajustes originais para iniciar o trabalho, sem EQ ou modelagem de voz/instrumento. Ajuste Threshold ouvindo a fonte e observando a redução de ganho. Os presets de instrumentos mantêm saída de 24 bits, sem dither nem shaping; o preset CD Master seleciona 16 bits, Type1 e Normal. Um asterisco no seletor indica alterações em um preset de fábrica.

## Revisão do processamento

A quantização agora funciona também com dither desligado. O ceiling é alinhado ao último nível de quantização representável abaixo do teto, evitando ultrapassagens após dither/shaping. A detecção usa os dois canais com ganho vinculado, e o ataque considera quando cada pico sairá do lookahead, evitando uma queda instantânea na chegada do pico. Threshold/Ceiling têm rampas de 10 ms e Release de 5 ms; uma redução de Ceiling tem proteção imediata. A medição de pico usa decaimento em função do tempo, e a interface decai mesmo quando o host para de enviar áudio. NaN/Inf de entrada são substituídos por silêncio antes do processamento e da medição.

Os testes exercitam 480 combinações (44,1/48/96/192 kHz × ARC ligado/desligado × 5 profundidades × 3 dithers × 4 shapings), grade de quantização, ceiling, latência, passagem em ganho unitário, estéreo, ataque de transientes, recuperação do release, viés estatístico do dither, automação aleatória, restauração dos 25 presets, Undo/Redo e buffers de diversos tamanhos. O núcleo também foi executado com AddressSanitizer e UndefinedBehaviorSanitizer. O VST3 Linux passou no pluginval 1.0.4 nível 5 sem testes de janela; o workflow executa esse validador no VST3 Windows com os testes de interface habilitados. Consulte `VALIDATION.md` para o escopo e limitações.
