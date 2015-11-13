
// BIBLIOTHÈQUES
#include <SDL/SDL.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// ADRESSE IP AR.DRONE
#define ADRESSEIP "192.168.1.1"
#define PORT 5556
#define BUFLEN 256

// COMMANDES AT
#define COMMANDE_AT_DECOLLAGE    ",290718208\r" // ENTRER
#define COMMANDE_AT_ATTERISSAGE    ",290717696\r" // BACKSPACE
#define COMMANDE_AT_ARRET_URGENCE    ",290717952\r" // ECHAP
#define COMMANDE_AT_ANTI_ARRET_URGENCE    ",290717696\r" // ECHAP
#define COMMANDE_AT_AVANT    ",1,0,-1082130432,0,0\r" // FLÈCHE HAUT
#define COMMANDE_AT_ARRIERE  ",1,0,1065353216,0,0\r" // FLÈCHE BAS
#define COMMANDE_AT_GAUCHE   ",1,-1082130432,0,0,0\r" // FLÈCHE GAUCHE
#define COMMANDE_AT_DROITE   ",1,1065353216,0,0,0\r" // FLÈCHE DROITE
#define COMMANDE_AT_HAUT ",1,0,0,1065353216,0\r" // TOUCHE "Z"
#define COMMANDE_AT_BAS  ",1,0,0,-1082130432,0\r" // TOUCHE "S"
#define COMMANDE_AT_ROTATION_GAUCHE  ",1,0,0,0,-1082130432\r" // TOUCHE "Q"
#define COMMANDE_AT_ROTATION_DROITE  ",1,0,0,0,1065353216\r" // TOUCHE "D"

#define DEBUG

// PROTOTYPES DES FONCTIONS
void err(char *s);
void envoyer_AT(char* commande, unsigned int compteur, char* argument);

// VARIABLES GLOBALES
struct sockaddr_in serv_addr;
int sockfd, slen;

// FONCTION MAIN
int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        fprintf(stderr, "Erreur d'initialisation de la SDL : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    slen=sizeof(serv_addr);
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        err("socket");
    }
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_aton(ADRESSEIP, &serv_addr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() erreur\n");
        exit(1);
    }
    
    int boucle = 1;
    int status = 0; // Status = 0 : Le drone est au sol
    int urgence = 0; // Mode urgence désactivé
    unsigned int compteur = 0; // Initialisation du compteur
    SDL_Event event;
    while(boucle)
    {
        SDL_PollEvent(&event);
        switch(event.type)
        {
            case SDL_QUIT:
                boucle = 0; // On quitte la boucle
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_p:
                        boucle = 0;
                        break;
                    
                    case SDLK_BACKSPACE:
                        if (urgence==0 && status==1)
                        {
                            #ifdef DEBUG
                            printf("\nAtterissage !\n");
                            #endif
                            compteur++;
                            envoyer_AT("AT*REF=",compteur,COMMANDE_AT_ATTERISSAGE);
                            status = 0; // Status = 0 : Le drone est au sol
                            sleep(2); // Atterissage
                        }
                        break;
                        
                    case SDLK_RETURN:
                        if (urgence==0 && status==0)
                        {
                            #ifdef DEBUG
                            printf("\nDécollage !\n");
                            #endif
                            compteur++;
                            envoyer_AT("AT*REF=",compteur,COMMANDE_AT_DECOLLAGE);
                            status = 1; // Status = 0 : Le drone est en vol
                            sleep(2); // Décollage
                        }
                        break;
                        
                    case SDLK_ESCAPE:
                        if (urgence==0 && status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("ARRET_URGENCE\n");
                            #endif
                            envoyer_AT("AT*REF=",compteur,COMMANDE_AT_ARRET_URGENCE);
                            sleep(1); // Mode d'urgence
                            status = 0;
                            urgence = 1;
                        }
                        else if (urgence==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("ANTI_ARRET_URGENCE\n");
                            #endif
                            envoyer_AT("AT*REF=",compteur,COMMANDE_AT_ANTI_ARRET_URGENCE);
                            urgence = 0;
                        }
                        break;
                        
                    case SDLK_UP:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("AVANT\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_AVANT);
                        }
                        break;
                        
                    case SDLK_DOWN:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("ARRIERE\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_ARRIERE);
                        }
                        break;
                        
                    case SDLK_LEFT:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("GAUCHE\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_GAUCHE);
                        }
                        break;
                        
                    case SDLK_RIGHT:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("DROITE\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_DROITE);
                        }
                        break;
                        
                    case SDLK_z:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("HAUT\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_HAUT);
                        }
                        break;
                        
                    case SDLK_s:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("BAS\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_BAS);
                        }
                        break;
                        
                    case SDLK_q:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("ROTATION_GAUCHE\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_ROTATION_GAUCHE);
                        }
                        break;
                        
                    case SDLK_d:
                        if (status==1)
                        {
                            compteur++;
                            #ifdef DEBUG
                            printf("ROTATION_DROITE\n");
                            #endif
                            envoyer_AT("AT*PCMD=",compteur,COMMANDE_AT_ROTATION_DROITE);
                        }
                        break;
                }
                break;
        }
        usleep(60000);
    }
    
    close(sockfd);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}

// DÉFINITION DES FONCTIONS

void err(char *signal)
{
    perror(signal);
    //exit(1); // Décommentez pour quitter le programme en cas d'erreur
}

void envoyer_AT(char* commande, unsigned int compteur, char* argument)
{
    char sequence_number[20];
    char message[100];
    sprintf(sequence_number,"%d",compteur);
    strcpy(message,commande);
    strcat(message,sequence_number);
    strcat(message,argument);
    #ifdef DEBUG
    printf("%d\n",message);
    #endif
    if (sendto(sockfd, message, BUFLEN, 0, (struct sockaddr*)&serv_addr, slen)==-1)
    {
        err("sendto()");
    }
    usleep(50000); // Délai (approximatif) entre deux commandes AT
}
