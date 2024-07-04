package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

// -------------------------------------------------------------------------------- //

type Semaphore struct {
	wait  uint
	mutex *sync.Mutex
	cond  *sync.Cond
}

// -------------------------------------------------------------------------------- //

func main() {
	fmt.Print("Start go semaphore\n")
	sem_tx := newSemaphore(1)
	sem_rx := newSemaphore(0)
	sheared := uint(0)

	wg := sync.WaitGroup{}
	wg.Add(2)
	go transmitter(sem_tx, sem_rx, &sheared, &wg)
	go receiver(sem_tx, sem_rx, &sheared, &wg)
	wg.Wait()

	fmt.Printf("End go semaphore\n")
}

// -------------------------------------------------------------------------------- //

func newSemaphore(n uint) (sem *Semaphore) {
	mutex := &sync.Mutex{}
	sem = &Semaphore{
		wait:  n,
		mutex: mutex,
		cond:  sync.NewCond(mutex),
	}
	return
}

// -------------------------------------------------------------------------------- //

func (sem *Semaphore) acquire() {
	sem.mutex.Lock()
	defer sem.mutex.Unlock()
	for sem.wait == 0 {
		sem.cond.Wait()
	}
	sem.wait--
}

// -------------------------------------------------------------------------------- //

func (sem *Semaphore) release() {
	sem.mutex.Lock()
	defer sem.mutex.Unlock()
	sem.wait++
	sem.cond.Signal()
}

// -------------------------------------------------------------------------------- //

func transmitter(sem_tx, sem_rx *Semaphore, sheared *uint, wg *sync.WaitGroup) {
	fmt.Printf("Starting transmitter\n")
	for i := 1; i <= 10; i++ {
		sem_tx.acquire()
		*sheared = uint(i + 410)
		fmt.Printf("Transmitter: %d (%d)\n", *sheared, i)
		time.Sleep(time.Duration(rand.Int63n(1000)) * time.Millisecond)
		sem_rx.release()
	}
	wg.Done()
}

// -------------------------------------------------------------------------------- //

func receiver(sem_tx, sem_rx *Semaphore, sheared *uint, wg *sync.WaitGroup) {
	fmt.Printf("Starting receiver\n")
	for i := 1; i <= 10; i++ {
		sem_rx.acquire()
		fmt.Printf("\t\t\tReceiver: %d (%d)\n", *sheared, i)
		time.Sleep(time.Duration(rand.Int63n(1000)) * time.Millisecond)
		sem_tx.release()
	}
	wg.Done()
}

// -------------------------------------------------------------------------------- //
