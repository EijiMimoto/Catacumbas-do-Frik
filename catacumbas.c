/*
 * ============================================================
 *  CATACUMBAS DE FRIK - RPG de texto em C (versao MVP)
 * ============================================================
 *  Compilar (GCC/MinGW):
 *      gcc catacumbas.c -o catacumbas.exe -Wall -Wextra
 *
 *  IMPORTANTE: salve este arquivo com codificacao UTF-8.
 *  Estrutura do arquivo:
 *      1) Constantes (#define)
 *      2) Structs (Arma, Monstro, Personagem)
 *      3) Dados fixos (catalogo de armas, monstros, racas, classes)
 *      4) Variaveis de estado do jogo (globais)
 *      5) Funcoes: entrada, tela, personagem, inventario,
 *         combate, exploracao, loja, fim de jogo e main()
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------
 * 1) CONSTANTES - altere aqui para ajustar o equilibrio do jogo
 * ------------------------------------------------------------ */
#define TAM_NOME 21             /* 20 letras + '\0' (fim do texto)  */
#define TAM_TEXTO 40            /* tamanho de textos curtos         */
#define MAX_INVENTARIO 8        /* maximo de armas carregadas       */
#define MAX_POCOES 9            /* maximo de pocoes carregadas      */
#define CURA_POCAO 40           /* vida restaurada por pocao        */
#define PRECO_POCAO 15          /* preco da pocao na loja           */
#define OURO_INICIAL 25
#define POCOES_INICIAIS 1
#define TOTAL_PISOS 3
#define TOTAL_ARMAS 7
#define TOTAL_MONSTROS 6
#define MAX_TESOUROS_PISO 3     /* tesouros que cada piso pode dar  */
#define CRITICO_INIMIGO 5       /* % de critico dos monstros        */
#define ESPECIAL_A_CADA 3       /* Frik usa o golpe especial a cada 3 turnos */
#define LIMITE_ESPECIAL 40      /* golpe especial tira no maximo 40% da vida maxima */

/* Estados possiveis da partida */
#define ESTADO_JOGANDO 0
#define ESTADO_VITORIA 1
#define ESTADO_DERROTA 2
#define ESTADO_SAIU 3

/* Resultados possiveis de um combate */
#define COMBATE_VENCEU 1
#define COMBATE_FUGIU 2
#define COMBATE_MORREU 3

/* ------------------------------------------------------------
 * 2) STRUCTS - "caixas" que agrupam varios dados de uma coisa so
 * ------------------------------------------------------------ */
typedef struct {
    char nome[TAM_TEXTO];
    int bonusAtaque;            /* soma ao ataque do personagem */
    char categoria[TAM_TEXTO];  /* Espada, Adaga, Machado, Cajado */
    int valorVenda;             /* ouro recebido ao vender      */
    int preco;                  /* ouro gasto ao comprar        */
} Arma;

typedef struct {
    char nome[TAM_TEXTO];
    int vidaMax;
    int vida;
    int ataque;
    int defesa;
    int ouro;                   /* recompensa base em ouro      */
    int chanceArma;             /* % de chance de largar arma   */
    int ehBoss;                 /* 1 = Frik, 0 = monstro comum  */
} Monstro;

typedef struct {
    char nome[TAM_NOME];
    char raca[TAM_TEXTO];
    char classe[TAM_TEXTO];
    int vidaMax;
    int vida;
    int ataque;                 /* ataque BASE (sem a arma)     */
    int defesa;
    int destreza;
    int critico;                /* % de chance de critico       */
    int ouro;
    int pocoes;
    Arma inventario[MAX_INVENTARIO];
    int qtdArmas;               /* quantas armas ha no inventario */
    int armaEquipada;           /* POSICAO da arma equipada no inventario */
} Personagem;

/* ------------------------------------------------------------
 * 3) DADOS FIXOS (const = nao muda durante o jogo)
 * ------------------------------------------------------------ */
const char *NOMES_PISOS[TOTAL_PISOS + 1] = {
    "", "Catacumbas Exteriores", "Criptas Antigas", "Santuário de Frik"
};

/* Quantos monstros precisam ser derrotados em cada piso */
const int MONSTROS_NECESSARIOS[TOTAL_PISOS + 1] = {0, 4, 5, 5};

const char *NOMES_RACAS[3] = {"Humano", "Elfo", "Anão"};
const char *DESC_RACAS[3] = {
    "atributos equilibrados",
    "mais destreza e crítico, menos vida",
    "mais vida e defesa, menos destreza"
};
const char *NOMES_CLASSES[3] = {"Guerreiro", "Ladino", "Mago"};
const char *DESC_CLASSES[3] = {
    "muita vida e bom dano físico",
    "pouca vida, alta destreza e crítico",
    "pouca defesa, ataque mágico poderoso"
};

/* Cada linha: {vida, ataque, defesa, destreza, critico} */
const int BASE_CLASSE[3][5] = {
    {100, 12, 8,  5,  5},   /* Guerreiro */
    { 70, 10, 5, 12, 15},   /* Ladino    */
    { 80, 16, 3,  7,  8}    /* Mago      */
};
const int BONUS_RACA[3][5] = {
    {  0, 0, 0,  0, 0},     /* Humano */
    {-10, 0, 0,  4, 7},     /* Elfo   */
    { 20, 0, 3, -2, 0}      /* Anão   */
};
/* Arma inicial de cada classe (posicao no CATALOGO_ARMAS) */
const int ARMA_INICIAL[3] = {0, 1, 2};

