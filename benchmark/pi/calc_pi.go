package main

import (
	"fmt"
	"math/rand"
	"time"
)

func estimatePi(numPoints int) float64 {
	x, y, rSquared, withinCircle := 0.0, 0.0, 0.0, 0
	for i := 0; i < numPoints; i++ {
		x = rand.Float64()
		y = rand.Float64()

		rSquared = x*x + y*y
		if rSquared <= 1 {
			withinCircle++
		}
	}

	pi := float64(withinCircle) / float64(numPoints) * 4
	return pi
}

func main() {
	avgTime := 0.0

	for i := 0; i < 10; i++ {
		start := time.Now().UnixMilli()
		pi := estimatePi(10_000_000)
		stop := time.Now().UnixMilli()
		time := stop - start
		avgTime += float64(time) / 1000
		fmt.Println("pi is", pi, "took", float64(time)/1000, "seconds")
	}

	fmt.Println("On average pi took", avgTime/10, "seconds to compute.")
}
