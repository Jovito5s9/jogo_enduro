#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>


typedef struct {
    float x, y;
    float dx, dy;
    float acumulo_y;
    int largura, altura;
    int velocidade_y, velocidade_x;
    float modificador;
    int pontuado;
    int cor;
} object;


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

object player;
object carro[3];

int carros_passados=0, metros_percorridos=0;

char carro0gg[]="##=#####=##";
char carro1gg[]="    ###    ";
char carro2gg[]="##_#####_##";
char carro3gg[]="##=#####=##";
char carro4gg[]="## ===== ##";

char carro0g[] = "X=/\\=X";
char carro1g[] = "#||||#";
char carro2g[] = " ---- ";

char carro0pp[] = "=--=";
char carro1pp[] = "H==H";

int cores_carros[4]={1,4,5,9};

float dia =1.0,clima=1.0;

int largura_carroGG = 11, altura_carroGG = 5;
int largura_carroG = 6, altura_carroG = 3;
int largura_carroPP = 4, altura_carroPP = 2;

int altura, largura, meio;
int altura_pista_minima = 0, altura_pista_max = 0;
int ambiente=0;
int climas[]={3,11, 6};
int n_carros = 3;

float curva_amplitude = 1.0f;     
float curva_wavelength = 50.0f;      
float curva_velocidade = 0.003f;     
float alvo_wavelength = 50.0f;       
float alvo_velocidade = 0.003f;     
float curva_transicao_vel = 0.02f;     
int offset_linha = 0;


float fase = 0.0f;

long long tempo_em_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

int get_random(int max) {
    return rand() % max;
}

void atualizar_curva() {
    curva_wavelength += (alvo_wavelength - curva_wavelength) * curva_transicao_vel;
    curva_velocidade += (alvo_velocidade - curva_velocidade) * curva_transicao_vel;
    fase += curva_velocidade;
}

long long ultima_grande_curva = 0;    
const long long intervalo_grande_curva = 20000; 

void gerar_grande_curva() {
    alvo_wavelength = 150 + get_random(60); 
    alvo_velocidade = 0.002f + (get_random(3) / 4000.0f);
}



void gerar_nova_curva() {
    long long agora = tempo_em_ms();

    if (agora - ultima_grande_curva > intervalo_grande_curva && get_random(100) < 5) {
        alvo_wavelength = 150 + get_random(60);
        alvo_velocidade = 0.003f + (get_random(5) / 3000.0f); 
        curva_amplitude = 3.0f; 
        ultima_grande_curva = agora;
        return;
    }

    int tipo = get_random(100);

    if (tipo < 30) {
        alvo_wavelength = 70 + get_random(50);       
        alvo_velocidade = 0.002f + (get_random(5) / 4000.0f);
        curva_amplitude = 1.5f;                    
    } 
    else if (tipo < 60) {
        alvo_wavelength = 40 + get_random(35);      
        alvo_velocidade = 0.003f + (get_random(5) / 3000.0f);
        curva_amplitude = 2.0f;
    } 
    else if (tipo < 90) {
        alvo_wavelength = 20 + get_random(20);       
        alvo_velocidade = 0.005f + (get_random(5) / 2000.0f);
        curva_amplitude = 2.5f;
    } 
    else {
        alvo_wavelength = 15 + get_random(10);      
        alvo_velocidade = 0.007f + (get_random(5) / 1500.0f);
        curva_amplitude = 3.0f;                  
    }
}



int quoficiente_esq(int j) {
    int jl = j;
    if (altura_pista_max > altura_pista_minima) {
        if (jl < altura_pista_minima) jl = altura_pista_minima;
        if (jl > altura_pista_max) jl = altura_pista_max;
    }
    float local_j = (float)(jl - altura_pista_minima);
    float track_h = (float)(altura_pista_max - altura_pista_minima);
    float t = (track_h > 0.0f) ? (local_j / track_h) : 0.0f;
    float offset_factor = 1.0f - 0.5f * t; 
    float offset = curva_amplitude * offset_factor * sinf((2.0f * M_PI * local_j) / curva_wavelength + fase);
    float half_road = (0.25f + 0.75f * t) * meio;
    int left = (int)round((float)meio - half_road + offset);
    if (left < 0) left = 0;
    if (left >= largura) left = largura - 1;
    return left;
}

