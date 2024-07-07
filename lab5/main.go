package main

import (
	"fmt"
	"sync"
)

// -------------------------------------------------------------------------------- //

type Buffer struct {
	element_A      uint16
	element_B      uint16
	elements_count uint16
	input          chan uint16
	output         chan uint16
}

// -------------------------------------------------------------------------------- //

func main() {
	fmt.Print("Start lab5 \n")
	wg := sync.WaitGroup{}
	buffer := buffer_init()

	wg.Add(2)
	go producer(buffer, &wg)
	go consumer(buffer, &wg)
	wg.Wait()

	fmt.Printf("End lab5\n")
}

// -------------------------------------------------------------------------------- //

func buffer_init() *Buffer {
	buffer := new(Buffer)

	buffer.elements_count = 0
	buffer.input = make(chan uint16)
	buffer.output = make(chan uint16)

	go buffer_worker(buffer)

	return buffer
}

func buffer_worker(buffer *Buffer) {
	for {
		switch buffer.elements_count {
		case 0:
			buffer.element_A = <-buffer.input
			println("\t\t\t\t\t\t\t\tBuffer in: ", buffer.element_A)
			buffer.elements_count++
		case 1:
			select {
			case buffer.element_B = <-buffer.input:
				println("\t\t\t\t\t\t\t\tBuffer in: ", buffer.element_B)
				buffer.elements_count++
			case buffer.output <- buffer.element_A:
				println("\t\t\t\t\t\t\t\tBuffer out: ", buffer.element_A)
				buffer.elements_count--
			}
		case 2:
			buffer.output <- buffer.element_A
			println("\t\t\t\t\t\t\t\tBuffer out: ", buffer.element_A)
			buffer.element_A = buffer.element_B
			buffer.elements_count--
		default:
			println("Buffer overflow")
		}
	}
}

// -------------------------------------------------------------------------------- //

func producer(buffer *Buffer, wg *sync.WaitGroup) {
	var val uint16 = 0

	for i := uint16(0); i < 10; i++ {
		val = i + 430
		println("Producing ", val)
		buffer.input <- val
	}

	defer wg.Done()
}

// -------------------------------------------------------------------------------- //

func consumer(buffer *Buffer, wg *sync.WaitGroup) {
	for i := uint16(0); i < 10; i++ {
		println("\t\t\t\tConsuming ", <-buffer.output)
	}

	defer wg.Done()
}
