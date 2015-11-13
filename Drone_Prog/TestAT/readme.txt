le programme clientudp permet de communiquer avec le drone en envoyant directement les commandes AT dans les paquets udp.
Compilation : "gcc clientudp.c -o nom_exe -lm"
utilisation : "./nom_exe <Adresse IP du serveur> <Port> <Commande AT>"
exemple : decoller -> "./nom_exe 192.168.1.1 5556 AT*REF=101,290718208"

liste des commandes sur http://www.upsilonaudio.com/ar-drone-recapitulatif-des-commandes-at/
