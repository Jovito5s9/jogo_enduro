#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// Estruturas

typedef struct {
    float x, y;
    float dx, dy;
    float acumulo_y;
    int largura, altura;
    int velocidade_y, velocidade_x;
    float modificador;
} object;

// Variáveis globais

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

object player;
object carro[3];

char carro0gg[] = "x=/\\=x";
char carro1gg[] = "H||||H";
char carro2gg[] = " ---- ";
char carro0pp[] = "=--=";
char carro1pp[] = "H==H";

int largura_carroGG = 6, altura_carroGG = 3;
int largura_carroPP = 4, altura_carroPP = 2;

int altura, largura, meio;
int ambiente = 3;
int n_carros = 3;

float curva_amplitude = 1.0f;          // Quanto a curva mexe
float curva_wavelength = 50.0f;        // Comprimento atual da curva
float curva_velocidade = 0.003f;       // Velocidade atual
float alvo_wavelength = 50.0f;        // Comprimento desejado
float alvo_velocidade = 0.003f;       // Velocidade desejada
float curva_transicao_vel = 0.02f;     // Velocidade de transição
int offset_linha = 0;


float fase = 0.0f;

// ------------------------------------
// Funções utilitárias
// ------------------------------------
long long tempo_em_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

int get_random(int max) {
    return rand() % max;
}

// ------------------------------------
// Curvas dinâmicas
// ------------------------------------
void atualizar_curva() {
    // Transição suave entre curvas
    curva_wavelength += (alvo_wavelength - curva_wavelength) * curva_transicao_vel;
    curva_velocidade += (alvo_velocidade - curva_velocidade) * curva_transicao_vel;
    fase += curva_velocidade;
}

long long ultima_grande_curva = 0;    // tempo da última curva longa
const long long intervalo_grande_curva = 20000; // mínimo 20 segundos

void gerar_grande_curva() {
    alvo_wavelength = 150 + get_random(60); // curva bem longa
    alvo_velocidade = 0.002f + (get_random(3) / 4000.0f);
}



void gerar_nova_curva() {
    long long agora = tempo_em_ms();

    // 5% de chance de gerar uma grande curva, se passou tempo suficiente
    if (agora - ultima_grande_curva > intervalo_grande_curva && get_random(100) < 5) {
        // Grande curva ainda mais intensa
        alvo_wavelength = 150 + get_random(60);
        alvo_velocidade = 0.003f + (get_random(5) / 3000.0f); // mais rápida
        curva_amplitude = 3.0f; // deslocamento maior
        ultima_grande_curva = agora;
        return;
    }

    // Caso não seja grande curva, gera curvas normais/rápidas mais intensas
    int tipo = get_random(100);

    if (tipo < 30) {
        // 45% retas longas
        alvo_wavelength = 70 + get_random(50);       // um pouco mais curtas
        alvo_velocidade = 0.002f + (get_random(5) / 4000.0f);
        curva_amplitude = 1.5f;                      // mais movimento
    } 
    else if (tipo < 60) {
        // 30% curvas leves
        alvo_wavelength = 40 + get_random(35);       // mais curtas = mais intensas
        alvo_velocidade = 0.003f + (get_random(5) / 3000.0f);
        curva_amplitude = 2.0f;
    } 
    else if (tipo < 90) {
        // 15% curvas fechadas
        alvo_wavelength = 20 + get_random(20);       
        alvo_velocidade = 0.005f + (get_random(5) / 2000.0f);
        curva_amplitude = 2.5f;
    } 
    else {
        // 10% curvas rápidas e fechadas (BEM desafiantes)
        alvo_wavelength = 15 + get_random(10);      
        alvo_velocidade = 0.007f + (get_random(5) / 1500.0f);
        curva_amplitude = 3.0f;                      // intensidade máxima
    }
}




int quoficiente_esq(int j) {
    float offset = curva_amplitude * sinf((2.0f * M_PI * j) / curva_wavelength + fase);
    float quoficiente = ((float)j * 0.7 / meio) * largura;
    float lado = (0.8 * meio) - quoficiente + offset;
    return (int)lado;
}

int quoficiente_dir(int j) {
    float offset = curva_amplitude * sinf((2.0f * M_PI * j) / curva_wavelength + fase);
    float quoficiente = ((float)j * 0.7 / meio) * largura;
    float lado = (1.2 * meio) + quoficiente + offset;
    return (int)lado;
}

