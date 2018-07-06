
/*
 * Simple example, taken from first KLEE tutorial (without main)
 */


int get_sign(int x)
{
	if(x == 0)
		return 0;
	if(x < 0)
		return -1;
	else
		return 1;
}
