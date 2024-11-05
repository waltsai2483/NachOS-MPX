#include "syscall.h"

int a[1000000] = {1};

int main() {
	int n;
	for (n=9; n>5; n--) {
		PrintInt(n);
	}
	return 0;
	//Halt();
}
