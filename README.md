# My Wonderful LINFO1341 Project
# Comment tester le code: 
make tests
OU si il y a la moindre erreur
dans le dossier src/ 
gcc -o receiver receiver.c -lz
gcc -o sender sender.c -lz
./receiver ::1 12345 &
./sender ::1 12345 -f test.txt
 ___________________________________________

 # Mais au fond qu'est ce qui fonctionne ?

Le système de packet (la structure des packets) fonctionne (normalement) parfaitement.
La connection entre le sender et receiver fonctionne (normalement) de manière bilatérale
    => Le sender peut écrire et lire sur le socket ainsi que le receiver.
Le sender sait envoyer les packets de type pkt_t au receiver, le receiver sait le récupérer, le décoder et afficher le payload.
Le sender attend le signal ackitement après avoir envoyé toute sa window pour pouvoir réenvoyer une nouvelle window.

# Mais alors, qu'est ce qui ne fonctionne pas ?

Le système de LOG n'est pas encore implémenté
Le sytème de NACK ainsi que FEC n'est pas encore implémenté
Le receiver ne garde pas en mémoire les payloads pendant le transfert
Le signal ackitement est envoyé tous les packets reçu => Implémentation pour l'envoyer toutes les windows
Tous les cas de truncation ne sont pas encore pris en compte

# Comment est-ce qu'on va s'y prendre ?

1) Rendre fonctionnel le système suivant: 
    => Le sender envoit un premier packet, receiver renvoit ack/nack avec la window.
    Ensuite, le sender envoit à chaque fois valeur_de_window packets avant de reçevoir 1 seul signal d'ack/nack pour valider la window entière. (pour l'instant 1 packet send, 1 ack reçu, 1 packet send,...)
2) Créer un buffer/fichier pour stocker tous les payload valides du receiver.
3) Gérer tous les cas de truncation possibles.
4) Rendre fonctionnel le système de LOG
5) Implémenter FEC
6) (ou 4b) Finir le rapport