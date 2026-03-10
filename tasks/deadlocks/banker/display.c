#include "display.h"
#include <stdio.h>

/* ANSI colour codes */
#define COL_RESET  "\033[0m"
#define COL_BOLD   "\033[1m"
#define COL_CYAN   "\033[36m"
#define COL_GREEN  "\033[32m"
#define COL_RED    "\033[31m"
#define COL_YELLOW "\033[33m"
#define COL_BLUE   "\033[34m"

/* */
static void
print_row_vector(const int *v,
		 int len)
{
  printf("[ ");
  for (int j = 0; j < len; j++)
    printf("%2d ", v[j]);
  printf("]");
}

/* */
static void
print_divider(int width)
{
  for (int i = 0; i < width; i++) putchar('-');
  putchar('\n');
}

/* */
void
print_vector(const char *label,
	     const int *vec,
	     int len)
{
    printf("  %-14s: ", label);
    print_row_vector(vec, len);
    putchar('\n');
}

/* */
void print_safe_sequence(const int *seq,
			 int n)
{
  printf(COL_GREEN COL_BOLD "  Safe sequence : <" COL_RESET);
  for (int i = 0; i < n; i++) {
    printf(COL_GREEN " T%d" COL_RESET, seq[i]);
    if (i < n - 1) printf(",");
  }
  printf(COL_GREEN " >" COL_RESET "\n");
}

/* */
void
print_state(const BankerState *state)
{
  const int n = state->n_threads;
  const int m = state->n_resources;
  
  //Dynamic column width: 3 chars per resource + brackets */
  int col_w = m * 3 + 4;
  int total  = 8 + col_w * 3 + col_w + 6;
  
  print_divider(total);
  printf(COL_BOLD COL_CYAN
	 "  Thread | %-*s| %-*s| %-*s| %-*s\n" COL_RESET,
	 col_w, " Allocation",
	 col_w, " Max",
	 col_w, " Need",
	 col_w, " Available");
  print_divider(total);
  
  for (int i = 0; i < n; i++) {
    printf("   T%-3d  | ", i);
    print_row_vector(state->allocation[i], m);
    printf(" | ");
    print_row_vector(state->max[i], m);
    printf(" | ");
    print_row_vector(state->need[i], m);
    
    //Print Available only on the first row
    if (i == 0) {
      printf(" | ");
      print_row_vector(state->available, m);
    }
    putchar('\n');
  }
  print_divider(total);
}

/* */
void
print_request_result(int thread_id,
		     const int *request,
		     int len,
		     BankerStatus status)
{
  printf("\n");
  print_divider(50);
  printf(COL_BOLD "  Request for T%d : " COL_RESET, thread_id);
  print_row_vector(request, len);
  putchar('\n');
  
  switch (status) {
  case BANKER_OK:
    printf(COL_GREEN COL_BOLD
	   "  Result  : GRANTED — state remains safe\n"
	   COL_RESET);
    break;
  case BANKER_ERR_EXCEEDS_MAX:
    printf(COL_RED COL_BOLD
	   "  Result  : DENIED  — request exceeds declared maximum\n"
	   COL_RESET);
    break;
  case BANKER_ERR_UNAVAILABLE:
    printf(COL_YELLOW COL_BOLD
	   "  Result  : WAITING — insufficient resources available\n"
	   COL_RESET);
    break;
  case BANKER_ERR_UNSAFE:
    printf(COL_RED COL_BOLD
	   "  Result  : DENIED  — granting would cause unsafe state\n"
	   COL_RESET);
    break;
  default:
    printf(COL_RED "  Result  : ERROR — invalid arguments\n" COL_RESET);
    break;
  }
  print_divider(50);
}
