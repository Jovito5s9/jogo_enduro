#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/time.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct{
    float x,y;
    float dx,dy;
    float acumulo_y;
    int largura,altura;
    int velocidade_y,velocidade_x;
    float modificador;
} object;

/* Sprites (texto) dos carros */
char carro0gg[]="x=/\\=x";
char carro1gg[]="H||||H";
char carro2gg[]=" ---- ";
char carro0pp[]=" =--=";
char carro1pp[]="H====H";

int largura_carroGG = 6, altura_carroGG = 3;
int largura_carroPP = 4, altura_carroPP = 2;

int altura, largura, meio;
int ambiente = 3;
int n_carros = 3;

object carro[10];
object player;
int pontuacao = 0;

/* ----- Curvas dinâmicas ----- */
float curva_amplitude = 1.0f;
float curva_wavelength = 50.0f;
float curva_velocidade = 0.003f;
float alvo_wavelength = 50.0f;
float alvo_velocidade = 0.003f;
float curva_transicao_vel = 0.02f;
float fase = 0.0f;

long long ultima_grande_curva = 0;
const long long intervalo_grande_curva = 20000;

/* tempo em ms */
long long tempo_em_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

/* random seguro */
int get_random(int max){
    if (max <= 1) return 0;
    return rand() % max;
}

void atualizar_curva(){
    curva_wavelength += (alvo_wavelength - curva_wavelength) * curva_transicao_vel;
    curva_velocidade += (alvo_velocidade - curva_velocidade) * curva_transicao_vel;
    fase += curva_velocidade;
}

void gerar_nova_curva(){
    long long agora = tempo_em_ms();

    if (agora - ultima_grande_curva > intervalo_grande_curva && get_random(100) < 5) {
        alvo_wavelength = 150 + get_random(60);
        alvo_velocidade = 0.003f + (get_random(5) / 3000.0f);
        curva_amplitude = 3.0f;
        ultima_grande_curva = agora;
        return;
    }

    int tipo = get_random(100);
    if (tipo < 45) {
        alvo_wavelength = 70 + get_random(50);
        alvo_velocidade = 0.002f + (get_random(5) / 4000.0f);
        curva_amplitude = 1.5f;
    } else if (tipo < 75) {
        alvo_wavelength = 40 + get_random(35);
        alvo_velocidade = 0.003f + (get_random(5) / 3000.0f);
        curva_amplitude = 2.0f;
    } else if (tipo < 90) {
        alvo_wavelength = 20 + get_random(20);
        alvo_velocidade = 0.005f + (get_random(5) / 2000.0f);
        curva_amplitude = 2.5f;
    } else {
        alvo_wavelength = 15 + get_random(10);
        alvo_velocidade = 0.007f + (get_random(5) / 1500.0f);
        curva_amplitude = 3.0f;
    }
}

int quoficiente_esq(int j){
    float offset = curva_amplitude * sinf((2.0f * M_PI * j) / curva_wavelength + fase);
    float q = ((float)j * 0.7f / meio) * largura;
    float lado = (0.8f * meio) - q + offset;
    return (int)lado;
}

int quoficiente_dir(int j){
    float offset = curva_amplitude * sinf((2.0f * M_PI * j) / curva_wavelength + fase);
    float q = ((float)j * 0.7f / meio) * largura;
    float lado = (1.2f * meio) + q + offset;
    return (int)lado;
}

void pista(){
    attron(COLOR_PAIR(ambiente));
    for (int j = 0; j <= altura; j++) {
        for (int i = 0; i < quoficiente_esq(j); i++) {
            move(j, i);
            addstr(" ");
        }
        for (int i = quoficiente_dir(j); i < largura; i++) {
            move(j, i+1);
            addstr(" ");
        }
    }
    attroff(COLOR_PAIR(ambiente));

    attron(COLOR_PAIR(2));
    for (int i = 0; i <= altura; i++) {
        mvprintw(i, quoficiente_esq(i), "/");
        mvprintw(i, quoficiente_dir(i), "\\");
    }
    attroff(COLOR_PAIR(2));
}

void mudar_modificador(object *obj){
    obj->modificador = (float)(get_random(3) - 1);
}

void criar_inimigos(){
    for (int i = 0; i < n_carros; i++) {
        int pista_esq = meio - 10;
        int pista_dir = meio + 10 - largura_carroGG;
        int span = pista_dir - pista_esq;
        if (span <= 0) span = 1;

        int tries = 0;
        do {
            carro[i].x = pista_esq + get_random(span + 1);
            tries++;
            if (tries > 20) break;
        } while (abs((int)carro[i].x - (int)player.x) < largura_carroGG);

        mudar_modificador(&carro[i]);
        carro[i].y = - (float)get_random(20) * altura_carroPP;
        carro[i].dx = 0.0f;
        carro[i].dy = 0.25f;
        carro[i].acumulo_y = 0.0f;
        carro[i].largura = largura_carroGG;
        carro[i].altura = altura_carroGG;
        carro[i].velocidade_x = 1;
        carro[i].velocidade_y = 1;
    }
}