/* Catalogo de armas: {nome, bonusAtaque, categoria, valorVenda, preco} */
const Arma CATALOGO_ARMAS[TOTAL_ARMAS] = {
    {"Espada Enferrujada",    2, "Espada",  5,  12},   /* 0 */
    {"Adaga Gasta",           2, "Adaga",   5,  12},   /* 1 */
    {"Cajado de Aprendiz",    2, "Cajado",  5,  12},   /* 2 */
    {"Adaga Sombria",         5, "Adaga",  15,  35},   /* 3 */
    {"Machado de Batalha",    7, "Machado",22,  55},   /* 4 */
    {"Cajado Arcano",         8, "Cajado", 28,  70},   /* 5 */
    {"Espada das Catacumbas",11, "Espada", 45, 110}    /* 6 */
};

/* Modelos de monstros: {nome, vidaMax, vida, ataque, defesa, ouro, chanceArma, ehBoss}
 * Posicoes 0-1 = piso 1, 2-3 = piso 2, 4-5 = piso 3 */
const Monstro MODELOS_MONSTROS[TOTAL_MONSTROS] = {
    {"Rato Gigante",           18, 18,  6,  1,  8,  5, 0},
    {"Esqueleto Guerreiro",    28, 28,  9,  3, 12, 12, 0},
    {"Aranha das Catacumbas",  36, 36, 11,  4, 22, 12, 0},
    {"Cavaleiro Amaldiçoado",  50, 50, 13,  7, 30, 20, 0},
    {"Guardião da Cripta",     60, 60, 15,  9, 40, 20, 0},
    {"Sacerdote Profano",      55, 55, 17,  7, 45, 20, 0}
};

const Monstro MODELO_FRIK = {
    "Frik, o Senhor das Catacumbas", 140, 140, 18, 11, 200, 0, 1
};

/* Armas vendidas em cada loja (posicoes do CATALOGO_ARMAS) */
const int LOJA_PISO1[2] = {3, 4};
const int LOJA_PISO2[3] = {4, 5, 6};

/* ------------------------------------------------------------
 * 4) ESTADO DO JOGO (variaveis globais: todas as funcoes enxergam)
 * ------------------------------------------------------------ */
Personagem jogador;                 /* o heroi                       */
Monstro inimigo;                    /* o inimigo do combate atual    */
int pisoAtual = 1;                  /* 1, 2 ou 3                     */
int monstrosDerrotados = 0;         /* progresso no piso atual       */
int tesourosPiso = 0;               /* tesouros ja achados no piso   */
int estadoJogo = ESTADO_JOGANDO;    /* jogando, vitoria, derrota...  */
int turnosInimigo = 0;              /* turnos do inimigo no combate  */

/* ------------------------------------------------------------
 * Prototipos: avisam ao compilador que as funcoes existem
 * ------------------------------------------------------------ */
void configurarConsole(void);
void limparTela(void);
void pausar(void);
void divisor(void);
void titulo(const char texto[]);
void lerLinha(char texto[], int tamanho);
int lerInteiro(const char pergunta[], int minimo, int maximo);
void lerNome(char destino[]);
void desenharBarra(int atual, int maximo);

void criarPersonagem(void);
void montarPersonagem(const char nome[], int raca, int classe);
void exibirCabecalho(void);
void exibirStatus(void);
void exibirEquipamento(void);
void listarInventario(void);
void verInventario(void);
void equiparArma(void);
int adicionarArmaInventario(int indiceCatalogo);
void removerArmaInventario(int posicao);
int usarPocao(void);
void curarCompleto(void);

int pisoLimpo(void);
void explorarDungeon(void);
void encontrarMonstro(void);
void encontrarOuro(void);
void encontrarArma(void);
void encontrarPocao(void);
int sortearMonstro(int piso);
int sortearArma(int piso);
void distribuirRecompensas(void);
void avancarPiso(void);

int rolar(int chance);
int calcularDano(int ataque, int defesa, int critico);
int ataqueTotalJogador(void);
int chanceAcertoJogador(void);
int chanceAcertoInimigo(void);
void exibirPainelCombate(void);
void exibirInimigo(void);
void atacarInimigo(void);
void turnoInimigo(void);
void golpeEspecialFrik(void);
int tentarFugir(void);
int iniciarCombate(int permiteFuga);
void enfrentarFrik(void);

void enfrentarVendedor(int piso);
void comprarPocoes(void);
void comprarArmas(const int estoque[], int qtd);
void venderArmas(void);

void gameOver(void);
void telaVitoria(void);

/* ============================================================
 * 5) FUNCOES DE ENTRADA E TELA
 * ============================================================ */

/* Prepara o console. No Windows, chcp 65001 ativa UTF-8 para os acentos.
 * Se aparecerem simbolos estranhos no lugar dos acentos, apague a linha do system. */
void configurarConsole(void) {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
}

