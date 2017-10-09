#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/time.h>

#include <rte_debug.h>
#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_cycles.h>
#include <rte_timer.h>
#include <rte_debug.h>
#include <rte_hash_crc.h>
#include <rte_tailq.h>
#include <rte_malloc.h>
#include <rte_mempool.h>

#define MAX_MONITOR (20)

static FILE *fptr = NULL;
static uint8_t console = 1;
static int8_t count = MAX_MONITOR;
static uint32_t crcValues[MAX_MONITOR];
static struct rte_timer timer[MAX_MONITOR];

typedef struct 
{
  uint8_t index;
  const struct rte_memzone *mzPtr;
} timerArgs_t;
static timerArgs_t timerArgs[MAX_MONITOR];

uint8_t monitor = 1;

void usrEnter (void);
void displayMenu (void); 
static int lcore_main (__attribute__((unused))void *arg);
static void timer_cb(__attribute__((unused)) struct rte_timer *tim, void *arg);

static void timer_cb(__attribute__((unused)) struct rte_timer *tim, void *arg)
{
  const timerArgs_t *timerArgs = (const timerArgs_t *) arg;
  uint32_t temp = 0x0;
  struct timeval tv;

  if (monitor) {
    gettimeofday( &tv, NULL );

    if (timerArgs->mzPtr) {
#if 0
      printf("Memzone details: Name: %s PHY addr: %p Len: %d Huge Page Size: %"PRIu64" add: %p\n",
            timerArgs->mzPtr->name, timerArgs->mzPtr->phys_addr, timerArgs->mzPtr->len, 
            timerArgs->mzPtr->hugepage_sz, timerArgs->mzPtr->addr); 
#endif

      temp = rte_hash_crc(timerArgs->mzPtr->addr, timerArgs->mzPtr->len, 0xBADA);

      if ((crcValues[timerArgs->index]) && (crcValues[timerArgs->index] != temp))
      {
        if ((console == 0) && (fptr))
          fprintf(fptr, "At %f Change for Memmzone Region: %s prev:%u curr:%u\n", 
                tv.tv_sec + ( tv.tv_usec / 1000000.0 ),
                timerArgs->mzPtr->name, crcValues[timerArgs->index], temp);
        else 
          printf("\033[%d;4H%d - At %d.%06d Change for Memmzone Region: %s prev:%u curr:%u\n", 
                timerArgs->index + 11, timerArgs->index,
                (int)tv.tv_sec, (int)tv.tv_usec,
                timerArgs->mzPtr->name, crcValues[timerArgs->index], temp);
      }
    
      crcValues[timerArgs->index] = temp;
    }
  }
  fflush(stdout);
  return;
}

static int lcore_main (__attribute__((unused))void *arg)
{
  printf("Inside %s() on lcore_id: %u\n", __func__, rte_lcore_id());
  fflush(stdout);

  while (1) 
  {
    if (monitor)
      rte_timer_manage();
  }

  return 0;
}

void displayMenu (void) 
{
  printf("\033[2;2H --- MEMZONE Info Menu ---");
  printf("\033[3;4H1) Display Regions");
  printf("\033[4;4H2) Begin Monitor Region");
  printf("\033[5;4H3) Stop Monitor");
  printf("\033[6;4H4) Start Monitor");
  printf("\033[7;4H5) toogle update - monitor.log or console");
  printf("\033[8;4H6) quit");
  printf("\033[9;4H -------------------------");
  printf("\033[10;2HEnter Choice:");

  return;
}

void usrEnter(void)
{
  printf(" \033[78;1H ENTER CHOICE:");
  return;
}


