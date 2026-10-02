# 📝 Guia de Uso - Pasta de Artigo

## Estrutura Criada

```
Artigo/
├── 01_conteudo/          → Arquivo principal do artigo (Markdown)
├── 02_imagens/           → Imagens, screenshots e diagramas
├── 03_codigos/           → Exemplos de código (.cs, .csproj, etc)
├── 04_referencias/       → Links, fontes e materiais de referência
├── 05_drafts/            → Rascunhos e versões anteriores
├── README.md             → Índice da pasta
├── GUIA_USO.md          → Este arquivo
└── artigo_principal.md   → Template do artigo (em 01_conteudo)
```

---

## 📂 Como Usar Cada Pasta

### 01_conteudo
Coloque aqui os arquivos Markdown com o conteúdo principal do artigo.

**Arquivos sugeridos:**
- `artigo_principal.md` - Artigo completo
- `secao_1.md` - Seções separadas (opcional)

### 02_imagens
Armazene todas as imagens, screenshots e diagramas aqui.

**Tipos de arquivo aceitos:**
- `.png`, `.jpg`, `.jpeg`
- `.svg` (para diagramas vetoriais)
- `.gif` (para animações)

**Nomenclatura sugerida:**
- `diagrama_fluxo.png`
- `screenshot_01.png`
- `grafico_resultados.png`

### 03_codigos
Coloque exemplos de código e trechos reutilizáveis.

**Organização:**
```
03_codigos/
├── exemplo_1.cs
├── exemplo_2.cs
├── classe_exemplo.cs
└── snippets/
    ├── inicializacao.cs
    ├── processamento.cs
    └── limpeza.cs
```

### 04_referencias
Lista de referências, links úteis e fontes.

**Formato sugerido:**
- `referencias.md` - Lista de links e fontes
- `referencias.json` - Formato estruturado
- `citacoes.txt` - Citações importantes

### 05_drafts
Versões anteriores, rascunhos e notas durante o desenvolvimento.

**Exemplo:**
```
05_drafts/
├── v1_rascunho.md
├── v2_primeira_versao.md
├── notas_capitulo_1.txt
└── ideias_futuras.md
```

---

## ✏️ Fluxo de Trabalho Recomendado

1. **Planejamento** 
   - Escreva o outline no `artigo_principal.md`
   - Use `05_drafts/ideias.md` para brainstorming

2. **Rascunho**
   - Escreva o conteúdo inicial
   - Salve versões no `05_drafts/`

3. **Desenvolvimento**
   - Adicione exemplos em `03_codigos/`
   - Crie screenshots em `02_imagens/`
   - Organize referências em `04_referencias/`

4. **Revisão**
   - Revise o conteúdo
   - Corrija formatação
   - Valide links e referências

5. **Finalização**
   - Mova a versão final para `01_conteudo/`
   - Arquivo antigos em `05_drafts/`

---

## 📋 Checklist de Artigo

- [ ] Definir título e escopo
- [ ] Criar outline
- [ ] Escrever introdução
- [ ] Desenvolver seções principais
- [ ] Adicionar exemplos de código
- [ ] Incluir imagens e diagramas
- [ ] Revisar ortografia e gramática
- [ ] Validar todos os links
- [ ] Formatar para o formato final
- [ ] Preparar versão de publicação

---

## 💡 Dicas

✅ **Boas práticas:**
- Use nomes descritivos para arquivos
- Mantenha imagens organizadas por tema
- Documente fontes de referências
- Faça commits frequentes das mudanças

❌ **Evite:**
- Misturar rascunhos com versão final
- Usar nomes genéricos (ex: "artigo.md")
- Deixar imagens soltas sem organização

---

**Criado em:** 02/10/2026  
**Última atualização:** 02/10/2026
