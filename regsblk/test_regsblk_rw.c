//----------------------------------------------------------------------------------------------
//
// Author   : Nahal Fadaei
// Function : A simple test application for regsblk device by reading/writing into registers
// Examples :
//          to read from register 0x1 run : ./test_regsblk_rw -a 0x1 
//          to write 0xDEADBEEF into register 0x4 run : ./test_regsblk_rw -a 0x4 -v 0xDEADBEEF
//----------------------------------------------------------------------------------------------

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  // Parse command-line options
  int opt;
  int flagA = 0, flagW = 0;
  uint32_t addr, value;

  while ((opt = getopt(argc, argv, "ha:v:")) != -1) 
  {
    switch (opt) 
    {
      case 'h':
        printf("Usage: %s [-h] -a address [-v value] \n", argv[0]);
        printf("  -h           Display this help message\n");
        printf("  -a address   Specify the regsiter address in hex\n");
        printf("  -v value     Specify the regsiter value (if -v is present write operation is performed, otherwise a read operation is performed)\n");
        exit(EXIT_SUCCESS);
      case 'a':
        addr = strtol(optarg,0,16);
        flagA = 1;
        break;
      case 'v':
        value = strtol(optarg,0,16);
        flagW = 1;
        break;
      case '?':
        fprintf(stderr, "Unknown option: %c\n", optopt);
        exit(EXIT_FAILURE);
     default:
        fprintf(stderr, "Error parsing options\n");
        exit(EXIT_FAILURE);
    }
  }

  if(!flagA) {
    fprintf(stderr, "missing flag -a: Unknown address\n");
    exit(EXIT_FAILURE);
  }

  int fd = open("/dev/regsblk", O_RDWR);
  if (fd < 0) {
      perror("open");
      return 1;
  }

  if(lseek(fd, addr * sizeof(uint32_t), SEEK_SET) < 0) {
      fprintf(stderr, "out of range address 0x%08x\n", addr);
      perror("lseek");
      exit(EXIT_FAILURE); 
  }

  if(!flagW) {
    //register read

    read(fd, &value, sizeof(value));
    printf("Reading Register 0x%08x: 0x%08x\n", addr, value);

  } else {
    //register write
    
    write(fd, &value, sizeof(value));
    printf("Writing Register 0x%08x: 0x%08x\n", addr, value);
  }

  close(fd);

  return 0;
}