void print_carro(object obj, int is_player){
    int ix = (int)obj.x;
    int iy = (int)obj.y;

    if (iy + obj.altura < 0 || iy >= altura) return;
    if (ix + obj.largura < 0 || ix >= largura) return;

    if (!is_player) attron(COLOR_PAIR(1));

    if (obj.largura == largura_carroGG && obj.altura == altura_carroGG) {
        if (iy >= 0 && iy < altura) mvprintw(iy, ix, "%s", carro0gg);
        if (iy+1 >= 0 && iy+1 < altura) mvprintw(iy+1, ix, "%s", carro1gg);
        if (iy+2 >= 0 && iy+2 < altura) mvprintw(iy+2, ix, "%s", carro2gg);
    } else {
        if (iy >= 0 && iy < altura) mvprintw(iy, ix+1, "%s", carro0pp);
        if (iy+1 >= 0 && iy+1 < altura) mvprintw(iy+1, ix+1, "%s", carro1pp);
    }

    if (!is_player) attroff(COLOR_PAIR(1));
}

void atualizar_pos(object *obj, int is_player){
    int linha = (int)obj->y;
    int left = quoficiente_esq(linha) + 1;
    int right = quoficiente_dir(linha) - largura_carroGG;

    float nx = obj->x + (obj->dx * obj->velocidade_x * 2);
    if (nx >= left && nx <= right) obj->x = nx;

    float ny = obj->y + (obj->dy * obj->velocidade_y);
    if (is_player) {
        if (ny >= 0 && ny <= (altura - altura_carroGG)) obj->y = ny;
    }
}

void gerenciar_carro(object *obj, int is_player){
    if (obj->y >= altura * 0.3f) {
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

int colidiu(object a, object b){
    return (a.x < b.x + b.largura &&
            a.x + a.largura > b.x &&
            a.y < b.y + b.altura &&
            a.y + a.altura > b.y);
}

int main(){
    srand((unsigned)time(NULL));

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(3, COLOR_GREEN, COLOR_GREEN);

    getmaxyx(stdscr, altura, largura);
    meio = largura / 2;

    player.x = (float)meio - (largura_carroGG / 2);
    player.y = (float)altura - altura_carroGG;
    player.dx = 0.0f;
    player.dy = 0.0f;
    player.largura = largura_carroGG;
    player.altura = altura_carroGG;
    player.velocidade_x = 2;
    player.velocidade_y = 1;
    player.modificador = 0.0f;

    criar_inimigos();
    long long ultima_mudanca = tempo_em_ms();

    int key;
    while (1){
        long long agora = tempo_em_ms();
        if (agora - ultima_mudanca > 5000){
            gerar_nova_curva();
            ultima_mudanca = agora;
        }
        atualizar_curva();

        erase();
        pista();

        key = getch();
        if (key == 'q' || key == 'Q') break;

        if (key == KEY_UP) player.dy = -1.0f;
        else if (key == KEY_DOWN) player.dy = 1.0f;
        else player.dy = 0.0f;

        if (key == KEY_LEFT) player.dx = -1.0f;
        else if (key == KEY_RIGHT) player.dx = 1.0f;
        else player.dx = 0.0f;

        gerenciar_carro(&player, 1);

        for (int i = 0; i < n_carros; i++){
            carro[i].acumulo_y += carro[i].dy;
            if (carro[i].acumulo_y > 1.0f || carro[i].acumulo_y < 0.0f)
                carro[i].acumulo_y = 0.0f;

            carro[i].y += carro[i].acumulo_y;

            if (carro[i].y > altura){
                int pista_esq = meio - 6;
                int pista_dir = meio + 6 - largura_carroGG;
                int span = pista_dir - pista_esq;
                if (span <= 0) span = 1;
                carro[i].x = pista_esq + get_random(span + 1);
                carro[i].y = - (float)get_random(30) * altura_carroPP;
                mudar_modificador(&carro[i]);
                usleep(20000);
                pontuacao++;
            }

            int movimento = get_random(5);
            if (movimento > 2) carro[i].dx = ((float)(get_random(11) - 5) / 5.0f);
            else carro[i].dx = ((float)(get_random(11) - 5) / 5.0f) + carro[i].modificador;

            gerenciar_carro(&carro[i], 0);

            if (colidiu(player, carro[i])){
                player.y += 3.0f;
                if (player.y > altura - altura_carroGG) player.y = altura - altura_carroGG;
                carro[i].y += 5.0f;
                carro[i].dx = -carro[i].dx;
            }
        }

        mvprintw(0, 2, "Pontos: %d", pontuacao);
        refresh();
        usleep(16000);
    }

    endwin();
    return 0;
}
