# PCSXR360 - Build Release Setup

## 📦 Empacotamento Automático

A build agora cria automaticamente um arquivo **ZIP** pronto para distribuição contendo:
- `default.xex` - Executável principal
- `media/PsxSkin.xzp` - Pacote de skins
- Pastas de estrutura: `bios/`, `memcards/`, `roms/`, `states/`
- `pcsx.ini` - Configuração padrão

### Arquivos Gerados

Após uma build bem-sucedida, os seguintes arquivos serão criados:

```
Release_OP/                          (ou Release/)
├── default.xex                      ← Executável gerado
├── pcsxr.pdb                        ← Debug symbols (opcional)
├── media/
│   ├── PsxSkin.xzp                  ← Gerado pelo xuipkg
│   └── Graphics/                    ← Arquivos XUI e imagens
├── dist/                            ← Pasta temporária criada pelo PostBuildEvent
│   ├── default.xex
│   ├── media/PsxSkin.xzp
│   ├── pcsx.ini
│   └── [pastas de estrutura]
└── PCSXr360_LTCG.zip                ← 📦 PACOTE FINAL!
```

## Estrutura de Pastas Necessária

Para que a build Release funcione corretamente, a seguinte estrutura de pastas deve estar presente no diretório de saída:

```
Release/
├── default.xex          (gerado pela compilação)
├── pcsxr.xex            (gerado pela compilação)
├── pcsx.ini             (arquivo de configuração)
├── media/
│   ├── PsxSkin.xzp      (arquivo XZP gerado pelo xuipkg)
│   ├── psx.jpg
│   └── Graphics/
│       ├── *.png        (ícones e imagens)
│       ├── *.xui        (interfaces XUI)
│       └── xarialuni.ttf (fonte)
├── bios/
│   └── SCPH1001.BIN     (BIOS do PlayStation - obrigatório!)
├── memcards/            (Memory Cards - criados automaticamente)
├── hlsl/                (Shaders HLSL - opcional)
│   ├── *.cg
│   └── */*.cg
├── covers/              (Capas de jogos - opcional)
├── gameguides/          (Guias de jogos - opcional)
├── states/              (Save States - criados automaticamente)
└── roms/                (ROMs/ISOs de jogos - adicione seus jogos aqui)
```

## Arquivos Necessários

### 1. Arquivos de Código (Gerados pela Build)
- `default.xex` - Executável principal do Xbox 360
- `pcsxr.exe`/.xex - Executável do emulador

### 2. Mídia e Interface
- Pasta `media/` com todas as imagens e arquivos XUI
- Arquivo `media/PsxSkin.xzp` - Pacote de skins compactado (gerado pelo xuipkg)

### 3. BIOS (Obrigatório)
- `bios/SCPH1001.BIN` - BIOS do PlayStation 1 (não incluso no repositório por copyright)
  - **Nota:** Este é o único arquivo que você precisa adicionar manualmente!

### 4. Memory Cards (Automático)
- **Não precisa incluir arquivos .mcd no ZIP!**
- Os memory cards (`Memcard1.mcd` e `Memcard2.mcd`) são **criados automaticamente** pelo emulador na primeira vez que você rodar um jogo
- Eles serão salvos na pasta `memcards/` conforme configurado no `pcsx.ini`

### 5. Shaders (Opcional)
- Pasta `hlsl/` com shaders Cg para filtros de vídeo
- Se não houver shaders, o emulador usará renderização padrão

### 6. Configuração
- `pcsx.ini` - Arquivo de configuração do emulador (gerado automaticamente se não existir)

## Como Configurar

### ✅ Opção 1: Build Automática com ZIP (Recomendada)

