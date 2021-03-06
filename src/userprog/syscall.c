#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);

int exit_status_old;

struct opened_files{
	int fnum;
	struct file *opfile;
	struct list_elem elem;
};




void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void end_of_proc(int exit_status){

	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, exit_status);
    

    /////////////////////////////////////////////////////////////ism
	//printf("when end: %i\n", thread_tid());


    struct childs *temp = point_sp();            
  	while(temp->child_id!=thread_tid()){
    	temp = temp->next;
  	}

  	//struct childs *del =temp;
  	struct thread* parent = temp->parent;
  	parent->child_status = exit_status;
  	
  	//temp->on_delete = 1;

  	/*temp = point_sp();

  	if(temp!=del){
	  	while(temp->next!=del){
	    	temp = temp->next;
	  	}
	  	temp->next = del->next;
  	}
  	else{
  		point_new(del->next);
  	}
  	free(del);
	*/

    /////////////////////////////////////////////////////////////
  	sema_up(&(parent->thsema));
    thread_exit();

    
}

void clean_elem_of_spis(int tid){

	struct childs *del = point_sp();
	struct childs *temp;

	while(del->child_id!=tid){
   		del = del->next;
  	}

  	temp = point_sp();

  	if(temp!=del){
	  	while(temp->next!=del){
	    	temp = temp->next;
	  	}
	  	temp->next = del->next;
  	}
  	else{
  		point_new(del->next);
  	}
  	free(del);

}

void check_pntr(const void *ptr){

	if(!is_user_vaddr(ptr)){
		end_of_proc(-1);
	}

}

void check_page(const void *ptr){
	check_pntr(ptr);
	if(!pagedir_get_page(thread_current()->pagedir, ptr)) end_of_proc(-1);
}

void check_buf(void* argbuf, unsigned size){
	void* buf = argbuf;
	for(unsigned i = 0; i<size; i++, buf++) check_pntr(buf);
}

 void args_deref(int *args, int n, struct intr_frame *f){

 	int *temp;
 	while(n){
 		temp = (int *)f->esp + n;
 		check_pntr((const void *)temp);
 		args[n-1] = *temp;
 		n--;
 	}
 }

 int open_file(const char *name){
 	//lock_acquire
 	struct file *fop = filesys_open(name);//hz
 	if(fop == 0){
 		//lock_release
 		return -1;
 	};
 	struct thread *c = thread_current();
 	struct opened_files *of = malloc(sizeof(struct opened_files));
 	of->opfile = fop;
 	of->fnum = c->fnum++;
 	list_push_back(&c->files, &of->elem);
 	//lock_release
 	return of->fnum;
 }