int quoficiente_dir(int j) {
    int jl = j;
    if (altura_pista_max > altura_pista_minima) {
        if (jl < altura_pista_minima) jl = altura_pista_minima;
        if (jl > altura_pista_max) jl = altura_pista_max;
    }
    float local_j = (float)(jl - altura_pista_minima);
    float track_h = (float)(altura_pista_max - altura_pista_minima);
    float t = (track_h > 0.0f) ? (local_j / track_h) : 0.0f;
    float offset_factor = 1.0f - 0.5f * t;
    float offset = curva_amplitude * offset_factor * sinf((2.0f * M_PI * local_j) / curva_wavelength + fase);
    float half_road = (0.25f + 0.75f * t) * meio;
    int right = (int)round((float)meio + half_road + offset);
    if (right < 0) right = 0;
    if (right >= largura) right = largura - 1;
    return right;
}

void pista() {
    if(dia>=0){
        attron(COLOR_PAIR(climas[ambiente]));
    }else{
        attron(COLOR_PAIR(10));
    }
    for (int j = altura_pista_minima; j <= altura_pista_max; j++) {
        for (int i = 0; i < quoficiente_esq(j); i++) {
            move(j, i);
            addstr(" ");
        }
        for (int i = (int)quoficiente_dir(j); i < largura; i++) {
            move(j, (i + 1));
            addstr(" ");
        }
    }
    if(dia>=0){
        attroff(COLOR_PAIR(climas[ambiente]));
    }else{
        attroff(COLOR_PAIR(10));
    }
    attron(COLOR_PAIR(2));
    for (int i = altura_pista_minima; i <= altura_pista_max; i++) {
        mvprintw(i, quoficiente_esq(i), "%s", " ");
        mvprintw(i, quoficiente_dir(i), "%s", " ");
    }
    attroff(COLOR_PAIR(2));
}

void desenhar_linha_centro() {
    attron(COLOR_PAIR(2)); 
    for (int j = altura_pista_minima; j <= altura_pista_max; j++) {
        int centro = (quoficiente_esq(j) + quoficiente_dir(j)) / 2;
        if ((j + offset_linha) % 6 < 3) {
            mvprintw(j, centro, "|");
        }
    }
    attroff(COLOR_PAIR(2));
}


void mudar_modificador(object *obj) {
    obj->modificador = get_random(3) - 1;
}

int mudar_cor_carro(){
    int x = get_random(4);
    return cores_carros[x];
}

int mudar_clima(){
    ambiente +=1;
    if (ambiente==3){
        ambiente = 0;
    }
    return ambiente;
}

void criar_inimigos() {
    for (int i = 0; i < n_carros; i++) {
        if (carro[i].y > altura_pista_minima && carro[i].y < altura_pista_max)
            continue;

        int pista_esq = meio - 10;
        int pista_dir = meio + 10 - largura_carroG;

        do {
            carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
        } while (abs(carro[i].x - player.x) < largura_carroG);
        carro[i].cor=mudar_cor_carro();
        mudar_modificador(&carro[i]);
        carro[i].y = altura_pista_minima - get_random(20) * altura_carroPP;
        carro[i].dx = 0;
        carro[i].dy = 0.25;
        carro[i].pontuado = 0;
    }
}

void print_carro(object obj, int is_player) {
    if (obj.y>=altura_pista_minima){
        if (is_player || dia>0) {
            attron(COLOR_PAIR(obj.cor));
        }else if (!is_player && dia<0){
            attron(COLOR_PAIR(1));
        }
        if (obj.largura == largura_carroG && obj.altura == altura_carroG) {
            if(dia>=0 || is_player){
                mvprintw((int)obj.y, (int)obj.x, "%s", carro0g);
                mvprintw((int)(obj.y + 1), (int)obj.x, "%s", carro1g);
                mvprintw((int)(obj.y + 2), (int)obj.x, "%s", carro2g);
            }else{
                mvprintw(obj.y+2, obj.x, " =  = ");
            }
        }else if (obj.largura==largura_carroGG && obj.altura==altura_carroGG){
            if(dia>=0 || is_player){
            mvprintw((int)obj.y, (int)obj.x, "%s", carro0gg);
            mvprintw((int)(obj.y + 1), (int)obj.x, "%s", carro1gg);
            mvprintw((int)(obj.y + 2), (int)obj.x, "%s", carro2gg);
            mvprintw((int)(obj.y + 2), (int)obj.x, "%s", carro3gg);
            mvprintw((int)(obj.y + 2), (int)obj.x, "%s", carro4gg);
            }else{
                mvprintw((int)(obj.y + 2), (int)obj.x, " ###   ### ");
            }
        }else {
            if(dia>=0){
                mvprintw((int)obj.y, (int)(obj.x + 1), "%s", carro0pp);
                mvprintw((int)(obj.y + 1), (int)(obj.x + 1), "%s", carro1pp);
            }else{
                mvprintw((int)(obj.y + 1), (int)(obj.x + 1), " ++ ");
            }
        }
        if (is_player || dia>0) {
            attroff(COLOR_PAIR(obj.cor));
        }else if (!is_player && dia<0){
            attron(COLOR_PAIR(1));
        }
    }
}