O arquivo `pcsxr.vcxproj` foi **atualizado** para incluir um PostBuildEvent que automaticamente:
1. ✅ Compila o projeto
2. ✅ Copia `default.xex` para a pasta `dist/`
3. ✅ Copia `media/PsxSkin.xzp` para a pasta `dist/media/`
4. ✅ Cria pastas vazias: `bios/`, `memcards/`, `roms/`, `states/`
5. ✅ Gera ou copia `pcsx.ini`
6. ✅ **Cria o arquivo ZIP** `PCSXr360_LTCG.zip` (ou `PCSXr360_Release.zip`)

**Para usar:**
1. Compile o projeto no Visual Studio (configuração `Release_OP` ou `Release`)
2. Aguarde a mensagem de sucesso no Output Window
3. O arquivo ZIP estará em: `360/Xdk/pcsxr/Release_OP/PCSXr360_LTCG.zip`

### 📁 Opção 2: Scripts de Build Manual

Se o PostBuildEvent não funcionar, use um dos scripts na raiz do projeto:

#### Script Completo (Build + Empacotar)
```batch
# Na raiz do projeto (C:\Users\Misa\Documents\GitHub\PCSXr360)
build_and_package.bat
```
Este script:
- Encontra o Visual Studio 2010
- Compila a solution completa
- Cria o arquivo XZP (se xuipkg disponível)
- Empacota tudo em um ZIP

#### Script Rápido (Apenas Empacotar)
```batch
# Se você já compilou manualmente no Visual Studio
package_only.bat
```
Este script:
- Assume que `default.xex` e `media/PsxSkin.xzp` já existem
- Cria a estrutura de pastas
- Gera o ZIP

### 🛠️ Opção 3: Script Manual (post_build_setup.bat)
Execute o script `post_build_setup.bat` após a compilação:
```batch
cd 360\Xdk\pcsxr
post_build_setup.bat
```

Este script irá:
- Criar todas as pastas necessárias
- Copiar arquivos da build de referência (se disponível)
- Gerar o arquivo XZP

**⚠️ Nota:** Este script não cria ZIP automaticamente.

### 📝 Opção 4: Cópia Manual
Copie as pastas da build de referência:
1. **Obrigatório:** Copie `C:\Users\Misa\Downloads\PCSXR360.2.1.1a\PCSXR360\bios` para `Release\bios` (deve conter `SCPH1001.BIN`)
2. Opcional: Copie `C:\Users\Misa\Downloads\PCSXR360.2.1.1a\PCSXR360\hlsl` para `Release\hlsl`
3. Opcional: Copie `C:\Users\Misa\Downloads\PCSXR360.2.1.1a\PCSXR360\covers` para `Release\covers`
4. Opcional: Copie `C:\Users\Misa\Downloads\PCSXR360.2.1.1a\PCSXR360\gameguides` para `Release\gameguides`
5. Opcional: Copie `C:\Users\Misa\Downloads\PCSXR360.2.1.1a\PCSXR360\pcsx.ini` para `Release\pcsx.ini` (se não copiar, um padrão será criado)
6. Execute: `xuipkg /nologo /d /a Release\media\PsxSkin.xzp media\* media\Graphics\*`

**Nota:** A pasta `memcards/` não precisa ser copiada - ela será criada automaticamente!

## Notas Importantes

1. **⚠️ BIOS Obrigatório**: O arquivo `SCPH1001.BIN` é necessário mas não está incluso no repositório por questões de copyright. Você deve obtê-lo de um PlayStation 1 real ou de outra fonte legal.
   - Coloque o arquivo na pasta `bios/` antes de rodar o emulador
   - Sem o BIOS, o emulador não funcionará!

2. **✅ Memory Cards Automáticos**: Não precisa se preocupar com arquivos `.mcd`! O emulador cria automaticamente:
   - `Memcard1.mcd` - Memory Card Slot 1
   - `Memcard2.mcd` - Memory Card Slot 2
   
   Eles são criados na primeira vez que você rodar um jogo e serão salvos automaticamente.

3. **xuipkg**: A ferramenta `xuipkg` é necessária para criar o arquivo `.xzp`. Ela geralmente está incluída no Xbox 360 SDK.

