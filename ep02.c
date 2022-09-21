// https://cpl.li/posts/2019-03-12-mbrfat/ 
// https://wiki.osdev.org/MBR_%28x86%29

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <math.h>

typedef struct Partitions{
   uint8_t status;
   char chsStart[3]; // 3 bytes
   uint8_t type; 
   char chsEnd[3]; // 3 bytes
   uint32_t lba;
   uint32_t qntSectors;
} partition;

#define READ_BINARY "rb"
#define BOOT_SIGNATURE_INDEX 510
#define FIRST_PARTITION_INDEX 446
#define QUANTITY_OF_PARTITIONS 4



enum errors {OPENING_FILE, MALLOC, READING_FILE, BOOT_SIGNATURE} error;

void copyChs(char * dest, char * src){
   for(int i = 0; i < 3; i++) {
      dest[i] = src[i];
   }
}

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
   printf("Boot signature: %04x\n", bootSignature);

   if(bootSignature != 0xaa55) {
      printf("Error: File without a valid boot signature");
      exit(BOOT_SIGNATURE);
   } 


   int partitionFileIndex = FIRST_PARTITION_INDEX;
   partition * partitions = malloc(QUANTITY_OF_PARTITIONS + sizeof(partition));
   for(int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      partition * pAux = &partitions[i];
      
      uint8_t cursor = 0;
      pAux->status = *(mbrBuffer + partitionFileIndex);
      cursor += sizeof(pAux->status);

      strncat(pAux->chsStart, (mbrBuffer + partitionFileIndex + cursor), 3); 
      cursor += sizeof(pAux->chsStart);

      pAux->type = *(mbrBuffer + partitionFileIndex + cursor); 
      cursor += sizeof(pAux->type);

      strncat(pAux->chsEnd, (mbrBuffer + partitionFileIndex + cursor), 3); 
      cursor += sizeof(pAux->chsEnd);
      
      pAux->lba = *( (uint32_t *) (mbrBuffer + partitionFileIndex + cursor));
      cursor += sizeof(pAux->lba);
      
      pAux->qntSectors = *( (uint32_t *) (mbrBuffer + partitionFileIndex + cursor));

      partitionFileIndex += sizeof(partition);
   }

  fclose(file);

   
   printf("Patition type is %02X check in https://en.wikipedia.org/wiki/Partition_type\n", partitions[0].type);
   printf("Unidades: setor de 1 * 512 = 512 bytes\n");
   
   printf("Dispositivo Inicializar Início    Fim    Setores Tamanho\n");

   char * auxText = malloc(sizeof(char) * 20);
   
   float_t memoryInDisk = 0;

   for(int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      if (partitions[i].lba ) {
         // uint64_t memoryInDisk = round( (partitions[i].lba + partitions[i].qntSectors) );
         printf("/dev/sda%d   ", i + 1);
         if (partitions[i].status == 0x80){
            printf("*           ");
         } else {
            printf("            ");
         }
         printf("%d\t", partitions[i].lba);
         printf("%d", partitions[i].qntSectors + partitions[i].lba - 1);
         printf("%d         ", partitions[i].qntSectors);
         uint64_t qntdSectors = (uint64_t) (partitions[i].qntSectors);
         float_t sectors_to_gb = 512 / pow(1024, 3);
         memoryInDisk += qntdSectors * sectors_to_gb;
         printf("%.1fG        ", qntdSectors * sectors_to_gb);

         printf("\n");
      }
   }

   memoryInDisk = round(memoryInDisk);

   printf("Disco /dev/sda: %.f GiB, %.f bytes, %.f setores\n", memoryInDisk, memoryInDisk * pow(1024, 3), (memoryInDisk * pow(1024, 3)) / 512);
   

  free(mbrBuffer);
  return 0;
}