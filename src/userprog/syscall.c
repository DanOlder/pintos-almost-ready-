#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"



static void syscall_handler (struct intr_frame *);

int exit_status_old;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void end_of_proc(int exit_status){
	//

	exit_status_old = exit_status;//temp

	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, exit_status);
    thread_exit();
}

void check_pntr(const void *ptr){

	if(!is_user_vaddr(ptr)){
		end_of_proc(-1);
	}

}






static void
syscall_handler (struct intr_frame *f) 
{
	int stat = *(int*)f->esp;
	int args[3];

	check_pntr((const void*)f->esp);

	switch(stat){

		case SYS_WRITE: {
			putbuf( ((const char**) f->esp)[2], ((size_t*) f->esp)[3]); 
			break;
		}

		case SYS_HALT: {
			shutdown_power_off(); 
			break;
		}

		case SYS_EXIT: {
			int *temp;
			temp = (int *)f->esp + 1;
			check_pntr((const void *)temp);
			args[0] = *temp;
			end_of_proc(args[0]);

		case SYS_EXEC:{
			
		};


		case SYS_WAIT:;
		case SYS_CREATE:;
		case SYS_REMOVE:; 
		case SYS_OPEN: ;
		case SYS_CLOSE: ;
		case SYS_FILESIZE: ;
		case SYS_READ: ;



		};

	}


}