void pista() {
    attron(COLOR_PAIR(ambiente));
    for (int j = 0; j <= altura; j++) {
        for (int i = 0; i < quoficiente_esq(j); i++) {
            move(j, i);
            addstr(" ");
        }
        for (int i = (int)quoficiente_dir(j); i < largura; i++) {
            move(j, (i + 1));
            addstr(" ");
        }
    }
    attroff(COLOR_PAIR(ambiente));
    attron(COLOR_PAIR(2));
    for (int i = 0; i <= altura; i++) {
        mvprintw(i, quoficiente_esq(i), "%s", " ");
        mvprintw(i, quoficiente_dir(i), "%s", " ");
    }
    attroff(COLOR_PAIR(2));
}

void desenhar_linha_centro() {
    attron(COLOR_PAIR(2)); // amarelo
    for (int j = 0; j < altura; j++) {
        int centro = (quoficiente_esq(j) + quoficiente_dir(j)) / 2;
        if ((j + offset_linha) % 6 < 3) { // tracejada, opcional
            mvprintw(j, centro, "|");
        }
    }
    attroff(COLOR_PAIR(2));
}


void mudar_modificador(object *obj) {
    obj->modificador = get_random(3) - 1;
}

void criar_inimigos() {
    for (int i = 0; i < n_carros; i++) {
        if (carro[i].y > 0 && carro[i].y < altura)
            continue;

        int pista_esq = meio - 10;
        int pista_dir = meio + 10 - largura_carroGG;

        do {
            carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
        } while (abs(carro[i].x - player.x) < largura_carroGG);

        mudar_modificador(&carro[i]);
        carro[i].y = -get_random(20) * altura_carroPP;
        carro[i].dx = 0;
        carro[i].dy = 0.25;
    }
}

void print_carro(object obj, int is_player) {
    if (!is_player) attron(COLOR_PAIR(1));
    if (obj.largura == largura_carroGG && obj.altura == altura_carroGG) {
        mvprintw(obj.y, obj.x, "%s", carro0gg);
        mvprintw(obj.y + 1, obj.x, "%s", carro1gg);
        mvprintw(obj.y + 2, obj.x, "%s", carro2gg);
    } else {
        mvprintw(obj.y, obj.x + 1, "%s", carro0pp);
        mvprintw(obj.y + 1, obj.x + 1, "%s", carro1pp);
    }
    if (!is_player) attroff(COLOR_PAIR(1));
}

void atualizar_pos(object *obj, int is_player) {
    if (obj->x + (obj->dx * obj->velocidade_x) >= quoficiente_esq(obj->y) + 1 &&
        obj->x + (obj->dx * obj->velocidade_x) <= quoficiente_dir(obj->y) - largura_carroGG) {
            if (is_player) {
                obj->x += obj->dx * obj->velocidade_x * 2;
            }else{
            obj->x += obj->dx * obj->velocidade_x;
            }
    }
    if (obj->y + (obj->dy * obj->velocidade_y) >= 0 &&
        obj->y + (obj->dy * obj->velocidade_y) <= altura - altura_carroGG) {
        if (is_player) {
            obj->y += obj->dy * obj->velocidade_y;
        }
    }
}

void gerenciar_carro(object *obj, int is_player) {
    if (obj->y >= altura * 0.3) {
        obj->largura = largura_carroGG;
        obj->altura = altura_carroGG;
        obj->velocidade_y = 1;
        obj->velocidade_x = 1;
    } else {
        obj->largura = largura_carroPP;
        obj->altura = altura_carroPP;
        obj->velocidade_y = 3;
        obj->velocidade_x = 3;
    }
    atualizar_pos(obj, is_player);
    print_carro(*obj, is_player);
}


