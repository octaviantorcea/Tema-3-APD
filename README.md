# Tema 3 APD 2022- MPI

Torcea Octavian 334CA

Fiecare proces (coordonatori si workeri) memoreaza topologia intr-o matrice,
in care pe prima linie se gasesc indicii proceselor worker asignate primului
coordonator, pe a doua linie, indicii proceselor worker asignate celui de al
doilea coordonator si pe a treia, indicii proceselor worker asignate celui de al
treilea coordonator (folosesc si un vector ce memoreaza numarul de procese
worker pentru fiecare coordonator).

# Stabilirea topologiei
* La inceput, fiecare coordonator citeste din fisierul de intrare nr de
procese worker pe care le are in subordine, apoi citeste indicii proceselor
worker. Tot acum, coordonatorii semnalizeaza workerilor proprii cine este
coordonatorul.
* Apoi, primul coordonator trimite catre ceilalti coordonatori si catre 
propriile procele worker numarul de procese worker pe care le are in subordine
si indicii proceselor worker. Ceilalti coordonatori asteapta datele si le trimit
apoi catre propriile procese worker.
* Acest proces se repeta si pentru cel de-al doilea si cel de-al treilea
coordonator.

# Realizarea calculelor
* Primul coordonator genereaza vectorul, apoi il imparte in fragmente,
proportionale cu numarul de workeri, fiecarui coordonator (inclusiv sie insusi).
Apoi, fiecare coordonator imparte fragmentul propriu in mod egal si trimite 
partile catre procesle worker.
* Dupa ce procesele worker termina de calculat, acestea trimit inapoi
fragmentele catre coordonatori, apoi coordonatorii catre primul coordonator.