void atualizar_pos(object *obj, int is_player) {
    if (obj->x + (obj->dx * obj->velocidade_x) >= quoficiente_esq((int)obj->y) + 1 &&
        obj->x + (obj->dx * obj->velocidade_x) <= quoficiente_dir((int)obj->y) - obj->largura) {
            if (is_player) {
                obj->x += obj->dx * obj->velocidade_x * 2;
            } else {
                obj->x += obj->dx * obj->velocidade_x;
            }
    }
    if (obj->y + (obj->dy * obj->velocidade_y) >= altura_pista_minima &&
        obj->y + (obj->dy * obj->velocidade_y) <= altura_pista_max - largura_carroG) {
        if (is_player) {
            obj->y += obj->dy * obj->velocidade_y;
        }
    }
    if (is_player && (obj->x > largura-2.8*obj->largura || obj->x < 1.8*obj->largura)){
        obj->x+=obj->dx*-9;
    }
}

void gerenciar_carro(object *obj, int is_player) {
    float track_h = (float)(altura_pista_max - altura_pista_minima);
    float t = 0.0f;
    if (track_h > 0.0f) {
        t = ((float)(obj->y - altura_pista_minima)) / track_h;
    }
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.2f) {               
        obj->largura = largura_carroPP;
        obj->altura  = altura_carroPP;
    } else if (t < 0.5f) {      
        obj->largura = largura_carroG;
        obj->altura  = altura_carroG;
    } else {         
        obj->largura = largura_carroGG;
        obj->altura  = altura_carroGG;
    }
    obj->velocidade_y = 3;
    obj->velocidade_x = 3;
    atualizar_pos(obj, is_player);
    print_carro(*obj, is_player);
}


int colisao(object obj1,object obj2){
    if (obj1.x < obj2.x + obj2.largura && obj1.x + obj1.largura > obj2.x && obj1.y < obj2.y + obj2.altura && obj1.y + obj1.altura > obj2.y) {
        return 1;
    }
    return 0;
}

void tabela_pontuacao(){
    for (int i=meio-20;i<=meio+20;i++){
        for (int j= altura_pista_max;j<=altura;j++){
            attron(COLOR_PAIR(6));
            mvprintw(j,i," ");
            attroff(COLOR_PAIR(6));
        }
    }
    attron(COLOR_PAIR(7));
    mvprintw(altura-3,meio-5,"q|_|p %d ",carros_passados);
    mvprintw(altura-2,meio-5," _|_     ");
    mvprintw(altura-5,meio-5,"%d metros",metros_percorridos);
    attroff(COLOR_PAIR(7));
}

void print_ceu(){
    char sol[]="     ";
    float y=2;
    float x= largura*dia;
    if (x<0){
        x=x+largura;
    }
    if (dia>0.9 || dia<-0.9){
        y=2;
    }else if (dia>0){
        y++;
    }else if (dia<0){
        y--;
    }
    if (dia>0){
        attron(COLOR_PAIR(8));
    }else{
        attron(COLOR_PAIR(10));
    }
    for (int j=altura_pista_minima-1;j>=0;j--){
        for(int i=0;i<=largura;i++){
            mvprintw(j,i," ");
        }
    }
    if (dia>0){
    attroff(COLOR_PAIR(8));
    }else{
        attroff(COLOR_PAIR(10));
    }
    if (dia <= 0){ 
        attron(COLOR_PAIR(9));
        int estrelas = 4;
        for(int e = 0; e < estrelas; e++){
            int ex = get_random(largura);
            int ey = get_random(altura_pista_minima);
            mvprintw(ey, ex, "*");
        }
        attroff(COLOR_PAIR(9));
    }
    if (dia>0){
        attron(COLOR_PAIR(2));
    }else{
    attron(COLOR_PAIR(11));
    }

    mvprintw(y,x,"%s",sol);
    mvprintw(y+1,x,"%s",sol);
    mvprintw(y+1,x,"%s",sol);

    if (dia>0){
        attroff(COLOR_PAIR(2));
    }else{
        attroff(COLOR_PAIR(11));
    }
}

