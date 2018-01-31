extern errno_t putchar(char);

#define TERMINATOR '!'

int main(void) {
	/* Prints "hello" to the standard output. */
	putchar('h');
	putchar('e');
	putchar('l');
	putchar('l');
	putchar('o');
	putchar(TERMINATOR);
	putchar('\n');
	return 0;
}

