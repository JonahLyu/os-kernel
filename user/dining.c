#include "P3.h"
#define P_NUM 16

bool forks[P_NUM];
int waiter;
int philo_pid[P_NUM];
int philo_priority[P_NUM];
char s[3];

typedef struct Philosophers{
  int pid;
  int id;
} phil;

int getPipe(int pid1, int pid2){
  if (open_pipe(pid1, pid2) < 0) { //some philosopers exit
    close_pipe(pid1, pid2);
    close_pipe(pid2, pid1);
    write(STDOUT_FILENO, "philosoper exit\n", 16);
    exit( EXIT_SUCCESS ); 
  }
  else return open_pipe(pid1, pid2);
}

void putDown(phil this_p){
  write(STDOUT_FILENO, "philosoper ", 11);
  itoa(s, this_p.id);
  if (this_p.id < 10) write(STDOUT_FILENO, s, 1);
  else write(STDOUT_FILENO, s, 2);
  write(STDOUT_FILENO, " puts down forks\n", 17);
}

void thinking(phil this_p){
  write(STDOUT_FILENO, "philosoper ", 11);
  itoa(s, this_p.id);
  if (this_p.id < 10) write(STDOUT_FILENO, s, 1);
  else write(STDOUT_FILENO, s, 2);
  write(STDOUT_FILENO, " is thinking\n", 13);
}

void eating(phil this_p){
  write(STDOUT_FILENO, "philosoper ", 11);
  itoa(s, this_p.id);
  if (this_p.id < 10) write(STDOUT_FILENO, s, 1);
  else write(STDOUT_FILENO, s, 2);
  write(STDOUT_FILENO, " is eating\n", 11);
}

void requesting(phil this_p){
  
  write(STDOUT_FILENO, "philosoper ", 11);
  itoa(s, this_p.id);
  if (this_p.id < 10) write(STDOUT_FILENO, s, 1);
  else write(STDOUT_FILENO, s, 2);
  write(STDOUT_FILENO, " is requesting\n", 15);
  
  int r = -1;
  write_pipe(getPipe(this_p.pid, waiter), this_p.id);
  while(r < 0){
    r = read_pipe(getPipe(waiter, this_p.pid));
  }
  if (r == 0) eating(this_p);
  else thinking(this_p);
}

void waiterAction(){  
  int r;
  for (int i = 0; i < P_NUM; i++){
    philo_priority[i] = 0;                //iniatilize all prioirty
    create_pipe(philo_pid[i], waiter);    //create pipe to all philosopers
    create_pipe(waiter, philo_pid[i]);
  }
  while(1){
    int highest_p = 0;
    int index;
    //reset all forks and find the highest prioirty
    for (int i = 0; i < P_NUM; i++){
      forks[i] = true;  
      if (philo_priority[i] > highest_p){
        highest_p = philo_priority[i];
        index = i;
      }
      
      write_pipe(getPipe(waiter, philo_pid[i]), 1);  //let all philosopers put down forks
    }
    
    int i = index;  //loop starting from the highest prioirty
    while(1){       //loop all philosopers to respond their requests
      r = -1;
      while(r < 0){
        r = read_pipe(getPipe(philo_pid[i], waiter));  //recieve request from philosoper
      }  
  
      write(STDOUT_FILENO, "waiter responds to:", 19);
      itoa(s, i);
      if (i < 10) write(STDOUT_FILENO, s, 1);
      else write(STDOUT_FILENO, s, 2);
      write(STDOUT_FILENO, "\n", 1);
      
      if (forks[r] && forks[(r + 1)%P_NUM]) {     //check if both two forks are available
        forks[r] = false;
        forks[(r + 1)%P_NUM] = false;
        write_pipe(getPipe(waiter, philo_pid[i]), 0);  //send eating signal to this philosoper
        philo_priority[i] = 0;
      }
      else {
        write_pipe(getPipe(waiter, philo_pid[i]), 1);  //send thinking signal to this philosoper
        philo_priority[i] ++;
      }
      i = (i + 1) % P_NUM;     //loop counter plus 
      if (i == index) break;  
    }
    yield();
    
  }
    
}

void main_Dining() {
  waiter = get_pid();         //get waiter's pid
  for (int i = 0; i < P_NUM; i++){
    int pid = fork();
    if (pid != 0) {          //waiter's behaviour
      philo_pid[i] = pid;    //store all philosopers' pid
      continue;
    }
    else{    //philosopers' behaviour
      phil this_p = (phil) {get_pid(), i};
      while(1){
        int r = -1;
        while(r < 0){
          r = read_pipe(getPipe(waiter, this_p.pid));  //recieve next round signal from waiter
        }
        if (r == 1) putDown(this_p);    //put down forks before each round
        requesting(this_p);             //send a request to waiter
      }
    }  
  }
  
  waiterAction();    
  exit( EXIT_SUCCESS );
}