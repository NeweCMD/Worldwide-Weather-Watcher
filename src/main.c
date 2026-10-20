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

/*///////// INITIALISATION DES VARIABLES GLOBALES /////////*/

char mode = 's'; /* Le code principal (hors configuration) fonctionnera en "state machine"
           cela signifie que la machine aura un nombre fini d'états possibles
           cette variable permet au code de savoir dans quel état il se trouve*/
/* valeurs possibles : "c" : mode configuration, "s": mode standard, "e" : mode économie, "m" : mode maintenance*/


/* variables globales pour le paramétrage */
/* paramètres du capteur de luminosité */

bool LUMIN = true; /* un paramètre déterminant si le capteur de luminosité est 
                    prit en compte / est installé lors du prélèvement de données*/

int LUMIN_LOW = 255; /* seuil de faible luminosité ; valeurs comprises entre 0 et 1023 */

int LUMIN_HIGH = 768; /* seuil de forte luminosité ; valeurs comprises entre 0 et 1023 */

/* paramètres des capteurs de la BME280 */
/* paramètres du capteur de température de l'air */

bool TEMP_AIR = true; /* paramètre déterminant si le capteur de température de l'air 
                        est prit en compte / est installé lors du prélèvement de données*/

int MIN_TEMP_AIR = -10; /* seuil de faible température de l'air ; valeurs comprises entre -40 et 85 */

int MAX_TEMP_AIR = 60; /* seuil de forte température de l'air ; valeurs comprises entre -40 et 85 */

/* paramètres du capteur d'hygrométrie */

bool HYGR = true; /* paramètre déterminant si le capteur de température de l'eau 
                est prit en compte / est installé lors du prélèvement de données*/

int HYGR_MINT = 0; /* seuil de faible température de l'eau ; valeurs comprises entre -40 et 85 */

int HYGR_MAXT = 50; /* seuil de forte température de l'eau ; valeurs comprises entre -40 et 85 */

/* paramètres du capteur de pression */

bool PRESSURE = true; /* paramètre déterminant si le capteur de pression est prit en
                         compte / est installé lors du prélèvement de données*/

int PRESSURE_MIN = 850; /* seuil minimal de pression sous marine ; valeurs comprises entre 300 et 1100*/

int PRESSURE_MAX = 1080; /* seuil maximal de pression sous marine ; valeurs comprises entre 300 et 1080*/

/* autres paramètres du mode configuration : */

int LOG_INTERVAL = 10; /* intervalle entre chaque relevé de données (en minutes) 
                        LOG_INTERVAL sera automatiquement multiplié par 2 en mode économie*/

int MAX_FILE_SIZE = 4096; /* taille maximale qu'un fichier de relevé de données peut prendre en mémoire SD 
            (une partition de  taille MAX_FILE_SIZE sera allouée pour chaque nouveau fichier enregistré)*/

int TIMEOUT = 30; /* durée en secondes avant que le capteur pélevant des données soit mit automatiquement en erreur 
                (au bout de deux timeouts consécutifs, le capteur est signalé comme défectueux)*/
  

/* Partie du code pour le mode configuration.
Objectif : sur appui du bouton vert, le système attend une entrée depuis l'interface série du microcontroleur.
Si 30 minutes d'inactivitées sont passées, (ou si le bouton rouge est à nouveau pressé) .
Le programme bascule en mode standard (en sortant de la boucle du mode configuration).*/
/* principe pour la détection de commandes : utilisation d'un AFD, détection de la commande par AFD, puis d'un symbole "=", le reste de la chaine de caractères sera la valeur du paramètre entré */
void configuration(){
    /* ( secondes de délai avant toute intéraction afin d'éviter que le bouton rouge soit détécté immédiatement en appui (et un retour immédiat en mode standard)*/
    HAL_Delay(4000);
    for (int i=0; i<5; i++){
        GroveRGB_SetColor(255,255,255); HAL_Delay(100);
        GroveRGB_SetColor(255,255,0); HAL_Delay(100);
    }
    /*boucle pour le mod configuration*/
    while (mode=='c'){
        /* passage en mode standard si bouton rouge pressé*/
        if (GroveButton1_IsPressed()==1){
            mode='s';
        }
    }
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
    /*utilisé pour le RGB de la LED pour les animations*/
    int r,g,b;
    while (true){
        /* changement de la couleur de la LED en fonction du mode de fonctionnement*/
        if (mode=='s'){
            GroveRGB_SetColor(0,255,0);
            r=0;
            g=255;
            b=0;
        }
        if (mode=='e'){
            GroveRGB_SetColor(0,0,255);
            r=0;
            g=0;
            b=255;
        }
        if (mode=='m'){
            GroveRGB_SetColor(255,150,0);
        }
        /*détection du bouton rouge pour passage en mode maintenance*/
        if (GroveButton1_IsPressed()==1){
            /* i est utilisé pour l'animation de la LED*/
            int i = 0 ;
            while (GroveButton1_IsPressed()==1 && i <4){
                if (mode!='m'){
                    GroveRGB_SetColor(255,150,0); HAL_Delay(100);
                    GroveRGB_SetColor(r,g,b); HAL_Delay(900);
                }else{
                    GroveRGB_SetColor(r,g,b); HAL_Delay(100);
                    GroveRGB_SetColor(255,150,0); HAL_Delay(900);
                }
                i++;
            }
            if (i==4 && GroveButton1_IsPressed()==1){
                if (mode!='m'){
                    mode = 'm';
                }else{
                    if (r==0 && g==255 && b==0){
                        mode = 's';
                    }else{
                        mode = 'e';
                    }
                }
            }
            i=0;
        }
        if (GroveButton2_IsPressed()==1){
            int i = 0;
            while (GroveButton2_IsPressed()==1 && i <4 && mode!='m'){
                if (mode=='s'){
                    GroveRGB_SetColor(0,0,255); HAL_Delay(100);
                    GroveRGB_SetColor(0,255,0); HAL_Delay(900);
                }else{
                    GroveRGB_SetColor(0,255,0); HAL_Delay(100);
                    GroveRGB_SetColor(0,0,255); HAL_Delay(900);
                }
                i++;
            }
            if (i==4 && GroveButton2_IsPressed()==1){
                if (mode=='s'){
                    mode='e';
                }else{
                    mode='s';
                }
            }

        }

    }
}

/* Cette fonction se lancera automatiquement au démarrage de la machine
il s'agit de la fonction qui déterminera un lancement en mode configuration ou en mode standard*/
void initialisation(){
    /* initialisation des boutons et de la led du système */
    GroveButton_Init();
    GroveRGB_Init();

    /* démarrage en mode configuration*/
    if (GroveButton1_IsPressed()==1){
        mode='c';
        GroveRGB_SetColor(255,255,0);
        configuration();
    }
    mode = 's';
    hub_mode();
    
}

int main(void)
{
    Global_Init();
    /* Appel de la fonction de configuration ici (si appui bouton rouge détecté au démarrage). */
    /* Appel du hub de modes */
    initialisation();
}
