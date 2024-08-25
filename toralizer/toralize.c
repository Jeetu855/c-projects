/* toralize.c */

#include "toralize.h"

/*

./toralize 1.2.3.4 8080
this tool will connect to a predefined proxy server

*/

int main(int argc, char *argv[]) {
  char *hostname; // hostname we want to connect to
  int port;

  if (argc < 3) { // If we dont have 2 arguments specified for IP and port
    fprintf(stderr, "Usage : %s <host> <port>\n",
            argv[0]); // argv[0] is the name of the file i.e ./toralize.c
    return -1;
  }
  hostname = argv[1];
  port = atoi(
      argv[2]); // argv[i] is a string so we can use it normally like above
                // atoi converts a number in string format to a regular number
}
