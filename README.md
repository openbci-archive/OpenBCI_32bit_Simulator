# OpenBCI_32bit_Simulator
A full blown simulator for the OpenBCI 32bit board.
INSTRUCTIONS:

1. Make the kernel module (board.c)
2. run ./runme.sh to insert the kernel module (Requires root access)
3. make the user-level code (board_driver.c)
4. Valid Uses: ./board_driver [no_of_iterations] [test_mode_enable]
	./board_driver		#will run the code with default no. of iterations(=1) for streaming loop. 
	./board_driver 100 	#will run 100 iterations of the streaming loop. 
	./board_driver 30 1 	#will run 30 iterations of the streaming loop in TEST MODE.

FEATURES:
1. Streaming mode (b). 
2. Reset (v) sends device info followed by $$$. 
3. Channels can be selectively turned on/off by sending 12345678/!@#$%^&*
4. Channels can be set to default state by writing 'd'
5. Writing 'D' returns 6 ASCII characters followed by $$$. 
More stuff coming soon!
