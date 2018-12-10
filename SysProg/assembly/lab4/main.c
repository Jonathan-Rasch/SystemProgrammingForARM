void asm_main(void);

// This 'dummy' main() function ensures that RAM
// is correctly initialised.  Your code should all
// be in asm_main.s.
int main(void) {
	asm_main();
	return 0;
}
