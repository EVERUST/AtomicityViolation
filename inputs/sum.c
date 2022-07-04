#include <stdio.h>

int main() {
	int n=0;
	int sum=0;

	while(fscanf(stdin, "%d", &n) >= 1)
		sum += n;

	fprintf(stdout, "%d\n", sum);
}

void foo() {}
void bar() {}