void nevasca(){
    char gota='*';
    int gotas = 20, x, y;
    for (int i = 0; i <= gotas; i++) {
        x = get_random(largura);
        y = get_random(altura_pista_max);
        mvaddch(y, x, gota);
        mvchgat(y, x, 1, A_NORMAL, 11, NULL);
    }
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
    altura_pista_minima = (int)(altura * 0.2); 
    altura_pista_max = altura - 6;             
    meio = largura / 2;
    int contador_de_linha=0;
    int contador_tempo=0;

    player.x = (int)meio - (largura_carroG / 2);
    player.y = altura_pista_max - largura_carroG;
    player.velocidade_x = 2;

    criar_inimigos();
    long long ultima_mudanca = tempo_em_ms();
    float count_metros=0;
    while (true) {

        erase();

        long long agora = tempo_em_ms();
        if (agora - ultima_mudanca > 5000) {
            gerar_nova_curva();
            contador_tempo++;
            if(contador_tempo>=8){
                mudar_clima();
                contador_tempo=0;
            }
            ultima_mudanca = agora;
        }
        atualizar_curva();

        count_metros+=0.1;

        print_ceu();
        pista();

        dia-=0.0007;
        if (dia<=-1){
            dia=1;
        }
        if (ambiente==1){
            nevasca();
        }
        tabela_pontuacao();
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

            if (carro[i].y + carro[i].altura > altura_pista_max) {
                carros_passados+=1;
                carro[i].pontuado = 1;
                int pista_esq = meio - 6;
                int pista_dir = meio + 6 - largura_carroG;
                carro[i].x = pista_esq + get_random(pista_dir - pista_esq);
                carro[i].y = altura_pista_minima - get_random(30) * altura_carroPP;
                mudar_modificador(&carro[i]);
                carro[i].cor=mudar_cor_carro();
                usleep(20000);
                carro[i].pontuado=0;
            }


            int movimento = get_random(10);
            if (movimento < 5) {
                carro[i].dx = ((get_random(11) - 5) / 5);
            } else if(movimento < 7){
                carro[i].dx = ((get_random(11) - 5) / 5) + carro[i].modificador;
            }
            gerenciar_carro(&carro[i], 0);
            if (colisao(carro[i],player)){
                refresh();
                count_metros-=2;
                carro[i].y-=10;
                carro[i].x-=player.dx*2;
                player.dx+=carro[i].dx*2;
                break;
            }else{
                metros_percorridos=round(count_metros);
            }
        }
        contador_de_linha++;
        if (contador_de_linha >= 4) { 
            offset_linha--;
            if (offset_linha <= altura_pista_minima) offset_linha = altura_pista_max;
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

        attron(COLOR_PAIR(5));
        mvprintw(0.75*altura,meio-6,"Créditos [C]");
        attroff(COLOR_PAIR(5));

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
        for (int i=0;i<=largura;i++){
            mvprintw(0,i,"=");
            mvprintw(4,i,"=");
            mvprintw(12,i,"=");
        }
        mvprintw(2,meio-4,"Créditos");
        mvprintw(6,meio-18,"Programador e designer geral: Jovito.");
        mvprintw(8,meio-17,"Programador das fisicas: Rodrigo.");
        mvprintw(11,meio-17,"Colaboradores: Ricardo e Matheus.");
        mvprintw(13,meio-5,"GITHUBs:");
        mvprintw(15,meio-18,"Jovito: https://github.com/Jovito5s9");
        mvprintw(16,meio-7,"(Digite [J])");
        mvprintw(18,meio-20,"Rodrigo: https://github.com/RodriSC-blip");
        mvprintw(19,meio-7,"(Digite [R])");
        mvprintw(21,meio-32,"Repositório do jogo: https://github.com/Jovito5s9/jogo_enduro");
        mvprintw(22,meio-7,"(Digite [G])");
        ch = getch();
        if (ch == 's' || ch == 'S'){
            return;
        }
        if (ch == 'j' || ch == 'J'){
            system("xdg-open https://github.com/Jovito5s9 2>/dev/null");
        }
        if (ch == 'r' || ch == 'R'){
            system("xdg-open https://github.com/RodriSC-blip 2>/dev/null");
        }
        if (ch == 'g' || ch == 'G'){
            system("xdg-open https://github.com/Jovito5s9/jogo_enduro 2>/dev/null");
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
    init_pair(6, COLOR_YELLOW,COLOR_YELLOW);
    init_pair(7, COLOR_WHITE,COLOR_RED);
    init_pair(8, COLOR_BLUE, COLOR_BLUE);
    init_pair(9, COLOR_WHITE, -1);
    init_pair(10, COLOR_BLACK, COLOR_BLACK);
    init_pair(11, COLOR_WHITE, COLOR_WHITE);
    init_pair(12, COLOR_RED, COLOR_RED);

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