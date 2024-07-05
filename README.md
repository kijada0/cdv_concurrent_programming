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

## Problem synchronizacji - /problemy_synch

### Synteza wody - synteza_wody_sem.c
Realizacja za pomocą semaforów na cieżkich procesach

- Dostawcy wodoru oraz tlenu umieszczają w swoich jednoelementowych buforach (w losowych odstępach czasu) atom wodoru lub atom tlenu. 
- Gdy wśród dostarczonych atomów znajdą się dwa wodoru i jeden tlenu, powstaje woda a ich dostawcy wracają do pracy. 
- Problem polega na zsynchronizowaniu pracy dostawców.

### Świety Mikołaj - santa_claus_mutex.c
Realizacja za pomocą mutexów i zmiennych warunkowych na lekkich procesach

- Święty Mikołaj śpi w swojej chatce na biegunie północnym. Może go zbudzić jedynie przybycie dziewięciu reniferów lub trzy spośród dziesięciu skrzatów, chcących poinformować Mikołaja o problemach z produkcją zabawek (snu Mikołaja nie może przerwać mniej niż dziewięć reniferów ani mniej niż trzy skrzaty!). 
- Gdy zbiorą się wszystkie renifery, Mikołaj zaprzęga je do sań, dostarcza zabawki grzecznym dzieciom (oraz studentom realizującym pilnie zadania z synchronizacji współbieżnych procesów), wyprzęga je i pozwala odejść na spoczynek. 
- Mikołaj zbudzony przez skrzaty wprowadza je do biura, udziela konsultacji a później żegna. Obsługa reniferów ma wyższy priorytet niż obsługa skrzatów. 
- Problem polega na synchronizacji działań Św. Mikołaja, reniferów i skrzatów.
