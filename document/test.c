#include<stdio.h>

int main()
{
	int a = 0;
	a = ({a++;a++;});
	printf("%d\n", a);
	return 0;
}
