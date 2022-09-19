// https://cpl.li/posts/2019-03-12-mbrfat/ 
// https://wiki.osdev.org/MBR_%28x86%29

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define READ_BINARY "rb"
#define BOOT_SIGNATURE_INDEX 510

enum errors {OPENING_FILE, MALLOC, READING_FILE, BOOT_SIGNATURE} error;

int main () {
  FILE * file = fopen("mbr.bin", READ_BINARY);
  if(file == NULL) {
      printf("Error: Opening file");
      exit(OPENING_FILE);
   }

  fseek(file , 0 , SEEK_END);
  uint16_t fileSize = ftell(file);
  rewind(file);

  char * mbrBuffer = (char*) malloc(sizeof(char)*fileSize);
  if(mbrBuffer == NULL) {
      printf("Error: Malloc return null");
      exit(MALLOC);
   }

  size_t resultSize = fread(mbrBuffer, 1, fileSize, file);
  if(resultSize != fileSize) {
      printf("Error: Reading file");
      exit(READING_FILE);
   }

   uint16_t bootSignature = *( (uint16_t *) &mbrBuffer[BOOT_SIGNATURE_INDEX] ); 
   printf("Boot signature: %04x", bootSignature);

   if(bootSignature != 0xaa55) {
      printf("Error: File without a valid boot signature");
      exit(BOOT_SIGNATURE);
   } 

  fclose(file);
  free(mbrBuffer);
  return 0;
}