## Configuração do Projeto Visual Studio

As seguintes modificações foram feitas no arquivo `pcsxr.vcxproj`:

### ✅ PostBuildEvent Ativado (Release_OP|Xbox 360)

**Alteração importante:** O `PostBuildEventUseInBuild` foi mudado de `false` para `true` para a configuração `Release_OP`.

Agora o PostBuildEvent:
1. Verifica se `default.xex` existe
2. Cria a pasta `dist/` e `dist/media/`
3. Copia os arquivos necessários
4. Cria pastas vazias para estrutura
5. Gera/copia `pcsx.ini`
6. **Cria o ZIP automaticamente** usando PowerShell

```xml
<PropertyGroup>
  <PostBuildEventUseInBuild Condition="'$(Configuration)|$(Platform)'=='Release_OP|Xbox 360'">true</PostBuildEventUseInBuild>
</PropertyGroup>

<PostBuildEvent>
  <Command>
    echo [PCSXR360] Post-Build Packaging...
    if not exist "$(OutDir)dist" mkdir "$(OutDir)dist"
    if not exist "$(OutDir)dist\media" mkdir "$(OutDir)dist\media"
    copy /Y "$(OutDir)default.xex" "$(OutDir)dist\default.xex"
    copy /Y "$(OutDir)media\PsxSkin.xzp" "$(OutDir)dist\media\PsxSkin.xzp"
    ...
    powershell -Command "Compress-Archive -Path '$(OutDir)dist\*' -DestinationPath '$(OutDir)PCSXr360_LTCG.zip' -Force"
  </Command>
</PostBuildEvent>
```

### PostBuildEvent (Release|Xbox 360)

Também atualizado para criar ZIP:
- Copia pasta `media/`
- Tenta criar `PsxSkin.xzp` usando xuipkg
- Cria pacote `PCSXr360_Release.zip`

### Arquivos de Script Adicionados

Novos scripts na raiz do projeto:
- **`build_and_package.bat`** - Build completo + empacotamento
- **`package_only.bat`** - Empacotamento rápido (após build manual)

### Troubleshooting

**Se o ZIP não for criado automaticamente:**
1. Verifique se PowerShell está disponível: `powershell -Command "Get-Host"`
2. Execute manualmente: `package_only.bat`
3. Ou crie manualmente: compacte a pasta `dist/` em um ZIP

**Se o xuipkg falhar:**
1. Verifique se o Xbox 360 SDK está instalado
2. O arquivo XZP pode ser criado manualmente: 
   ```batch
   xuipkg /nologo /d /a Release_OP\media\PsxSkin.xzp 360\Xdk\pcsxr\media\* 360\Xdk\pcsxr\media\Graphics\*
   ```

Isso garante que todos os arquivos necessários sejam copiados e empacotados automaticamente após a compilação.

---

## 🎮 Primeira Execução (O que Esperar)

Quando você rodar o emulador pela primeira vez:

1. **O emulador inicia** - Carrega `default.xex`
2. **Criação automática de arquivos**:
   - Memory cards (`memcards/Memcard1.mcd` e `Memcard2.mcd`) são criados automaticamente
   - Save states serão salvos em `states/`
3. **Tela inicial** - Aparece a interface XUI com o menu principal
4. **Para jogar**:
   - Adicione seus jogos (ISOs/BINs) na pasta `roms/`
   - Configure o caminho do BIOS em `bios/` (deve ser `SCPH1001.BIN`)
   - Selecione um jogo e divirta-se!

**💾 Seus saves serão preservados:**
- Memory cards são salvos automaticamente quando você salva no jogo
- Save states (salvamento rápido) são armazenados em `states/`
- Configurações são salvas em `pcsx.ini`

**⚠️ Importante:** O único arquivo que você realmente precisa adicionar manualmente é o BIOS (`SCPH1001.BIN`). Todo o resto é gerenciado automaticamente pelo emulador!
