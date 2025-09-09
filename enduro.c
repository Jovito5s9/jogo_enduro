#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
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
}object;

long long tempo_em_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

object player;
char carro0gg[]="x=/\\=x";
char carro1gg[]="H||||H";
char carro2gg[]=" ---- ";
char carro0pp[]="=--=";
char carro1pp[]="H==H";
int largura_carroGG = 6,altura_carroGG=3;
int largura_carroPP = 4,altura_carroPP=2;
int altura,largura,meio,quoficiente,ambiente=3,tempo_de_curva=500;
float curva_da_pista=1;
int n_carros=3;
object carro[3];

// --- parametros da curva ---
float *curva = NULL; // offset horizontal para cada linha
float curva_amplitude = 12.0f;
float curva_wavelength = 100.0f;
float curva_speed = 0.003f;

int get_random(int max){
    int x=rand() % max;
    return x;
}

void mudar_modificador(object *obj){
    obj->modificador=get_random(3)-1;
}

void criar_inimigos() {
    for (int i = 0; i < n_carros; i++) {
        if (carro[i].y > 0 && carro[i].y < altura) continue;

        int pista_esq = meio - 10;
        int pista_dir = meio + 10 - largura_carroGG;

        do {
            carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
        } while (abs(carro[i].x - player.x) < largura_carroGG);
        mudar_modificador(&carro[i]);
        carro[i].y = -get_random(20)*altura_carroPP; 
        carro[i].dx = 0;
        carro[i].dy = 0.25;
    }
}

void calcular_curva(long long agora_ms){
    float phase = agora_ms * curva_speed;
    for(int j = 0; j <= altura; j++){
        float perspective = 1.0f - ((float)j / (float)altura) * 0.6f;
        float target = curva_amplitude * perspective * sinf((2.0f * M_PI * j) / curva_wavelength + phase);
        curva[j] = curva[j] * 0.85f + target * 0.15f;
    }
}

int quoficiente_esq(int j){
    float quof_base=((float)j*0.7f/meio)*largura;
    float lado = (0.8f*meio)-quof_base + curva[j];
    if(lado<0) lado=0;
    return (int)lado;
}

int quoficiente_dir(int j){
    float quof_base=((float)j*0.7f/meio)*largura;
    float lado = (1.2f*meio)+quof_base + curva[j];
    if(lado>largura) lado=largura;
    return (int)lado;
}

void print_carro(object obj, int is_player){
    int ix=(int)roundf(obj.x);
    int iy=(int)roundf(obj.y);
    if(!is_player) attron(COLOR_PAIR(1));
    if(obj.largura==largura_carroGG && obj.altura==altura_carroGG){
        if(iy>=0 && iy<altura && ix>=0 && ix+largura_carroGG<largura){
            mvprintw(iy, ix,"%s",carro0gg);
            mvprintw(iy+1, ix,"%s",carro1gg);
            mvprintw(iy+2, ix,"%s",carro2gg);
        }
    }else{
        if(iy>=0 && iy<altura && ix>=0 && ix+largura_carroPP<largura){
            mvprintw(iy, ix+1,"%s",carro0pp);
            mvprintw(iy+1, ix+1,"%s",carro1pp);
        }
    }
    if(!is_player) attroff(COLOR_PAIR(1));
}

void atualizar_pos(object *obj,int is_player){
    if(obj->x + (obj->dx * obj->velocidade_x) >= quoficiente_esq(obj->y)+1 && obj->x + (obj->dx * obj->velocidade_x) <= quoficiente_dir(obj->y) - largura_carroGG){
        obj->x += obj->dx * obj->velocidade_x;
    }
    if(obj->y + (obj->dy * obj->velocidade_y) >= 0 && obj->y + (obj->dy * obj->velocidade_y) <= altura - altura_carroGG){
        if(is_player){
            obj->y += obj->dy * obj->velocidade_y;
        }
    }
}

void gerenciar_carro(object *obj,int is_player){
    if(obj->y >= altura*0.3){
        obj->largura=largura_carroGG;
        obj->altura=altura_carroGG;
        obj->velocidade_y=1;
        obj->velocidade_x=1;
    }else{
        obj->largura=largura_carroPP;
        obj->altura=altura_carroPP;
        obj->velocidade_y=3;
        obj->velocidade_x=3;
    }
    atualizar_pos(obj,is_player);
    print_carro(*obj,is_player);
}

void pista(){
    attron(COLOR_PAIR(ambiente));
    for(int j=0;j<=altura;j++){
        int left=quoficiente_esq(j);
        int right=quoficiente_dir(j);

        for(int x=0;x<left && x<largura;x++){
            mvaddch(j,x,' ');
        }
        for(int x=right+1;x<largura;x++){
            mvaddch(j,x,' ');
        }
    }
    attroff(COLOR_PAIR(ambiente));

    attron(COLOR_PAIR(2));
    for(int i=0;i<=altura;i++){
        int le=quoficiente_esq(i);
        int ri=quoficiente_dir(i);
        if(le>=0 && le<largura) mvaddch(i,le,'/');
        if(ri>=0 && ri<largura) mvaddch(i,ri,'\\');
    }
    attroff(COLOR_PAIR(2));
}

int main(){
    srand(time(NULL));
    int key;
    initscr();
    nodelay(stdscr,true);
    keypad(stdscr,true);
    curs_set(0);
    noecho();
    start_color();
    use_default_colors();
    init_pair(1,COLOR_RED,-1);
    init_pair(2,COLOR_YELLOW,COLOR_YELLOW);
    init_pair(3,COLOR_GREEN,COLOR_GREEN);
    getmaxyx(stdscr,altura,largura);
    meio=largura/2;

    curva=(float*)malloc(sizeof(float)*(altura+1));
    for(int i=0;i<=altura;i++) curva[i]=0.0f;

    player.x=(int)meio-(largura_carroGG/2);
    player.y=altura-altura_carroGG;
    player.velocidade_x=2;

    long long intervalo = 800;
    long long ultimo_tempo = tempo_em_ms();

    criar_inimigos();

    while(true){
        long long agora=tempo_em_ms();
        if(agora-ultimo_tempo>=intervalo){
            ultimo_tempo=agora;
        }

        calcular_curva(agora);
        erase();
        pista();

        key=getch();
        if(key=='q') break;

        if(key==KEY_UP) player.dy=-1;
        else if(key==KEY_DOWN) player.dy=1;
        else player.dy=0;

        if(key==KEY_LEFT) player.dx=-1;
        else if(key==KEY_RIGHT) player.dx=1;
        else player.dx=0;

        gerenciar_carro(&player,1);
        for(int i=0;i<n_carros;i++){
            carro[i].acumulo_y+=carro[i].dy;
            if(carro[i].acumulo_y>1 || carro[i].acumulo_y<0){
                carro[i].acumulo_y=0;
            }
            carro[i].y += carro[i].acumulo_y;
            if(carro[i].y>altura){
                int pista_esq=meio-6;
                int pista_dir=meio+6-largura_carroGG;
                carro[i].x=pista_esq+get_random(pista_dir-pista_esq)+curva[0];
                carro[i].y=-get_random(30)*altura_carroPP;
                mudar_modificador(&carro[i]);
                usleep(20000);
            }
            int movimento=get_random(5);
            if(movimento>2){
                carro[i].dx=((get_random(11)-5)/5);
            }else{
                carro[i].dx=((get_random(11)-5)/5)+carro[i].modificador;
            }
            gerenciar_carro(&carro[i],0);
        }

        refresh();
        usleep(16000);
    }
    free(curva);
    endwin();
    return 0;
}
