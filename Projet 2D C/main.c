#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>

#define MAX_MASSES     16
#define MAX_RESSORTS   32

#define GRAVITY        1.0f
#define SOL_Y          500.0f

#define ENERGIE_DEPART     100.0f
#define ENERGIE_PERTE_SOL  0.1f
#define ENERGIE_PERTE_TPS  0.003f

typedef struct {
    float x, y;
    float vitessex, vitessey;
    float forcex, forcey;
    float poid;
    float rayon;
    float amortissement;
    int type_materiau;
} Masse;

typedef struct {
    int id_materiaux;
    float poid;
    float rayon;
    float amortissement;
} Materiaux;

typedef struct {
    int masse1, masse2;
    float longueur;
    float rigidite;
    float amortissement;
    int muscle;
    float amplitude;
    float frequence;
} Ressort;

typedef struct {
    Masse masses[MAX_MASSES];
    int nb_masses;
    Ressort ressorts[MAX_RESSORTS];
    int nb_ressorts;
    float energie;
    float temps_survie;
} Creature;

Materiaux listes_materiaux[4] = {
    { 0, 0.5f, 10.0f, 0.8f }, // chair
    { 1, 1.0f, 12.0f, 0.3f }, // os
    { 2, 0.3f, 8.0f, 1.3f },  // slime
    { 3, 1.8f, 10.0f, 0.1f }  // acier
};

void Initialisation_creature(Creature *c) {
    c->nb_masses = 0;
    c->nb_ressorts = 0;
    c->energie = ENERGIE_DEPART;
    c->temps_survie = 0;
}

void creature_ajoute_masse(Creature *c, float x, float y, Materiaux type) {
    Masse *m = &c->masses[c->nb_masses++];
    m->x = x; m->y = y;
    m->vitessex = m->vitessey = 0;
    m->forcex = m->forcey = 0;
    m->poid = type.poid;
    m->rayon = type.rayon;
    m->amortissement = type.amortissement;
    m->type_materiau = type.id_materiaux;
}

void creature_ajoute_ressort(Creature *c, int m1, int m2,
                             float rigidite, float amortissement,
                             int muscle, float amplitude, float frequence) {
    Masse *a = &c->masses[m1];
    Masse *b = &c->masses[m2];
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dist = sqrtf(dx*dx + dy*dy);

    Ressort *r = &c->ressorts[c->nb_ressorts++];
    r->masse1 = m1; r->masse2 = m2;
    r->longueur = dist;
    r->rigidite = rigidite;
    r->amortissement = amortissement;
    r->muscle = muscle;
    r->amplitude = amplitude;
    r->frequence = frequence;
}

void draw_circle(SDL_Renderer *renderer, int x, int y, int rayon) {
    for (int w = 0; w < rayon * 2; w++)
        for (int h = 0; h < rayon * 2; h++) {
            int dx = rayon - w;
            int dy = rayon - h;
            if (dx*dx + dy*dy <= rayon*rayon)
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
        }
}

void set_color_material(SDL_Renderer *r, int type) {
    switch(type) {
        case 0: SDL_SetRenderDrawColor(r, 220,50,50,255); break;   // chair rouge
        case 1: SDL_SetRenderDrawColor(r, 240,240,240,255); break; // os blanc
        case 2: SDL_SetRenderDrawColor(r, 80,220,80,255); break;   // slime vert
        case 3: SDL_SetRenderDrawColor(r, 50,100,220,255); break;  // acier bleu
    }
}

void creature_appliquer_force(Creature *c, float temps) {
    for (int i = 0; i < c->nb_masses; i++) {
        Masse *m = &c->masses[i];
        m->forcex = 0;
        m->forcey = m->poid * GRAVITY;

        if (m->y > SOL_Y) {
            m->y = SOL_Y;
            m->vitessey = 0;        // le sol arrête le mouvement vertical
            c->energie -= ENERGIE_PERTE_SOL;
        }
    }
}

void afficher_HUD(SDL_Renderer *r, Creature *c) {
    SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
    SDL_Rect hud = {10, 10, 200, 40};
    SDL_RenderFillRect(r, &hud);
}

int main(int argc, char *argv[]) {
    Creature c;
    Initialisation_creature(&c);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("Creature Masse-Ressort",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, 0);
    SDL_Renderer *r = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);

    creature_ajoute_masse(&c, 400,150, listes_materiaux[1]); // tête (os)
    creature_ajoute_masse(&c, 400,200, listes_materiaux[0]); // torse (chair)
    creature_ajoute_masse(&c, 360,200, listes_materiaux[0]); // bras G
    creature_ajoute_masse(&c, 440,200, listes_materiaux[0]); // bras D
    creature_ajoute_masse(&c, 380,260, listes_materiaux[2]); // jambe G (slime)
    creature_ajoute_masse(&c, 420,260, listes_materiaux[2]); // jambe D

    creature_ajoute_ressort(&c, 0,1, 30,1.0f, 1,0.03f,1); // tête-torse
    creature_ajoute_ressort(&c, 1,2, 35,1.0f, 0,0,0);     // torse-bras G
    creature_ajoute_ressort(&c, 1,3, 35,1.0f, 0,0,0);     // torse-bras D
    creature_ajoute_ressort(&c, 1,4, 40,1.0f, 1,0.05f,1); // torse-jambe G
    creature_ajoute_ressort(&c, 1,5, 40,1.0f, 1,0.05f,1); // torse-jambe D

    float temps = 0.0f, dt = 0.016f;
    int running = 1;

    while (running) {
        if (c.energie <= 0) break;

        SDL_Event e;
        while (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = 0;

        creature_appliquer_force(&c, temps);
        c.temps_survie += dt;
        c.energie -= ENERGIE_PERTE_TPS;

        // intègre les forces → mouvement
        for (int i = 0; i < c.nb_masses; i++) {
            Masse *m = &c.masses[i];
            m->vitessex += (m->forcex / m->poid) * dt;
            m->vitessey += (m->forcey / m->poid) * dt;
            m->vitessex *= 0.98f;
            m->vitessey *= 0.98f;
            m->x += m->vitessex * dt;
            m->y += m->vitessey * dt;
        }

        SDL_SetRenderDrawColor(r, 20,20,20,255);
        SDL_RenderClear(r);

        // Sol
        SDL_SetRenderDrawColor(r, 120,120,120,255);
        SDL_Rect sol = {0, (int)SOL_Y + 10, 800, 20};
        SDL_RenderFillRect(r, &sol);

        // Ressorts
        SDL_SetRenderDrawColor(r, 255,255,255,255);
        for(int i=0;i<c.nb_ressorts;i++){
            Ressort *rs=&c.ressorts[i];
            Masse *m1=&c.masses[rs->masse1];
            Masse *m2=&c.masses[rs->masse2];
            SDL_RenderDrawLine(r,m1->x,m1->y,m2->x,m2->y);
        }

        // Masses (cercles)
        for(int i=0;i<c.nb_masses;i++){
            Masse *m = &c.masses[i];
            set_color_material(r, m->type_materiau);
            draw_circle(r, m->x, m->y, m->rayon);
        }

        afficher_HUD(r, &c);

        SDL_RenderPresent(r);
        temps += dt;
    }

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    SDL_Quit();
    exit(EXIT_SUCCESS);
}
