# CDV Concurrent Programming Course
## Semester 4 (2024)

## Lab 1
Instrukcja fork oraz implementacja pamięci współdzielonej

## Lab 2

### lab2_001.c
Procesy ciężkie i semafory

### lab2_002.c
Procesy lekkie i semafory

### lab2_003.c
Rodzaje zmiennych współdzielonych przez wątki

## Lab 3

### lab3_001.c
Mutexy

### lab3_002.c
Mutexy i zmienne warunkowe

### lab3_003.c
Mutexy i zmienne warunkwe wielu producentów i konsumentów

## Lab 4

### lab4_001.go
Semafory w Go

## Zaliczenie
Problem synchronizacji

### Synteza wody
- Dostawcy wodoru oraz tlenu umieszczają w swoich jednoelementowych
buforach (w losowych odstępach czasu) atom wodoru lub atom tlenu. 
- Gdy wśród dostarczonych atomów znajdą się dwa wodoru i jeden tlenu,
powstaje woda a ich dostawcy wracają do pracy. 
- Problem polega na zsynchronizowaniu pracy dostawców.

### synteza_wody_sem.c
Realizacja za pomocą semaforach na cięzkich procesach

### synteza_wody_mutex.c
Realizacja za pomocą mutexów i zmiennych warunkowych na lekkich procesach