/* system("cls") e "clear" dependem do sistema operacional: nao sao portaveis. */
void limparTela(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* Alternativa portavel ao system("pause"): espera o jogador apertar ENTER */
void pausar(void) {
    char lixo[64];
    printf("\nPressione ENTER para continuar...");
    fflush(stdout);
    lerLinha(lixo, sizeof(lixo));
}

void divisor(void) {
    printf("==================================================\n");
}

void titulo(const char texto[]) {
    divisor();
    printf("  %s\n", texto);
    divisor();
}

/* Le UMA linha do teclado com fgets (seguro: nunca passa do tamanho do vetor).
 * Remove o '\n' final e descarta o que sobrar se o jogador digitar demais. */
void lerLinha(char texto[], int tamanho) {
    int tamanhoLido, c;

    if (fgets(texto, tamanho, stdin) == NULL) {
        /* teclado fechado (Ctrl+Z / Ctrl+D): sai para nao ficar em loop infinito */
        printf("\nEntrada encerrada. Saindo do jogo.\n");
        exit(0);
    }
    tamanhoLido = (int)strlen(texto);
    if (tamanhoLido > 0 && texto[tamanhoLido - 1] == '\n') {
        texto[tamanhoLido - 1] = '\0';
        tamanhoLido--;
        if (tamanhoLido > 0 && texto[tamanhoLido - 1] == '\r') {
            texto[tamanhoLido - 1] = '\0';
        }
    } else {
        /* a linha era maior que o vetor: joga fora o resto dela */
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
}

/* Pede um numero inteiro entre minimo e maximo. Repete (com while) ate ser valido. */
int lerInteiro(const char pergunta[], int minimo, int maximo) {
    char linha[64];
    char *resto;
    long valor;

    while (1) {
        printf("%s", pergunta);
        fflush(stdout);
        lerLinha(linha, sizeof(linha));

        if (linha[0] == '\0') {
            printf("Entrada vazia. Digite um número.\n");
            continue;
        }
        valor = strtol(linha, &resto, 10);   /* converte texto em numero */
        while (*resto == ' ') {
            resto++;
        }
        if (*resto != '\0') {
            printf("Entrada inválida. Digite apenas números.\n");
            continue;
        }
        if (valor < minimo || valor > maximo) {
            printf("Opção inválida. Digite um número de %d a %d.\n", minimo, maximo);
            continue;
        }
        return (int)valor;
    }
}

/* Pede o nome do heroi: de 2 a 20 caracteres e com pelo menos uma letra/numero */
void lerNome(char destino[]) {
    char linha[64];
    int i, valido, tamanho;

    while (1) {
        printf("Digite o nome do seu herói (2 a 20 caracteres): ");
        fflush(stdout);
        lerLinha(linha, sizeof(linha));
        tamanho = (int)strlen(linha);

        valido = 0;
        for (i = 0; i < tamanho; i++) {
            if (linha[i] != ' ') {
                valido = 1;
            }
        }
        if (!valido || tamanho < 2) {
            printf("Nome inválido: use pelo menos 2 caracteres.\n");
            continue;
        }
        if (tamanho > TAM_NOME - 1) {
            printf("Nome muito longo: use no máximo 20 caracteres.\n");
            continue;
        }
        strcpy(destino, linha);
        return;
    }
}

/* Barra de vida: [########------------] 20/50 (exemplo de uso do for) */
void desenharBarra(int atual, int maximo) {
    int i, cheios;
    int total = 20;

    cheios = (maximo > 0) ? (atual * total / maximo) : 0;
    if (atual > 0 && cheios == 0) {
        cheios = 1;
    }
    printf("[");
    for (i = 0; i < total; i++) {
        printf(i < cheios ? "#" : "-");
    }
    printf("] %d/%d", atual, maximo);
}

/* ============================================================
 * PERSONAGEM E INVENTARIO
 * ============================================================ */

/* Guarda no inventario uma copia da arma do catalogo. Retorna 1 se deu certo, 0 se cheio. */
int adicionarArmaInventario(int indiceCatalogo) {
    if (indiceCatalogo < 0 || indiceCatalogo >= TOTAL_ARMAS) {
        return 0;
    }
    if (jogador.qtdArmas >= MAX_INVENTARIO) {
        return 0;
    }
    jogador.inventario[jogador.qtdArmas] = CATALOGO_ARMAS[indiceCatalogo];
    jogador.qtdArmas++;
    return 1;
}

/* Remove a arma da posicao indicada e "puxa" as seguintes para preencher o buraco */
void removerArmaInventario(int posicao) {
    int i;

    if (posicao < 0 || posicao >= jogador.qtdArmas) {
        return;
    }
    for (i = posicao; i < jogador.qtdArmas - 1; i++) {
        jogador.inventario[i] = jogador.inventario[i + 1];
    }
    jogador.qtdArmas--;
    /* a arma equipada pode ter mudado de posicao */
    if (posicao < jogador.armaEquipada) {
        jogador.armaEquipada--;
    }
}

/* Combina a base da CLASSE com o bonus da RACA e prepara o heroi */
void montarPersonagem(const char nome[], int raca, int classe) {
    strcpy(jogador.nome, nome);
    strcpy(jogador.raca, NOMES_RACAS[raca]);
    strcpy(jogador.classe, NOMES_CLASSES[classe]);

    jogador.vidaMax  = BASE_CLASSE[classe][0] + BONUS_RACA[raca][0];
    jogador.vida     = jogador.vidaMax;
    jogador.ataque   = BASE_CLASSE[classe][1] + BONUS_RACA[raca][1];
    jogador.defesa   = BASE_CLASSE[classe][2] + BONUS_RACA[raca][2];
    jogador.destreza = BASE_CLASSE[classe][3] + BONUS_RACA[raca][3];
    jogador.critico  = BASE_CLASSE[classe][4] + BONUS_RACA[raca][4];
    jogador.ouro     = OURO_INICIAL;
    jogador.pocoes   = POCOES_INICIAIS;

    jogador.qtdArmas = 0;
    adicionarArmaInventario(ARMA_INICIAL[classe]);
    jogador.armaEquipada = 0;

    /* reinicia o estado da aventura */
    pisoAtual = 1;
    monstrosDerrotados = 0;
    tesourosPiso = 0;
    estadoJogo = ESTADO_JOGANDO;
}

void criarPersonagem(void) {
    char nome[TAM_NOME];
    int raca, classe, i, confirmado = 0;

    while (!confirmado) {
        limparTela();
        titulo("CRIAÇÃO DE PERSONAGEM");
        lerNome(nome);

        printf("\nEscolha sua raça:\n");
        for (i = 0; i < 3; i++) {
            printf("  %d - %s: %s\n", i + 1, NOMES_RACAS[i], DESC_RACAS[i]);
        }
        raca = lerInteiro("Raça: ", 1, 3) - 1;

        printf("\nEscolha sua classe:\n");
        for (i = 0; i < 3; i++) {
            printf("  %d - %s: %s\n", i + 1, NOMES_CLASSES[i], DESC_CLASSES[i]);
        }
        classe = lerInteiro("Classe: ", 1, 3) - 1;

        montarPersonagem(nome, raca, classe);

        limparTela();
        titulo("SEU PERSONAGEM");
        exibirStatus();
        printf("\n1 - Confirmar e começar a aventura\n");
        printf("2 - Refazer o personagem\n");
        confirmado = (lerInteiro("Escolha uma opção: ", 1, 2) == 1);
    }
}

void exibirCabecalho(void) {
    limparTela();
    titulo("CATACUMBAS DE FRIK");
    printf("Personagem: %s   Raça: %s   Classe: %s\n", jogador.nome, jogador.raca, jogador.classe);
    printf("Vida: %d/%d   Ataque: %d   Defesa: %d   Ouro: %d   Poções: %d\n",
           jogador.vida, jogador.vidaMax, ataqueTotalJogador(), jogador.defesa,
           jogador.ouro, jogador.pocoes);
    printf("\nPiso atual: %d - %s\n", pisoAtual, NOMES_PISOS[pisoAtual]);
    if (pisoLimpo()) {
        printf("Progresso: piso concluído (%d/%d monstros).\n",
               monstrosDerrotados, MONSTROS_NECESSARIOS[pisoAtual]);
    } else {
        printf("Progresso: %d/%d monstros derrotados.\n",
               monstrosDerrotados, MONSTROS_NECESSARIOS[pisoAtual]);
    }
    divisor();
}

void exibirStatus(void) {
    printf("Nome: %s\n", jogador.nome);
    printf("Raça: %s   Classe: %s\n", jogador.raca, jogador.classe);
    printf("Vida: %d/%d\n", jogador.vida, jogador.vidaMax);
    printf("Ataque: %d (base %d + arma %d)\n", ataqueTotalJogador(), jogador.ataque,
           jogador.inventario[jogador.armaEquipada].bonusAtaque);
    printf("Defesa: %d\n", jogador.defesa);
    printf("Destreza: %d (chance de acertar: %d%%)\n", jogador.destreza, chanceAcertoJogador());
    printf("Crítico: %d%%\n", jogador.critico);
    printf("Ouro: %d   Poções: %d\n", jogador.ouro, jogador.pocoes);
    printf("Arma equipada: %s\n", jogador.inventario[jogador.armaEquipada].nome);
}

void exibirEquipamento(void) {
    Arma arma = jogador.inventario[jogador.armaEquipada];

    titulo("EQUIPAMENTO");
    printf("Arma equipada: %s\n", arma.nome);
    printf("Categoria: %s\n", arma.categoria);
    printf("Bônus de ataque: +%d\n", arma.bonusAtaque);
    printf("Valor de venda: %d ouro\n", arma.valorVenda);
    printf("Ataque total: %d\n", ataqueTotalJogador());
}

void listarInventario(void) {
    int i;

    printf("Armas (%d/%d):\n", jogador.qtdArmas, MAX_INVENTARIO);
    for (i = 0; i < jogador.qtdArmas; i++) {
        printf("  %d - %s (+%d ataque, %s, vale %d ouro)%s\n", i + 1,
               jogador.inventario[i].nome, jogador.inventario[i].bonusAtaque,
               jogador.inventario[i].categoria, jogador.inventario[i].valorVenda,
               (i == jogador.armaEquipada) ? "  [EQUIPADA]" : "");
    }
}

void verInventario(void) {
    titulo("INVENTÁRIO");
    listarInventario();
    printf("\nPoções de cura: %d/%d (cada uma cura %d de vida)\n", jogador.pocoes, MAX_POCOES, CURA_POCAO);
    printf("Ouro: %d\n", jogador.ouro);
}

void equiparArma(void) {
    int escolha;

    titulo("EQUIPAR ARMA");
    listarInventario();
    escolha = lerInteiro("\nNúmero da arma a equipar (0 = cancelar): ", 0, jogador.qtdArmas);
    if (escolha == 0) {
        printf("Nada foi alterado.\n");
        return;
    }
    jogador.armaEquipada = escolha - 1;
    printf("Você empunha %s. Ataque total agora: %d.\n",
           jogador.inventario[jogador.armaEquipada].nome, ataqueTotalJogador());
}

/* Usa uma pocao. Retorna 1 se gastou a pocao, 0 se nao foi possivel. */
int usarPocao(void) {
    int cura;

    if (jogador.pocoes <= 0) {
        printf("Você não tem nenhuma poção.\n");
        return 0;
    }
    if (jogador.vida >= jogador.vidaMax) {
        printf("Sua vida já está cheia. A poção não foi usada.\n");
        return 0;
    }
    cura = CURA_POCAO;
    if (jogador.vida + cura > jogador.vidaMax) {
        cura = jogador.vidaMax - jogador.vida;   /* nunca passa do maximo */
    }
    jogador.vida += cura;
    jogador.pocoes--;
    printf("Você bebe uma poção e recupera %d de vida. (Vida: %d/%d, poções restantes: %d)\n",
           cura, jogador.vida, jogador.vidaMax, jogador.pocoes);
    return 1;
}

void curarCompleto(void) {
    jogador.vida = jogador.vidaMax;
}

/* ============================================================
 * REGRAS DE COMBATE (ficam em um unico lugar, sem duplicacao)
 *
 *  - Acertar:  jogador = 70 + destreza (max 95%)
 *              inimigo = 90 - destreza do jogador (min 50%)
 *  - Dano    = ataque + (0 a 2) - defesa_do_alvo / 2   (minimo 1)
 *  - Critico = dano x 2
 * ============================================================ */

/* Retorna 1 com 'chance' por cento de probabilidade */
int rolar(int chance) {
    return (rand() % 100) < chance;
}

int calcularDano(int ataque, int defesa, int critico) {
    int dano = ataque + (rand() % 3) - defesa / 2;

    if (dano < 1) {
        dano = 1;
    }
    if (critico) {
        dano = dano * 2;
    }
    return dano;
}

int ataqueTotalJogador(void) {
    return jogador.ataque + jogador.inventario[jogador.armaEquipada].bonusAtaque;
}

int chanceAcertoJogador(void) {
    int chance = 70 + jogador.destreza;
    if (chance > 95) {
        chance = 95;
    }
    return chance;
}

int chanceAcertoInimigo(void) {
    int chance = 90 - jogador.destreza;
    if (chance < 50) {
        chance = 50;
    }
    return chance;
}

void exibirPainelCombate(void) {
    limparTela();
    titulo("COMBATE");
    printf("%s  ", jogador.nome);
    desenharBarra(jogador.vida, jogador.vidaMax);
    printf("\n%s  ", inimigo.nome);
    desenharBarra(inimigo.vida, inimigo.vidaMax);
    printf("\n");
    divisor();
}

void exibirInimigo(void) {
    printf("--- %s ---\n", inimigo.nome);
    printf("Vida: %d/%d\n", inimigo.vida, inimigo.vidaMax);
    printf("Ataque: %d\n", inimigo.ataque);
    printf("Defesa: %d\n", inimigo.defesa);
    if (inimigo.ehBoss) {
        printf("Habilidade: Lamento do Sepulcro (a cada %d turnos, ignora sua defesa).\n", ESPECIAL_A_CADA);
    }
}

void atacarInimigo(void) {
    int critico, dano;

    if (jogador.vida <= 0 || inimigo.vida <= 0) {
        return;   /* ninguem morto ataca */
    }
    if (!rolar(chanceAcertoJogador())) {
        printf("Você ataca, mas %s desvia do golpe!\n", inimigo.nome);
        return;
    }
    critico = rolar(jogador.critico);
    dano = calcularDano(ataqueTotalJogador(), inimigo.defesa, critico);
    inimigo.vida -= dano;
    if (inimigo.vida < 0) {
        inimigo.vida = 0;
    }
    if (critico) {
        printf("GOLPE CRÍTICO! Você causa %d de dano em %s.\n", dano, inimigo.nome);
    } else {
        printf("Você acerta %s e causa %d de dano.\n", inimigo.nome, dano);
    }
}

/* Golpe especial de Frik: ignora a defesa, mas e avisado antes e limitado a 40% da vida maxima */
void golpeEspecialFrik(void) {
    int dano, limite;

    dano = inimigo.ataque * 3 / 2;                      /* ignora a defesa */
    limite = jogador.vidaMax * LIMITE_ESPECIAL / 100;
    if (dano > limite) {
        dano = limite;
    }
    if (dano < 1) {
        dano = 1;
    }
    printf("Frik solta o LAMENTO DO SEPULCRO! Uma onda de sombras atravessa sua armadura!\n");
    jogador.vida -= dano;
    if (jogador.vida < 0) {
        jogador.vida = 0;
    }
    printf("Você sofre %d de dano (a defesa foi ignorada).\n", dano);
}

void turnoInimigo(void) {
    int critico, dano;

    if (inimigo.vida <= 0 || jogador.vida <= 0) {
        return;   /* inimigo derrotado nao age */
    }
    turnosInimigo++;

    if (inimigo.ehBoss && turnosInimigo % ESPECIAL_A_CADA == 0) {
        golpeEspecialFrik();
        return;
    }
    if (inimigo.ehBoss && turnosInimigo % ESPECIAL_A_CADA == ESPECIAL_A_CADA - 1) {
        printf("Frik ergue os braços e as sombras se reúnem... algo terrível se aproxima!\n");
    }
    if (!rolar(chanceAcertoInimigo())) {
        printf("%s ataca, mas você se esquiva!\n", inimigo.nome);
        return;
    }
    critico = rolar(CRITICO_INIMIGO);
    dano = calcularDano(inimigo.ataque, jogador.defesa, critico);
    jogador.vida -= dano;
    if (jogador.vida < 0) {
        jogador.vida = 0;
    }
    if (critico) {
        printf("%s acerta um golpe CRÍTICO e causa %d de dano!\n", inimigo.nome, dano);
    } else {
        printf("%s ataca e causa %d de dano em você.\n", inimigo.nome, dano);
    }
}

/* Retorna 1 se conseguiu fugir */
int tentarFugir(void) {
    int chance = 40 + jogador.destreza * 2;
    if (chance > 90) {
        chance = 90;
    }
    if (rolar(chance)) {
        printf("Você dá meia-volta e escapa na escuridão!\n");
        return 1;
    }
    printf("Você tenta fugir, mas %s bloqueia o caminho!\n", inimigo.nome);
    return 0;
}

/* Combate por turnos. Usa o monstro guardado na variavel global 'inimigo'.
 * Retorna COMBATE_VENCEU, COMBATE_FUGIU ou COMBATE_MORREU. */
int iniciarCombate(int permiteFuga) {
    int opcao, maxOpcao, gastouTurno;

    turnosInimigo = 0;
    while (jogador.vida > 0 && inimigo.vida > 0) {
        exibirPainelCombate();
        printf("1 - Atacar\n");
        printf("2 - Usar poção (você tem %d)\n", jogador.pocoes);
        printf("3 - Ver meus atributos\n");
        printf("4 - Ver informações do inimigo\n");
        if (permiteFuga) {
            printf("5 - Tentar fugir\n");
        }
        maxOpcao = permiteFuga ? 5 : 4;
        opcao = lerInteiro("Escolha uma ação: ", 1, maxOpcao);
        printf("\n");

        gastouTurno = 0;
        switch (opcao) {
            case 1:
                atacarInimigo();
                gastouTurno = 1;
                break;
            case 2:
                gastouTurno = usarPocao();   /* so gasta o turno se usou mesmo */
                break;
            case 3:
                exibirStatus();
                break;
            case 4:
                exibirInimigo();
                break;
            case 5:
                if (tentarFugir()) {
                    pausar();
                    return COMBATE_FUGIU;
                }
                gastouTurno = 1;
                break;
        }
        if (gastouTurno) {
            turnoInimigo();   /* so age se ainda estiver vivo (checado la dentro) */
        }
        pausar();
    }

    if (jogador.vida <= 0) {
        return COMBATE_MORREU;
    }
    return COMBATE_VENCEU;
}

/* ============================================================
 * EXPLORACAO DA DUNGEON
 * ============================================================ */

int pisoLimpo(void) {
    return monstrosDerrotados >= MONSTROS_NECESSARIOS[pisoAtual];
}

/* Piso 1 sorteia monstros 0-1, piso 2 sorteia 2-3, piso 3 sorteia 4-5 */
int sortearMonstro(int piso) {
    return (piso - 1) * 2 + rand() % 2;
}

/* Armas que podem aparecer em cada piso (mais fortes nos pisos finais) */
int sortearArma(int piso) {
    if (piso == 1) {
        return rand() % 4;        /* 0 a 3 */
    }
    if (piso == 2) {
        return 3 + rand() % 3;    /* 3 a 5 */
    }
    return 4 + rand() % 3;        /* 4 a 6 */
}

void distribuirRecompensas(void) {
    int ouro, indiceArma;

    ouro = inimigo.ouro + rand() % (inimigo.ouro / 2 + 1);
    jogador.ouro += ouro;
    printf("Recompensa: você encontra %d de ouro com os restos de %s.\n", ouro, inimigo.nome);

    if (rolar(inimigo.chanceArma)) {
        indiceArma = sortearArma(pisoAtual);
        printf("Entre os despojos há uma arma: %s (+%d ataque)!\n",
               CATALOGO_ARMAS[indiceArma].nome, CATALOGO_ARMAS[indiceArma].bonusAtaque);
        if (adicionarArmaInventario(indiceArma)) {
            printf("Ela foi guardada no inventário.\n");
        } else {
            jogador.ouro += CATALOGO_ARMAS[indiceArma].valorVenda;
            printf("Seu inventário está cheio! Você vende a arma na hora por %d de ouro.\n",
                   CATALOGO_ARMAS[indiceArma].valorVenda);
        }
    }
}

void encontrarMonstro(void) {
    int resultado;

    inimigo = MODELOS_MONSTROS[sortearMonstro(pisoAtual)];   /* copia o modelo */
    printf("Passos ecoam no escuro... %s surge à sua frente!\n", inimigo.nome);
    pausar();

    resultado = iniciarCombate(1);
    switch (resultado) {
        case COMBATE_VENCEU:
            monstrosDerrotados++;
            printf("\n%s tomba sem vida. O silêncio retorna às catacumbas.\n", inimigo.nome);
            distribuirRecompensas();
            break;
        case COMBATE_FUGIU:
            printf("\nVocê escapou, mas %s continua rondando por aí.\n", inimigo.nome);
            break;
        case COMBATE_MORREU:
            gameOver();
            break;
    }
}

void encontrarOuro(void) {
    int ouro = 10 * pisoAtual + rand() % (10 * pisoAtual + 1);

    tesourosPiso++;
    jogador.ouro += ouro;
    printf("Atrás de uma pilha de ossos você encontra uma bolsa com %d de ouro!\n", ouro);
}

void encontrarArma(void) {
    int indice = sortearArma(pisoAtual);

    tesourosPiso++;
    printf("Entre os escombros jaz uma arma abandonada: %s (+%d ataque)!\n",
           CATALOGO_ARMAS[indice].nome, CATALOGO_ARMAS[indice].bonusAtaque);
    if (adicionarArmaInventario(indice)) {
        printf("Você a guarda no inventário.\n");
    } else {
        jogador.ouro += CATALOGO_ARMAS[indice].valorVenda;
        printf("Inventário cheio! Você a vende por %d de ouro.\n", CATALOGO_ARMAS[indice].valorVenda);
    }
}

void encontrarPocao(void) {
    tesourosPiso++;
    if (jogador.pocoes < MAX_POCOES) {
        jogador.pocoes++;
        printf("Um frasco intacto brilha num nicho: você encontra uma poção de cura! (Total: %d)\n",
               jogador.pocoes);
    } else {
        printf("Você acha uma poção, mas já carrega o máximo (%d) e a deixa para trás.\n", MAX_POCOES);
    }
}

/* Uma "rodada" de exploracao: sorteia o que acontece.
 * Cada piso tem no maximo MAX_TESOUROS_PISO tesouros, entao nao da para farmar ouro infinito.
 * Como monstros aparecem em 55% das tentativas, o piso sempre pode ser concluido. */
void explorarDungeon(void) {
    int sorteio = rand() % 100;   /* numero de 0 a 99 */
    int temTesouro = (tesourosPiso < MAX_TESOUROS_PISO);

    printf("Você avança com cuidado pelo %s...\n\n", NOMES_PISOS[pisoAtual]);

    if (sorteio < 55) {
        encontrarMonstro();
    } else if (temTesouro && sorteio < 70) {
        encontrarOuro();
    } else if (temTesouro && sorteio < 80) {
        encontrarArma();
    } else if (temTesouro && sorteio < 90) {
        encontrarPocao();
    } else {
        printf("Corredores vazios, teias e poeira. Nada de interessante por aqui.\n");
    }
}

/* Chamada quando o piso esta concluido: vendedor + descida, ou Frik no piso 3 */
void avancarPiso(void) {
    if (!pisoLimpo()) {
        printf("Você ainda não concluiu este piso.\n");
        return;
    }
    if (pisoAtual == TOTAL_PISOS) {
        enfrentarFrik();
        return;
    }

    printf("Você encontra a saída do piso e, num canto iluminado por uma tocha, um vendedor ambulante.\n");
    curarCompleto();
    printf("Uma luz morna o envolve: sua vida foi totalmente restaurada (%d/%d).\n",
           jogador.vida, jogador.vidaMax);
    pausar();

    enfrentarVendedor(pisoAtual);

    pisoAtual++;
    monstrosDerrotados = 0;
    tesourosPiso = 0;
    printf("\nVocê desce as escadas rumo ao piso %d: %s.\n", pisoAtual, NOMES_PISOS[pisoAtual]);
}

void enfrentarFrik(void) {
    int resultado;

    if (pisoAtual != TOTAL_PISOS || !pisoLimpo()) {
        printf("O salão de Frik ainda está selado.\n");
        return;
    }
    limparTela();
    titulo("O SALÃO DE FRIK");
    printf("As portas de pedra se abrem sozinhas. No fundo do salão, sentado num trono de ossos,\n");
    printf("Frik ergue o olhar vazio: \"Mais um tolo veio me acordar...\"\n");
    pausar();

    inimigo = MODELO_FRIK;
    resultado = iniciarCombate(0);   /* 0 = sem fuga contra o chefe */

    if (resultado == COMBATE_MORREU) {
        gameOver();
    } else {
        telaVitoria();
    }
}

/* ============================================================
 * VENDEDOR
 * ============================================================ */

void comprarPocoes(void) {
    int maxPorOuro = jogador.ouro / PRECO_POCAO;
    int maxPorEspaco = MAX_POCOES - jogador.pocoes;
    int maximo = (maxPorOuro < maxPorEspaco) ? maxPorOuro : maxPorEspaco;
    int quantidade;

    if (maxPorEspaco <= 0) {
        printf("Você já carrega o máximo de poções (%d).\n", MAX_POCOES);
        return;
    }
    if (maxPorOuro <= 0) {
        printf("Você não tem ouro suficiente (cada poção custa %d).\n", PRECO_POCAO);
        return;
    }
    printf("Poção de cura: %d ouro cada. Você pode comprar até %d.\n", PRECO_POCAO, maximo);
    quantidade = lerInteiro("Quantas deseja comprar? (0 = cancelar): ", 0, maximo);
    if (quantidade == 0) {
        return;
    }
    jogador.ouro -= quantidade * PRECO_POCAO;
    jogador.pocoes += quantidade;
    printf("Você compra %d poção(ões). Poções: %d, ouro restante: %d.\n",
           quantidade, jogador.pocoes, jogador.ouro);
}

void comprarArmas(const int estoque[], int qtd) {
    int i, escolha, indice;

    printf("Armas à venda:\n");
    for (i = 0; i < qtd; i++) {
        indice = estoque[i];
        printf("  %d - %s (+%d ataque, %s) - %d ouro\n", i + 1,
               CATALOGO_ARMAS[indice].nome, CATALOGO_ARMAS[indice].bonusAtaque,
               CATALOGO_ARMAS[indice].categoria, CATALOGO_ARMAS[indice].preco);
    }
    printf("Seu ouro: %d\n", jogador.ouro);
    escolha = lerInteiro("Qual arma deseja comprar? (0 = cancelar): ", 0, qtd);
    if (escolha == 0) {
        return;
    }
    indice = estoque[escolha - 1];

    if (jogador.ouro < CATALOGO_ARMAS[indice].preco) {
        printf("Ouro insuficiente para %s.\n", CATALOGO_ARMAS[indice].nome);
        return;
    }
    if (!adicionarArmaInventario(indice)) {
        printf("Seu inventário está cheio. Venda uma arma antes de comprar outra.\n");
        return;
    }
    jogador.ouro -= CATALOGO_ARMAS[indice].preco;
    printf("Você comprou %s por %d ouro. Ela está no seu inventário (equipe-a pelo menu).\n",
           CATALOGO_ARMAS[indice].nome, CATALOGO_ARMAS[indice].preco);
}

void venderArmas(void) {
    int escolha, posicao;

    if (jogador.qtdArmas <= 1) {
        printf("Você só tem a arma equipada, e ela não pode ser vendida.\n");
        return;
    }
    listarInventario();
    escolha = lerInteiro("Qual arma deseja vender? (0 = cancelar): ", 0, jogador.qtdArmas);
    if (escolha == 0) {
        return;
    }
    posicao = escolha - 1;
    if (posicao == jogador.armaEquipada) {
        printf("Você não pode vender a arma que está equipada.\n");
        return;
    }
    jogador.ouro += jogador.inventario[posicao].valorVenda;
    printf("Você vendeu %s por %d ouro.\n", jogador.inventario[posicao].nome,
           jogador.inventario[posicao].valorVenda);
    removerArmaInventario(posicao);
}

/* Loja: o mesmo codigo serve para os pisos 1 e 2, so muda o estoque */
void enfrentarVendedor(int piso) {
    int estoque[3];
    int qtdEstoque, i, opcao, sair = 0;

    if (piso == 1) {
        qtdEstoque = 2;
        for (i = 0; i < qtdEstoque; i++) {
            estoque[i] = LOJA_PISO1[i];
        }
    } else {
        qtdEstoque = 3;
        for (i = 0; i < qtdEstoque; i++) {
            estoque[i] = LOJA_PISO2[i];
        }
    }

    while (!sair) {
        limparTela();
        titulo("O VENDEDOR");
        printf("\"Ouro por equipamento, forasteiro. Sem fiado.\"\n\n");
        printf("Seu ouro: %d   Poções: %d/%d\n\n", jogador.ouro, jogador.pocoes, MAX_POCOES);
        printf("1 - Comprar poções de cura\n");
        printf("2 - Comprar armas\n");
        printf("3 - Vender armas\n");
        printf("4 - Consultar ouro\n");
        printf("5 - Equipar arma\n");
        printf("6 - Sair da loja e continuar a aventura\n");
        opcao = lerInteiro("Escolha uma opção: ", 1, 6);
        printf("\n");

        switch (opcao) {
            case 1:
                comprarPocoes();
                break;
            case 2:
                comprarArmas(estoque, qtdEstoque);
                break;
            case 3:
                venderArmas();
                break;
            case 4:
                printf("Você tem %d de ouro.\n", jogador.ouro);
                break;
            case 5:
                equiparArma();
                break;
            case 6:
                sair = 1;
                break;
        }
        if (!sair) {
            pausar();
        }
    }
}

/* ============================================================
 * FIM DE JOGO
 * ============================================================ */

void gameOver(void) {
    estadoJogo = ESTADO_DERROTA;
    limparTela();
    titulo("VOCÊ MORREU");
    printf("Suas forças se esvaem e a escuridão das catacumbas o envolve.\n");
    printf("%s, o(a) %s %s, caiu no piso %d: %s.\n", jogador.nome, jogador.classe,
           jogador.raca, pisoAtual, NOMES_PISOS[pisoAtual]);
    printf("\nFIM DE JOGO - DERROTA\n");
}

void telaVitoria(void) {
    estadoJogo = ESTADO_VITORIA;
    jogador.ouro += inimigo.ouro;
    limparTela();
    titulo("VITÓRIA!");
    printf("Frik solta um último lamento e se desfaz em poeira e sombras.\n");
    printf("As catacumbas tremem... e então tudo fica em paz.\n\n");
    printf("Recompensa final: a Coroa de Ossos de Frik e %d de ouro!\n\n", inimigo.ouro);
    printf("%s, o(a) %s %s, concluiu a aventura!\n", jogador.nome, jogador.classe, jogador.raca);
    printf("Ouro final: %d   Vida restante: %d/%d\n", jogador.ouro, jogador.vida, jogador.vidaMax);
    printf("\nFIM DE JOGO - VITÓRIA\n");
}

/* ============================================================
 * MAIN: loop principal + switch (a mesma ideia do programa de conta bancaria)
 * ============================================================ */
int main(void) {
    int opcao, confirmaSair;

    configurarConsole();
#ifdef SEMENTE_FIXA
    srand(SEMENTE_FIXA);               /* para testes: sempre o mesmo "acaso" */
#else
    srand((unsigned int)time(NULL));   /* semente diferente a cada execucao   */
#endif

    limparTela();
    titulo("CATACUMBAS DE FRIK");
    printf("Dizem que Frik, um necromante esquecido, dorme no fundo das catacumbas.\n");
    printf("Três pisos de horror separam você dele. Prepare-se.\n");
    pausar();

    criarPersonagem();

    while (estadoJogo == ESTADO_JOGANDO) {
        exibirCabecalho();
        printf("O que deseja fazer?\n\n");
        if (!pisoLimpo()) {
            printf("1 - Explorar\n");
        } else if (pisoAtual < TOTAL_PISOS) {
            printf("1 - Seguir para o vendedor e descer ao próximo piso\n");
        } else {
            printf("1 - Entrar no salão de Frik\n");
        }
        printf("2 - Ver atributos\n");
        printf("3 - Ver inventário\n");
        printf("4 - Equipar arma\n");
        printf("5 - Consultar equipamento\n");
        printf("6 - Usar poção\n");
        printf("7 - Sair do jogo\n");
        opcao = lerInteiro("\nEscolha uma opção: ", 1, 7);
        printf("\n");

        switch (opcao) {
            case 1:
                if (pisoLimpo()) {
                    avancarPiso();
                } else {
                    explorarDungeon();
                }
                break;
            case 2:
                titulo("ATRIBUTOS");
                exibirStatus();
                break;
            case 3:
                verInventario();
                break;
            case 4:
                equiparArma();
                break;
            case 5:
                exibirEquipamento();
                break;
            case 6:
                usarPocao();
                break;
            case 7:
                printf("1 - Sim, sair\n2 - Não, continuar jogando\n");
                confirmaSair = lerInteiro("Tem certeza que deseja sair? ", 1, 2);
                if (confirmaSair == 1) {
                    estadoJogo = ESTADO_SAIU;
                }
                break;
        }

        if (estadoJogo == ESTADO_JOGANDO) {
            pausar();
        }
    }

    if (estadoJogo == ESTADO_SAIU) {
        printf("\nVocê abandona as catacumbas. Até a próxima aventura!\n");
    }
    pausar();   /* evita que a janela feche sozinha ao terminar */
    return 0;
}
