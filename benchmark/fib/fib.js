function fib(n) {
	if (n < 2) {
		return n
	}
    
	return fib(n - 2) + fib(n - 1)
}

for (var i = 0; i < 40; i += 1) {
	fib(i)
}
