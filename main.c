#include <stdio.h>
#include <math.h>

#define MAX_MASSES     32
#define MAX_RESSORTS   64

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
    int id;
    Masse masses[MAX_MASSES];
    int nb_masses;
    Ressort ressorts[MAX_RESSORTS];
    int nb_ressorts;
    float energie;
    float temps_survie;
    float distance_parcourue;
} Creature;

Materiaux listes_materiaux[4] = {
    { 0, 0.5f, 6.0f, 0.8f },
    { 1, 1.0f, 7.0f, 0.3f },
    { 2, 0.3f, 8.0f, 1.3f },
    { 3, 1.8f, 5.0f, 0.1f }
};

void Initialisation_creature(Creature *creature){
    creature->nb_masses = 0;
    creature->nb_ressorts = 0;
    creature->energie = 100;
    creature->temps_survie = 0;
    creature->distance_parcourue = 0;
}

void creature_ajoute_masse(Creature *creature, int x, int y, Materiaux type){
    if (creature->nb_masses >= MAX_MASSES) {
        printf("Erreur : trop de masses\n");
        return;
    }

    Masse *masse = &creature->masses[creature->nb_masses++];
    masse->x = x;
    masse->y = y;
    masse->vitessex = 0;
    masse->vitessey = 0;
    masse->forcex = 0;
    masse->forcey = 0;
    masse->poid = type.poid;
    masse->rayon = type.rayon;
    masse->amortissement = type.amortissement;
    masse->type_materiau = type.id_materiaux;
}

void creature_ajoute_ressort(Creature *creature,int masse1,int masse2,float rigidite,float amortissement,int muscle,float amplitude,float frequence){
    if (creature->nb_ressorts >= MAX_RESSORTS) {
        printf("Erreur : trop de ressorts\n");
        return;
    }

    if (masse1 < 0 || masse2 < 0 || masse1 >= creature->nb_masses || masse2 >= creature->nb_masses) {
        printf("Erreur indices de masses invalides (%d, %d)\n", masse1, masse2);
        return;
    }

    Masse *m1 = &creature->masses[masse1];
    Masse *m2 = &creature->masses[masse2];

    float dx = m2->x - m1->x;
    float dy = m2->y - m1->y;
    float distance = sqrtf(dx*dx + dy*dy);

    if (distance < 0.0001f)
        distance = 0.0001f;

    Ressort *r = &creature->ressorts[creature->nb_ressorts++];
    r->masse1 = masse1;
    r->masse2 = masse2;
    r->longueur = distance;
    r->rigidite = rigidite;
    r->amortissement = amortissement;
    r->muscle = muscle;
    r->amplitude = amplitude;
    r->frequence = frequence;
}

void creature_appliquer_force(Creature *creature, float temps_total) {

    // Gravité corrigée (vers le bas)
    for (int i = 0; i < creature->nb_masses; i++) {
        Masse *masse = &creature->masses[i];
        masse->forcex = 0;
        masse->forcey = -masse->poid * 9.81f;
    }

    for (int i = 0; i < creature->nb_ressorts; i++) {

        Ressort *r = &creature->ressorts[i];
        Masse *m1 = &creature->masses[r->masse1];
        Masse *m2 = &creature->masses[r->masse2];

        float dx = m2->x - m1->x;
        float dy = m2->y - m1->y;
        float distance = sqrtf(dx*dx + dy*dy);

        // éviter division par zéro
        if (distance < 0.0001f)
            continue;

        float longueur = r->longueur;

        if (r->muscle) {
            float oscillation = r->amplitude * sinf(2.0f * 3.14159f * r->frequence * temps_total);
            longueur = r->longueur * (1.0f + oscillation);
        }

        float force = r->rigidite * (distance - longueur);
        float nx = dx / distance;
        float ny = dy / distance;

        float dvx = m2->vitessex - m1->vitessex;
        float dvy = m2->vitessey - m1->vitessey;
        float amort = r->amortissement * (dvx * nx + dvy * ny);

        float fx = (force + amort) * nx;
        float fy = (force + amort) * ny;

        m1->forcex += fx;
        m1->forcey += fy;

        m2->forcex -= fx;
        m2->forcey -= fy;
    }
}

int main(void) {

    Creature creature;
    Initialisation_creature(&creature);

    creature_ajoute_masse(&creature, 100.0f, 200.0f, listes_materiaux[0]);
    creature_ajoute_masse(&creature, 140.0f, 200.0f, listes_materiaux[1]);
    creature_ajoute_ressort(&creature, 0, 1, 50.0f, 0.5f, 1, 0.2f, 2.0f);

    float temps = 0.0f;
    float dt = 0.016f;

    for (int step = 0; step < 10; ++step) {

        creature_appliquer_force(&creature, temps);

        for (int i = 0; i < creature.nb_masses; i++) {
            Masse *m = &creature.masses[i];

            float ax = m->forcex / m->poid;
            float ay = m->forcey / m->poid;

            m->vitessex += ax * dt;
            m->vitessey += ay * dt;

            m->x += m->vitessex * dt;
            m->y += m->vitessey * dt;
        }

        printf("M0 : x=%.2f y=%.2f\n", creature.masses[0].x, creature.masses[0].y);

        temps += dt;
    }

    printf("Simulation terminée\n");
    return 0;
}
