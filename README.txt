This is a README file for TCPProxy.c

compilation of program:
	    ">make" will compile and link all relevant files (provided they are in the same directory)
	    ">make clean" will remove all .o and executable files
	    ">make run" will compile, link and execute the program

	Need to change the proxy settings for Firefox so that it uses ./myProxy
	Open Firefox
	Go to "hamburger" menu -> Options -> "Network Proxy" Settings...
	Select Manual proxy configuration
	Terminal Side:
		  type "ifconfig" to get your network settings
	 	  under "wifi0" copy the IP address immediately following "inet addr:"
		  paste this address into the HTTP Proxy bar
	This Proxy uses port 6666, enter this number into the Port bar
	run program using command "./myProxy" in the terminal
	please note, the proxy will only listen for 10 seconds (while not receiving anything)
	       before deciding to shut down (it will shut down on its own, no ctrl-C needed!)
	load web pages as normal
	enjoy the spelling errors!!!

Proxy does not do images!
I'm pretty certain I handled the Not Modified and Partial Content "Errors"
>90% of testing was done on my home laptop, I have cleared the cache many times
     so that nothing sneaky got by 