void jogo() {
    srand(time(NULL));
    int key;
    nodelay(stdscr, true);
    keypad(stdscr, true);
    curs_set(0);
    noecho();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(3, COLOR_GREEN, COLOR_GREEN);

    getmaxyx(stdscr, altura, largura);
    meio = largura / 2;
    int contador_de_linha=0;

    player.x = (int)meio - (largura_carroGG / 2);
    player.y = altura - altura_carroGG;
    player.velocidade_x = 2;

    criar_inimigos();
    long long ultima_mudanca = tempo_em_ms();

    while (true) {
        long long agora = tempo_em_ms();

        // Gera nova curva a cada 8 segundos
        if (agora - ultima_mudanca > 5000) {
            gerar_nova_curva();
            ultima_mudanca = agora;
        }

        atualizar_curva();

        erase();
        pista();
        desenhar_linha_centro();

        key = getch();
        if (key == 's' || key == 'S') break;

        if (key == KEY_UP) player.dy -= 1;
        else if (key == KEY_DOWN) player.dy += 1;
        else player.dy = 0;

        if (key == KEY_LEFT) player.dx -= 1;
        else if (key == KEY_RIGHT) player.dx += 1;
        else player.dx = 0;

        gerenciar_carro(&player, 1);

        for (int i = 0; i < n_carros; i++) {
            carro[i].acumulo_y += carro[i].dy;
            if (carro[i].acumulo_y > 1 || carro[i].acumulo_y < 0) {
                carro[i].acumulo_y = 0;
            }
            carro[i].y += carro[i].acumulo_y;

            if (carro[i].y > altura) {
                int pista_esq = meio - 6;
                int pista_dir = meio + 6 - largura_carroGG;
                carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
                carro[i].y = -get_random(30) * altura_carroPP;
                mudar_modificador(&carro[i]);
                usleep(20000);
            }


            int movimento = get_random(10);
            if (movimento < 5) {
                carro[i].dx = ((get_random(11) - 5) / 5);
            } else if(movimento < 7){
                carro[i].dx = ((get_random(11) - 5) / 5) + carro[i].modificador;
            }
            gerenciar_carro(&carro[i], 0);
        }

 
        contador_de_linha++;
        if (contador_de_linha >= 4) { // quanto maior, mais lenta a linha
            offset_linha--;
            if (offset_linha <= 0) offset_linha = altura;
            contador_de_linha = 0;
        }


        refresh();
        usleep(16000);
    }

    endwin();
}

int menu() {
    erase();
    getmaxyx(stdscr, altura, largura);
    meio = largura / 2;
    char enduro1[]="####  #  #  #=_   #  #  ##*.  +##+";
    char enduro2[]="#     #+ #  #  #  #  #  #  #  #  #";
    char enduro3[]="###   ## #  #  #  #  #  ###   #  #";
    char enduro4[]="#     # ##  #  #  #  #  #  #  #  #";
    char enduro5[]="####  # +#  #=*   *##*  #  #  *##*";
    int ch;
    while (true) {
        erase();
        attron(COLOR_PAIR(5));
        mvprintw((int)((0.1 * altura)+1), meio-17, "%s",enduro1);
        mvprintw((int)((0.1 * altura)+2), meio-17, "%s",enduro2);
        mvprintw((int)((0.1 * altura)+3), meio-17, "%s",enduro3);
        mvprintw((int)((0.1 * altura)+4), meio-17, "%s",enduro4);
        mvprintw((int)((0.1 * altura)+5), meio-17, "%s",enduro5);
        attroff(COLOR_PAIR(5));

        attron(COLOR_PAIR(4));
        mvprintw((int)(0.55 * altura), meio - 6, "Jogar [ENTER]");
        attroff(COLOR_PAIR(4));

        attron(COLOR_PAIR(1));
        mvprintw((int)(0.65 * altura), meio - 4, "Sair [S]");
        attroff(COLOR_PAIR(1));

        mvprintw(0.75*altura,meio-6,"Créditos[C]");
        
        refresh();

        ch = getch();
        if (ch == 's' || ch == 'S') {
            return 0;
        }
        if (ch == '\n') { 
            return 1; 
        }
        if (ch == 'c' || ch == 'C'){
            return 2;
        }
        usleep(16000);
    }
}

void creditos(){
    erase();
    getmaxyx(stdscr, altura, largura);
    meio = largura / 2;
    char ch;
    while (true){
        erase();
        mvprintw(0.3*altura,meio-4,"Créditos");
        mvprintw(0.5*altura,meio-18,"Programador e designer geral: Jovito.");
        mvprintw(0.6*altura,meio-17,"Programador das fisicas: Rodrigo.");
        mvprintw(0.8*altura,meio-17,"Colaboradores: Ricardo e Matheus.");
        ch = getch();
        if (ch == 's' || ch == 'S'){
            return;
        }
        refresh();
    }
    
}

void gerenciar_telas(){
    srand(time(NULL));
    nodelay(stdscr, true);
    keypad(stdscr, true);
    curs_set(0);
    noecho();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(3, COLOR_GREEN, COLOR_GREEN);
    init_pair(4, COLOR_GREEN, -1);
    init_pair(5, COLOR_YELLOW, -1);

    while (true) {
        int opcao = menu();
        if (opcao == 0) break; 
        if (opcao == 1) jogo();
        if (opcao == 2) creditos();
    }
    endwin();
}


int main(){
    initscr();
    gerenciar_telas(); 
    return 0;
}

