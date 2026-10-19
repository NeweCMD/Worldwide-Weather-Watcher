#include <stdio.h>
#include "setup.h"
#include "cesi_types.h"
#include "grove_rgb.h"
#include "grove_button.h"
#include "grove_light.h"
#include "grove_rtc_ds1307.h"
#include "grove_gps_air530z.h"
#include "grove_bme680.h"
#include "sd_logger.h"

/* Partie du code pour le mode configuration.
Objectif : sur appui du bouton vert, le système attend une entrée depuis l'interface série du microcontroleur.
Si 30 minutes d'inactivitées sont passées, (ou si le bouton rouge est à nouveau pressé) .
Le programme bascule en mode standard (en sortant de la boucle du mode configuration).*/
void configuration(){

}

/* Partie du code pour le mode prélèvement de données
Ceci inclut une détection du mode économique qui modifie légèrement le comportement da la fonction*/
void prelevement(){

}

/* Partie du code pour le mode maintenance :
 Nous devons être capable de consulter les données de la carte SD, et également pouvoir l'éjecter*/
void maintenance(){

}

/* Partie pour la fonction de changement de mode
Il s'agit du "menu principal" de notre programme, qui permet d'initialiser les prélèvement et de changer de mode en cas d'appui de bouton
*/
void hub_mode(){

}

/* Cette fonction se lancera automatiquement au démarrage de la machine
il s'agit de la fonction qui déterminera un lancement en mode configuration ou en mode standard*/
void initialisation(){

}

int main(void)
{
    Global_Init();
    /* Appel de la fonction de configuration ici (si appui bouton rouge détecté au démarrage). */
    /* Appel du hub de modes */
}
