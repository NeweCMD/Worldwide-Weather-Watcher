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
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef huart2;

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
     

/* configuration d'uart2 pour l'entrée de commandes en mode configuration */
void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

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
        /* détection de l'appui du bouton vert - on initie une attente d'entrée de commande*/
        if (GroveButton2_IsPressed()==1){
            char ligne[64];
            uint8_t c;
            size_t pos = 0;

            printf("\r\n=== Bouton vert détecté ===\r\n");
            printf("Veuillez entrer une commande\r\n");

            while (1)
            {
                HAL_StatusTypeDef status =
                    HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY);

                if (status != HAL_OK)
                {
                    printf("Erreur de reception UART\r\n");
                    continue;
                }

                // Fin de commande : CR ou LF
                if (c == '\r' || c == '\n')
                {
                    // Ignorer les lignes vides
                    if (pos == 0)
                        continue;

                    ligne[pos] = '\0';
                    break;
                }

                // Stocker le caractère si le tampon a encore de la place
                if (pos < sizeof(ligne) - 1)
                {
                    ligne[pos++] = (char)c;
                }
                else
                {
                    printf("Erreur : commande trop longue\r\n");

                    // Vider le reste de la ligne
                    while (1)
                    {
                        if (HAL_UART_Receive(&huart2, &c, 1,
                                            HAL_MAX_DELAY) == HAL_OK)
                        {
                            if (c == '\r' || c == '\n')
                                break;
                        }
                    }

                    pos = 0;
                    printf("Reessayez.\r\n");
                    continue;
                }
            }

            printf("Commande recue : >%s<\r\n", ligne);
            printf("Longueur : %u\r\n", (unsigned int)strlen(ligne));

            // Séparer COMMANDE=VALEUR
            char *separateur = strchr(ligne, '=');

            if (separateur == NULL)
            {
                printf("Format invalide : utilisez COMMANDE=VALEUR\r\n");
            }
            else
            {
                *separateur = '\0';

                char *commande = ligne;
                char *valeur = separateur + 1;


                if (strcmp(commande, "PRESSURE") == 0 ||
                    strcmp(commande, "pressure") == 0)
                {
                    if (strcmp(valeur, "true") == 0)
                    {
                        PRESSURE = true;
                        printf("capteur de pression activé\r\n");
                    }
                    else if (strcmp(valeur, "false") == 0)
                    {
                        PRESSURE = false;
                        printf("capteur de pression désactivé\r\n");
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être égale à true ou false\n", valeur);
                    }
                }
                else if (strcmp(commande, "PRESSURE_MIN") == 0 ||
                        strcmp(commande, "pressure_min") == 0 ||
                        strcmp(commande, "pressuremin") == 0 ||
                        strcmp(commande, "PRESSUREMIN") == 0 )

                {
                    if (atoi(valeur) <= 1100 && atoi(valeur) >= 300 )
                    {
                        PRESSURE_MIN = atoi(valeur);
                        printf("le seuil minimal de pression est réglé à %d HPa \r\n", PRESSURE_MIN);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre 300 et 1100\n", valeur);
                    }
                }else if (strcmp(commande, "PRESSURE_MAX") == 0 ||
                        strcmp(commande, "pressure_max") == 0 ||
                        strcmp(commande, "pressuremax") == 0 ||
                        strcmp(commande, "PRESSUREMAX") == 0 )

                {
                    if (atoi(valeur) <= 1100 && atoi(valeur) >= 300 )
                    {
                        PRESSURE_MAX = atoi(valeur);
                        printf("le seuil maximal de pression est réglé à %d HPa \r\n", PRESSURE_MAX);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre 300 et 1100\n", valeur);
                    }
                }else if (strcmp(commande, "LUMIN") == 0 ||
                    strcmp(commande, "lumin") == 0)
                {
                    if (strcmp(valeur, "true") == 0)
                    {
                        LUMIN = true;
                        printf("capteur de luminosité activé\r\n");
                    }
                    else if (strcmp(valeur, "false") == 0)
                    {
                        LUMIN = false;
                        printf("capteur de luminosité désactivé\r\n");
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être égale à true ou false\n", valeur);
                    }
                }
                else if (strcmp(commande, "LUMIN_LOW") == 0 ||
                        strcmp(commande, "lumin_low") == 0 ||
                        strcmp(commande, "luminlow") == 0 ||
                        strcmp(commande, "LUMINLOW") == 0 )

                {
                    if (atoi(valeur) <= 1023 && atoi(valeur) >= 0 )
                    {
                        LUMIN_LOW = atoi(valeur);
                        printf("le seuil minimal de luminosité est réglé à %d lumen \r\n", LUMIN_LOW);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre 0 et 1023\n", valeur);
                    }
                }else if (strcmp(commande, "LUMIN_HIGH") == 0 ||
                        strcmp(commande, "lumin_high") == 0 ||
                        strcmp(commande, "luminhigh") == 0 ||
                        strcmp(commande, "LUMINHIGH") == 0 )

                {
                    if (atoi(valeur) <= 1023 && atoi(valeur) >= 0 )
                    {
                        LUMIN_HIGH = atoi(valeur);
                        printf("le seuil maximal de luminosité est réglé à %d lumen \r\n", LUMIN_HIGH);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre 0 et 1023\n", valeur);
                    }
                }else if (strcmp(commande, "TEMP_AIR") == 0 ||
                        strcmp(commande, "TEMPAIR") == 0 ||
                        strcmp(commande, "tempair") == 0 ||
                        strcmp(commande, "temp_air") == 0)
                {
                    if (strcmp(valeur, "true") == 0)
                    {
                        TEMP_AIR = true;
                        printf("capteur de température de l'air activé\r\n");
                    }
                    else if (strcmp(valeur, "false") == 0)
                    {
                        TEMP_AIR = false;
                        printf("capteur de température de l'air désactivé\r\n");
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être égale à true ou false\n", valeur);
                    }
                }
                else if (strcmp(commande, "MIN_TEMP_AIR") == 0 ||
                        strcmp(commande, "min_temp_air") == 0 ||
                        strcmp(commande, "mintempair") == 0 ||
                        strcmp(commande, "MINTEMPAIR") == 0 )

                {
                    if (atoi(valeur) <= 85 && atoi(valeur) >= -40 )
                    {
                        MIN_TEMP_AIR = atoi(valeur);
                        printf("le seuil minimal de température de l'air est réglé à %d degrés celsius \r\n", MIN_TEMP_AIR);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre -40 et 85\n", valeur);
                    }
                }else if (strcmp(commande, "MAX_TEMP_AIR") == 0 ||
                        strcmp(commande, "max_temp_air") == 0 ||
                        strcmp(commande, "MAXTEMPAIR") == 0 ||
                        strcmp(commande, "maxtempair") == 0 )

                {
                    if (atoi(valeur) <= 85 && atoi(valeur) >= -40 )
                    {
                        MAX_TEMP_AIR = atoi(valeur);
                        printf("le seuil maximal de température de l'air est réglé à %d degrés celsius \r\n", MAX_TEMP_AIR);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre -40 et 85\n", valeur);
                    }
                }else if (strcmp(commande, "HYGR") == 0 ||
                    strcmp(commande, "hygr") == 0)
                {
                    if (strcmp(valeur, "true") == 0)
                    {
                        HYGR = true;
                        printf("capteur d'hygrométrie activé\r\n");
                    }
                    else if (strcmp(valeur, "false") == 0)
                    {
                        HYGR = false;
                        printf("capteur d'hygrométrie désactivé\r\n");
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être égale à true ou false\n", valeur);
                    }
                }
                else if (strcmp(commande, "hygr_mint") == 0 ||
                        strcmp(commande, "HYGR_MINT") == 0 ||
                        strcmp(commande, "HYGRMINT") == 0 ||
                        strcmp(commande, "hygrmint") == 0 )

                {
                    if (atoi(valeur) <= 85 && atoi(valeur) >= -40 )
                    {
                        HYGR_MINT = atoi(valeur);
                        printf("le seuil minimal d'hygrométrie est réglé à %d degrés celsius \r\n", HYGR_MINT);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre -40 et 85\n", valeur);
                    }
                }else if (strcmp(commande, "hygr_maxt") == 0 ||
                        strcmp(commande, "HYGR_MAXT") == 0 ||
                        strcmp(commande, "HYGRMAXT") == 0 ||
                        strcmp(commande, "hygrmaxt") == 0 )

                {
                    if (atoi(valeur) <= 85 && atoi(valeur) >= -40 )
                    {
                        HYGR_MAXT = atoi(valeur);
                        printf("le seuil maximal d'hygrométrie est réglé à %d degrés celsius \r\n", HYGR_MAXT);
                    }
                    else
                    {
                        printf("Valeur invalide : >%s<\r\n La valeur doit être entre -40 et 85\n", valeur);
                    }
                }else if (strcmp(commande,"LOG_INTERVAL") == 0 ||
                        strcmp(commande,"LOGINTERVAL") == 0 ||
                        strcmp(commande,"log_interval") == 0 ||
                        strcmp(commande,"loginterval") == 0    ){
                            LOG_INTERVAL=atoi(valeur);
                            printf("La durée entre les relevés sera maintenant de %d minutes. \r\n", LOG_INTERVAL);

                }else if (strcmp(commande,"MAX_FILE_SIZE") == 0 ||
                        strcmp(commande,"MAXFILESIZE") == 0 ||
                        strcmp(commande,"max_file_size") == 0 ||
                        strcmp(commande,"maxfilesize") == 0    ){
                            MAX_FILE_SIZE=atoi(valeur);
                            printf("La taille maximale des fichiers sera maintenant de %d octets. \r\n", MAX_FILE_SIZE);
                }else if (strcmp(commande,"TIMEOUT") == 0 ||
                        strcmp(commande,"timeout") == 0 ||
                        strcmp(commande,"TIME_OUT") == 0 ||
                        strcmp(commande,"time_out") == 0    ){
                            TIMEOUT=atoi(valeur);
                            printf("La durée maximale avant qu'un capteur ne soit en timeout est maintenant de %d secondes. \r\n", TIMEOUT);
                }else if (strcmp(commande,"RESET") == 0 ||
                        strcmp(commande,"WIPE") == 0 ||
                        strcmp(commande,"reset") == 0 ||
                        strcmp(commande,"wipe") == 0    ){
                            printf("Réinitialisation des paramètres...\r\n");
                            LUMIN = true; 
                            LUMIN_LOW = 255;
                            LUMIN_HIGH = 768;
                            TEMP_AIR = true;
                            MIN_TEMP_AIR = -10;
                            MAX_TEMP_AIR = 60; 
                            HYGR = true;
                            HYGR_MINT = 0;
                            HYGR_MAXT = 50;
                            PRESSURE = true; 
                            PRESSURE_MIN = 850;
                            PRESSURE_MAX = 1080;
                            LOG_INTERVAL = 10;
                            MAX_FILE_SIZE = 4096;
                            TIMEOUT = 30;
                            printf("Paramètres réinitialisés ! \r\n");

                        }
                else
                {
                    printf("Commande inconnue : >%s<\r\nSi vous ne vous souvenez pas du nom de la commande,\n veuillez vous référer au manuel d'utilisation\n", commande);
                }
            }
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
        /*détection du bouton vert pour passage en mode économie*/
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

volatile uint8_t rx_data[10];
volatile int rx_status[10];
volatile uint32_t rx_duration[10];

int UART_LireCommande(char *ligne, size_t taille)
{
    uint8_t c;
    size_t pos = 0;

    if (ligne == NULL || taille < 2)
        return 0;

    while (1)
    {
        HAL_StatusTypeDef status =
            HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY);

        if (status != HAL_OK)
            return 0;

        if (c == '\r' || c == '\n')
        {
            if (pos == 0)
                continue;

            ligne[pos] = '\0';
            return 1;
        }

        if (pos < taille - 1)
            ligne[pos++] = (char)c;
        else
        {
            ligne[pos] = '\0';
            return 0;  // Commande trop longue
        }
    }
}


int main(void)
{
    Global_Init();

    /* Appel de la fonction de configuration ici (si appui bouton rouge détecté au démarrage). */
    /* Appel du hub de modes */

    initialisation();
}