int main(int argc, char **argv)
{
  int ret;
  uint8_t quit = 1, i = 0;
  char region[100] = {0};
  const struct rte_memzone *mzPtr = NULL;
  
  char usrMenu = NULL;
  char c_flag[] = "--lcores";
  char n_flag[] = "(1-2)@(0)";
  char mp_flag[] = "--proc-type=secondary";
  char nopci_flag[] = "--no-pci";
  char nohpet_flag[] = "--no-hpet";
  char *argp[argc + 5];

  argp[0] = argv[0];
  argp[1] = c_flag;
  argp[2] = n_flag;
  argp[3] = mp_flag;
  argp[4] = nopci_flag;
  argp[5] = nohpet_flag;

  argc = (argc == 2)?2:0;

  for (i = 1; i < argc; i++)
    argp[i + 5] = argv[i];

  argc += 5;

  memset(crcValues, 0, sizeof(crcValues));
  memset(timer, 0, sizeof(timer));
  memset(timerArgs, 0, sizeof(timerArgs));

  ret = rte_eal_init(argc, argp);
  if (ret < 0)
    rte_panic("Cannot init EAL\n");

  if (!rte_eal_primary_proc_alive(NULL))
    rte_exit(EXIT_FAILURE, "No primary DPDK process is running.\n");

  rte_timer_subsystem_init();
  rte_hash_crc_set_alg (CRC32_SSE42|CRC32_SSE42_x64);

  if (rte_lcore_count() < 2)
    rte_exit(EXIT_FAILURE, "requires minimum 2 lthread. %d\n", rte_lcore_count());

  fptr = fopen("monitor.log", "a+");
  if (fptr == NULL)
    rte_exit(EXIT_FAILURE, "Failed to open file!!!\n");
  
  for (i = 0; i <MAX_MONITOR; i++)
    rte_timer_init(&(timer[i]));

  /*worker*/
  unsigned lcore_id = rte_get_next_lcore(rte_lcore_id(), 0, 0);
  rte_eal_remote_launch((lcore_function_t *)lcore_main, NULL, lcore_id),

  fflush(stdout);
  printf("\033[2J");
  printf("\033[2J");

  while(quit) 
  {
    displayMenu ();
    usrMenu = getchar();

    switch (usrMenu) {
      case '1':
        rte_memzone_dump (stdout);
        //rte_dump_tailq (stdout);
        //rte_malloc_dump_stats (stdout, NULL);
        //rte_mempool_list_dump (stdout);
        rte_dump_physmem_layout (stdout);
        printf(" Press Key to continue -----\n");
        getchar();
        getchar();
        printf("\033[2J");
        break;

      case '2':
        printf("\nEnter the region to monitor:");
        getchar();
        scanf("%[^\n]%*c", region);
        //fgets (region, 100, stdin);
        region[99] = '\0';

        if (count >= 0) {
          mzPtr =  rte_memzone_lookup ((const char*)region);
          if (mzPtr) {
            count -= 1;
            timerArgs[count].index = count;
            timerArgs[count].mzPtr = mzPtr;

            rte_timer_reset(&timer[count], rte_get_timer_hz(), PERIODICAL, lcore_id, timer_cb, &timerArgs[count]);

            printf("Added Memzone region : %s\n ", region);
          }
          else {
            printf(" Memzone details not found, press key to continue......!!!\n");
          }
          printf(" Press Key to continue -----\n");
        }
        else {
          printf(" There are no slots for monitor, Press Key to continue -----\n");
        }
        getchar();
        printf("\033[2J");
        break;

      case '3':
        monitor = 0;
        printf("\033[10:15H                                        ");
        break;

      case '4':
        monitor = 1;
        printf("\033[10:15H                                        ");
        break;

      case '5':
        console = (console == 1)?0:1;
        printf("\033[10:15H                                        ");
        break;

      case '6':
        quit = 0;
        printf("\033[2J");
        for (i = 0; i < MAX_MONITOR; i++) {
          if (timer[i].f != NULL) {
            printf("\nstopping timer in index %d\n", i);
            rte_timer_stop (&timer[i]);
          }
        }
        fclose(fptr); 
        printf("\033[2J");
        printf("\033[5;1H -------------------------");
        printf("\033[6;4H Stoppping all monitor and exiting!!");
        printf("\033[7;1H -------------------------");
        printf("\n\n");
        break;
    }
  }

  return 0;
}

