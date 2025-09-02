#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

typedef struct{
    int x,y;
    float dx,dy;
    float acumulo_y;
}object;

long long tempo_em_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

object player;
//char carro0[]= " ██==█==██ "; sonho jogado no
//char carro1[]= "░░ ▓███▓ ░░"; lixo, agora, eh
//char carro2[]= "░░ ▓███▓ ░░"; apenas memoria
char carro0[]="x=/\\=x";
char carro1[]="H||||H";
char carro2[]=" ---- ";
int largura_carro = 6,altura_carro=3;
int altura,largura,meio,quoficiente,ambiente=3,tempo_de_curva=500;
float curva_da_pista=1;
int n_carros=3;
object carro[3];

int get_random(int max){
    int x=rand() % max;
    return x;
}

void curvar_pista(){
    for(int i=10;i>=0;i--){
        //curva_da_pista=1+i/10;
        //usleep(100000);
    }
    for(int i=0;i<=10;i++){
        //curva_da_pista=1+(10-i)/10;
        //usleep(100000);
    }
}


void criar_inimigos() {
    for (int i = 0; i < n_carros; i++) {
        if (carro[i].y > 0 && carro[i].y < altura) continue;

        int pista_esq = meio - 15;
        int pista_dir = meio + 15 - largura_carro;

        do {
            carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
        } while (abs(carro[i].x - player.x) < largura_carro);

        carro[i].y = 0; 
        carro[i].dx = 0;
        carro[i].dy = 0.25;
    }
}



void print_carro(object obj, int is_player){
    if(!is_player) attron(COLOR_PAIR(1));
    //else attron(COLOR_PAIR(2));

    mvprintw(obj.y, obj.x,"%s",carro0);
    mvprintw(obj.y+1, obj.x,"%s",carro1);
    mvprintw(obj.y+2, obj.x,"%s",carro2);

    if(!is_player) attroff(COLOR_PAIR(1));
    //else attroff(COLOR_PAIR(2));
}

int quoficiente_esq(int j){
    float lado=0;
    quoficiente=((float)j*0.7/meio)*largura;
    lado = (0.8*meio)-quoficiente;
    return (int)lado;
}

int quoficiente_dir(int j){
    float lado=0;
    quoficiente=((float)j*0.7/meio)*largura;
    lado = (1.2*meio)+quoficiente;
    return (int)lado;
}

void atualizar_pos(object *obj){
    int velocidade = 2;
    if(obj->x + (obj->dx * velocidade) >= quoficiente_esq(obj->y) && obj->x + (obj->dx * velocidade) <= quoficiente_dir(obj->y) - largura_carro){
        obj->x += obj->dx * velocidade;
    }
    if(obj->y + (obj->dy * velocidade) >= 0 && obj->y + (obj->dy * velocidade) <= altura - altura_carro){
        obj->y += obj->dy * velocidade;
    }
}

void pista(){
    attron(COLOR_PAIR(ambiente));
    for(int j=0;j<=altura;j++){
        
        curva_da_pista;
        //quoficiente=((float)j*0.7/meio)*largura;
        for (int i=0;i<quoficiente_esq(j);i++){
            int x=curva_da_pista*i;
            move(j,x);
            addstr(" ");
        }
        for (int i=(int)quoficiente_dir(j);i<largura;i++){
            int x=curva_da_pista*i;
            move(j,(x+1));
            addstr(" ");
        }
    }
    attroff(COLOR_PAIR(ambiente));
    attron(COLOR_PAIR(2));
    for(int i=0;i<=altura;i++){
        quoficiente=((float)i*0.7/meio)*largura;
        //fazer amanha um for q print " " para usar color_pair e mudar o fundo
        mvprintw(i,(0.8*meio)-quoficiente,"%s","/");
        mvprintw(i,(1.2*meio)+quoficiente,"%s","\\");
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
    init_pair(1,COLOR_RED,COLOR_BLUE);
    init_pair(2,COLOR_YELLOW,COLOR_YELLOW);
    init_pair(3,COLOR_GREEN,COLOR_GREEN);
    getmaxyx(stdscr,altura,largura);
    meio=largura/2;
    player.x=(int)meio-(largura_carro/2);
    player.y=altura-altura_carro;
    long long intervalo = 800; // 1000ms = 1 segundo
    long long ultimo_tempo = tempo_em_ms();
    criar_inimigos();
    while(true){
        long long agora = tempo_em_ms();

        if (agora - ultimo_tempo >= intervalo) {//agendano funcao
            //curvar_pista();
            //criar_inimigos();                
            ultimo_tempo = agora;
        }
        erase();
        pista();
        print_carro(player,1);
        key=getch();

        if(key=='q'){
            break;
        }

        if(key==KEY_UP){
            player.dy-=1;
        }else if(key==KEY_DOWN){
            player.dy+=1;
        }
        else{
            player.dy=0;
        }

        if(key==KEY_LEFT){
            player.dx-=1;
        }else if(key==KEY_RIGHT){
            player.dx+=1;
        }
        else{
            player.dx=0;
        }

        atualizar_pos(&player);
        for (int i = 0; i < n_carros; i++) {
            carro[i].acumulo_y+=carro[i].dy;
            if(carro[i].acumulo_y>1 || carro[i].acumulo_y<0){
                carro[i].acumulo_y=0;
            }
            carro[i].y += carro[i].acumulo_y;
            if (carro[i].y > altura) {
                int pista_esq = meio - 10;
                int pista_dir = meio + 10 - largura_carro;
                carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
                carro[i].y = 0 * altura_carro;
                usleep(20000);
            }
            carro[i].dx=(get_random(7)-3)/3;
            
            //carro[i].x+=carro[i].dx;
            atualizar_pos(&carro[i]);
            print_carro(carro[i], 0);
        }
        refresh();

        usleep(16000);
        
    }
    endwin();
    return 0;
}