static void
syscall_handler (struct intr_frame *f) 
{
	check_pntr((const void*)f->esp);
	int stat = *(int*)f->esp;
	int args[3];

	

	switch(stat){

		case SYS_HALT: {
			shutdown_power_off(); 
			break;
		};

		case SYS_EXIT: {
			args_deref(args, 1, f);
			end_of_proc(args[0]);
		};

		case SYS_EXEC:{		
			args_deref(args, 1, f);
			check_page((const void*)args[0]);
			
			int tid = process_execute (args[0]);

		  	if(thread_current()->child_load_status == 1){
		  		f->eax = tid;
		  	}
		  	if(thread_current()->child_load_status == 0) {
		  		f->eax = -1;
		  	}
			break;
			
		};

		case SYS_WAIT:{
			args_deref(args, 1, f);

			if(args[0] > 100000 || args[0] <1){
				f->eax = -1;
				break;
			}

			struct childs *temp = point_sp();       
 			while(temp!=NULL){
 				if (temp->child_id == args[0]){
 					if(temp->on_wait==1){
 					f->eax = -1;
					break;
					}
					else break;
 				}
   				temp = temp->next;
  			}

  			if(f->eax == -1) break;

  			temp->on_wait=1;

			f->eax = process_wait(args[0]);

			//clean_elem_of_spis(args[0]);	//попробовать через if(temp->on_delete)

			break;
		};

		case SYS_CREATE:{
			args_deref(args, 2, f);
			check_page((const void*)args[0]);
			//lock_acquire
			f->eax = filesys_create((const char *)args[0], (unsigned) args[1]);
			//lock_release
			break;
		};

		case SYS_REMOVE:{
			args_deref(args, 1, f);
			check_page((const void*)args[0]);
			//lock_acquire
			f->eax = filesys_remove((const char *)args[0], (unsigned) args[1]);
			//lock_release
			break;
		}; 
		case SYS_OPEN:{
			args_deref(args, 1, f);
			check_page((const void*)args[0]);
			//lock_acquire
			f->eax = open_file((const char *)args[0]);
			//lock_release
			break;

		};

		case SYS_CLOSE:{
			args_deref(args, 1, f);
			//lock_acquire

			if(args[0]<=1) end_of_proc(-1);
			struct list_elem *head;
			struct list_elem *tail = list_end(&thread_current()->files);
			struct opened_files *tmp;
			int i=0;
			for(head=list_begin(&thread_current()->files); head!=tail; head = list_next(head)){
				if(list_entry(head, struct opened_files, elem)->fnum==args[0]){i=1; break;}
			}
			if(i){
				tmp = list_entry(head, struct opened_files, elem);
			}
			else return end_of_proc(-1); 
			file_close(tmp->opfile);
			list_remove(&tmp->elem);
			free(tmp);

			//lock_release
			break;
		};

		case SYS_FILESIZE:{
			args_deref(args, 1, f);
			//lock_acquire
			struct list_elem *head;
			struct list_elem *tail = list_end(&thread_current()->files);
			struct opened_files *tmp;
			int i=0;
			for(head=list_begin(&thread_current()->files); head!=tail; head = list_next(head)){
				if(list_entry(head, struct opened_files, elem)->fnum==args[0]){i=1; break;}
			}
			if(i){
				tmp = list_entry(head, struct opened_files, elem);
				if (tmp == 0) i--;
			}
			if(i) f->eax = file_length(tmp->opfile);
			else f->eax = -1;
			//lock_release
			break;
		};
		case SYS_READ:{

			args_deref(args, 3, f);
			check_page((const void*)args[1]);
			check_buf((void *) args[1], (unsigned) args[2]);
			
			if(args[0] == 0){
				unsigned* buf = (unsigned*) args[1];//hz
				for(unsigned i=0; i<args[2]; i++) buf[i] = input_getc();
				f->eax = args[2];
				return;
			}

			//lock_acquire
			struct list_elem *head;
			struct list_elem *tail = list_end(&thread_current()->files);
			struct opened_files *tmp;
			int i=0;
			for(head=list_begin(&thread_current()->files); head!=tail; head = list_next(head)){
				if(list_entry(head, struct opened_files, elem)->fnum==args[0]){i=1; break;}
			}
			if(i){
				tmp = list_entry(head, struct opened_files, elem);
				if (tmp == 0) i--;
			}

			if(i) f->eax = file_read(tmp->opfile, args[1], args[2]);
			else f->eax = -1;
			//lock_release
			break;
		};

		case SYS_WRITE: {
			args_deref(args, 3, f);
			check_page((const void*)args[1]);
			check_buf((void *) args[1], (unsigned) args[2]);

			if(args[0] == 1){
				putbuf( args[1], args[2]);
				f->eax = args[2];
				return;
			}

			//lock_acquire

			struct list_elem *head;
			struct list_elem *tail = list_end(&thread_current()->files);
			struct opened_files *tmp;
			int i=0;
			for(head=list_begin(&thread_current()->files); head!=tail; head = list_next(head)){
				if(list_entry(head, struct opened_files, elem)->fnum==args[0]){i=1; break;}
			}
			if(i){
				tmp = list_entry(head, struct opened_files, elem);
				if (tmp == 0) i--;
			}

			if(i) f->eax = file_write(tmp->opfile, args[1], args[2]);
			else f->eax = -1;

			//lock_realise
			break;
		};
	}
}
