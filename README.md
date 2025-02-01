# Proxy-FTP

Ce projet consiste à établir un proxy pour pouvoir utiliser les commandes `CD` et `LS` à l'aide de la commande suivante :  
```sh
ftp -z nossl -d <@IP-du-proxy> <n° port>
```
# Description
Le Proxy-FTP permet de se connecter à un serveur FTP et d'exécuter des commandes FTP via un proxy intermédiaire.
Le proxy intercepte les commandes CD et LS et les traite de manière appropriée.

# Fonctionnalités
- Établir une connexion proxy entre le client FTP et le serveur FTP.
- Intercepter et traiter les commandes CD et LS.
- Support des connexions passives et actives.
- Gérer les erreurs et les connexions multiples via le forking.

# Pré-requis
- Un système basé sur UNIX (Linux, macOS, etc.).
- Un compilateur C (comme gcc).
- Les bibliothèques de développement pour les sockets réseau.

# Installation
1. Clonez le dépôt :
```sh
git clone https://github.com/Arcols/Proxy-FTP.git
```
2. Compliez le projet :
```sh
cd Proxy-FTP
make
```

# Utilisation
1. Lancez le proxy : 
```sh
./proxy
```
Le proxy affichera l'adresse et le port sur lesquels il écoute.

2. Connectez-vous au proxy via un client FTP avec la commande suivante :
```sh
ftp -z nossl -d <@IP-du-proxy> <n° port>
```

# Exemple
Supposons que le proxy écoute sur l'adresse 127.0.0.1 et le port 2121, vous pouvez vous connecter en utilisant :
```sh
ftp -z nossl -d 127.0.0.1 2121
```
# Structure du code
proxy.c : Contient le code principal du proxy, y compris la gestion des connexions et le traitement des commandes FTP.
simpleSocketAPI.h : Définition des fonctions d'aide pour la gestion des sockets.
Contributions
Les contributions sont les bienvenues ! Veuillez soumettre une issue ou une pull request pour toute amélioration ou correction de bug.

# Licence
Ce projet est sous licence MIT. Veuillez consulter le fichier LICENSE pour plus de détails.
