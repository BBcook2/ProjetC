#include <stdio.h>
#include <math.h>


#define MAX_MASSES     32 // plus tard fair en sorte que se soit dynamique 1/2 stack
#define MAX_RESSORTS   64 // 1 stack

typedef struct {//structure des masses de la créature
    float x, y;
    float vitessex, vitessey;
    float forcex, forcey;
    float poid; 
    float rayon;
    float amortissement;
    int type_materiau; // id du materiaux
} Masse;

typedef struct{ // pemet de fair des precept de materiaux
    int id_materiaux;
    float poid;
    float rayon;
    float amortissement;
}Materiaux;

typedef struct { //Structure des ressorts et des muscle ressorts
    int masse1, masse2;            
    float longueur;      
    float rigidite;               
    float amortissement;           
    // Paramètre Ressort musculaires
    int muscle; //1 est un muscle 0 n'est pas un muscle   
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

Materiaux listes_materiaux[4] = { //Listes des matériaux possible pour le joueur
    { 0, 0.5f, 6.0f, 0.8f },  // chair
    { 1, 1.0f, 7.0f, 0.3f },  // os
    { 2, 0.3f, 8.0f, 1.3f },  // slime 
    { 3, 1.8f, 5.0f, 0.1f }   // acier galvanisé
};

void Initialisation_creature(Creature *creature){// Initialisation de la creature avec les parametre par défaut
    (*creature).nb_masses = 0;
    (*creature).nb_ressorts = 0;
    (*creature).energie = 100 ;
    (*creature).temps_survie = 0;
    (*creature).distance_parcourue = 0;
}

void creature_ajoute_masse(Creature *creature,int x, int y,Materiaux type){
    if ((*creature).nb_masses >= MAX_MASSES) {
        printf("erreur trop de masses\n");
        return;
    }
    Masse *masse = &creature->masses[(*creature).nb_masses];
    (*creature).nb_masses += 1;

    (*masse).x = x;
    (*masse).y = y;
    (*masse).vitessex = 0;
    (*masse).vitessey = 0;
    (*masse).forcex = 0;
    (*masse).forcey = 0;
    (*masse).poid = type.poid;
    (*masse).rayon = type.rayon;
    (*masse).amortissement = type.amortissement;
    (*masse).type_materiau = type.id_materiaux;
}

void creature_ajoute_ressort(Creature *creature,int masse1,int masse2,float rigidite,float amortissement,int muscle,float amplitude,float frequence){
    if ((*creature).nb_ressorts >= MAX_RESSORTS) { // creation d'un ressort entre deux masses en fonction des critere de la fonction
        printf("erreur trop de muscle\n");
        return;
    }

    if (masse1 < 0 || masse1 >= creature->nb_masses || masse2 < 0 || masse2 >= creature->nb_masses) {
        printf("erreur indices de masses invalides (%d, %d)\n", masse1, masse2);
        return;
    }

    float dx = (*creature).masses[masse2].x - (*creature).masses[masse1].x;
    float dy = (*creature).masses[masse2].y - (*creature).masses[masse1].y;
    float distance = sqrtf(dx*dx + dy*dy);  


    Ressort *ressort = &creature->ressorts[(*creature).nb_ressorts];
    (*creature).nb_ressorts +=1;

    (*ressort).masse1 = masse1;
    (*ressort).masse2 = masse2;
    (*ressort).longueur = distance;
    (*ressort).rigidite = rigidite;
    (*ressort).amortissement = amortissement;

    (*ressort).muscle = muscle;
    (*ressort).amplitude = amplitude;
    (*ressort).frequence = frequence;

}

void creature_appliquer_force(Creature *creature, float temps_total) {
    for (int i = 0; i < (*creature).nb_masses; i++) { //aplique sur chaque masse la gravité de base
        Masse *masse = &creature->masses[i];
        (*masse).forcex = 0;
        (*masse).forcey = (*masse).poid * 9.81f; // 9.81 gravité arrondie
    }

    for (int i = 0; i < creature->nb_ressorts; i++) { //Pour chaque ressort aplique les forces celon le type de liasion muscle etc

        Ressort *ressort = &creature->ressorts[i];

        Masse *masse1 = &creature->masses[ressort->masse1];
        Masse *masse2 = &creature->masses[ressort->masse2];

        float distancex = masse2->x - masse1->x;
        float distancey = masse2->y - masse1->y;
        float distance = sqrtf(distancex*distancex + distancey*distancey);

        float longueur = ressort->longueur;

        if (ressort->muscle) { //Pour que le muscle change la longueur en fonction du temp sur le temp 
            float oscillation = ressort->amplitude * sinf(2.0f * 3.14159f * ressort->frequence * temps_total);// oscillation permet de savoir si un muscle s'etire en longueur donc  positif ou retractre négatif d'ou le sinf => sinus et PI
            longueur = ressort->longueur * (1.0f + oscillation); //si besoin video explicative "https://www.youtube.com/watch?v=0fGSabpRA-8"
        }

        float force = ressort->rigidite * (distance - longueur);
        float newx = distancex / distance;
        float newy = distancey / distance;

        float distancevitessex = masse2->vitessex - masse1->vitessex;
        float distancevitessey = masse2->vitessey - masse1->vitessey;
        float amortissement = ressort->amortissement * (distancevitessex * newx + distancevitessey * newy);

        float forcex = (force + amortissement) * newx;
        float forcey = (force + amortissement) * newy;

        //Aplication des force opposé
        masse1->forcex += forcex;
        masse1->forcey += forcey;

        masse2->forcex -= forcex;
        masse2->forcey -= forcey;
    }
}

int main(void) {
    Creature creature;
    Initialisation_creature(&creature);

    creature_ajoute_masse(&creature, 100.0f, 200.0f, listes_materiaux[0]);
    creature_ajoute_masse(&creature, 140.0f, 200.0f, listes_materiaux[1]);
    creature_ajoute_ressort(&creature, 0, 1, 50.0f, 0.5f, 1, 0.2f, 2.0f);

    float temps = 0.0f;
    float deltatemp = 0.016f; // 60 FPS (= combien de frame/image par seconde actuellement c'est 1/60 )

    for (int step = 0; step < 10; ++step) {
        creature_appliquer_force(&creature, temps);

        for (int i = 0; i < creature.nb_masses; ++i) {
            Masse *masse = &creature.masses[i];
            float ax = masse->forcex / masse->poid;
            float ay = masse->forcey / masse->poid;
            masse->vitessex += ax * deltatemp;
            masse->vitessey += ay * deltatemp;
            masse->x += masse->vitessex * deltatemp;
            masse->y += masse->vitessey * deltatemp;
        }
        printf("distance x : %2f Distance y : %2f \n",creature.masses[0].x,creature.masses[0].y);

        temps += deltatemp;
    }

    printf("Simulation terminée\n");
    return 0;
}

//video expliquant le systeme masse ressort "https://www.youtube.com/watch?v=qMx4GSX6-2s" si besoin