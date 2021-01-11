#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define PROC_DIRNAME "myproc"   
#define PROC_FILENAME "myproc"  
#define STR_LEN  100    // 문자열을 저장하는 원형 큐 원소 하나의 길이 입니다.
#define Q_LEN   1000    // 문자열을 저장하는 원형 큐의 원소 개수 입니다.

static struct proc_dir_entry *proc_dir;   
static struct proc_dir_entry *proc_file;
char for_my_write[STR_LEN];   // my_write에서 echo > myproc   으로 user_buffer에 입력된 값을 복사하여 저장하는 커널space 메모리 입니다.
int first;                    // my_read가 한번만 실행되도록 할 때 사용되는 변수입니다. 전역변수이기 때문에 0으로 초기화 됩니다.
extern int q_index;           // blk-core.c에서 선언된 원형 큐 인덱스 입니다. 현재 코드에서는 사용되고 있지 않습니다.(시행착오로 인한 수정 과정에서 더이상 사용하지 않게 되었습니다.)
extern char cir_queue[Q_LEN][STR_LEN]; // blk-core.c에서 선언된 원형 큐 입니다. time, FS name, block_no 등을 저장하고 있습니다.

static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos)
{
   int len;
   len = (count > STR_LEN) ? STR_LEN : count;            // for_my_write 공간 보다 더 많은 값을 입력했으면 for_my_write 공간 만큼만 저장할 수 있도록 합니다.
   if (copy_from_user(for_my_write, user_buffer, len))   // user space에서 kernel space로 len만큼 memcpy 합니다.
      return -EFAULT;                                    // copy_from_user는 정상적으로 실행되면 0을 return 합니다.
   for_my_write[len] = 0x00;                             // for_my_write의 마지막에 EOF를 표시합니다.
   printk("%s count:%d\n", for_my_write, count);         // printk로 my_write에서 받아온 값을 확인할 수 있습니다. (뒤에 my_read 함수에서 for_my_write를 출력하지 않기 때문에 printk로 구현)
   printk(KERN_INFO "Simple Module Write!!\n");          // my_read가 정상적으로 실행되었습니다.
        return count;
}

static ssize_t my_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos)
{  
   // ADD MY SCY 2019.11.01
   if(first) {first=0; return 0;} // my_read가 다시 호출 되었을 때는 여기 if문에 걸리면서 first를 0으로 초기화하고 0을 return하여 my_read를 정상 종료합니다.
   if(copy_to_user(user_buffer, cir_queue[0], sizeof(char)*STR_LEN*Q_LEN)){   // kernel space에 선언된 원형 큐 전체를 user space로 memcpy합니다. 
        printk(KERN_INFO "copy_to_user error\n");                             // 만약 copy_to_user가 0을 return하지 않았다면 정상적인 실행이 되지 않은 것 이므로
        return -EFAULT;                                                       // -EFAULT를 return합니다.
   }   
        printk(KERN_INFO "Simple Module Read!!\n");   
        count = sizeof(char)*STR_LEN*Q_LEN;     // copy_to_user를 한 만큼(=count) return해야 cat myproc 명령어로 copy_to_user한 내용을 terminal에서 확인할 수 있습니다.
        first=1;                                // 두번째 my_read가 호출되면 상단의 if문에 걸릴 수 있도록 first를 1로 설정합니다.
        return count;
   // END
}

static int my_open(struct inode *inode, struct file *file)
{
   printk(KERN_INFO "Simple Module Open!!\n");
   return 0;   
}

static const struct file_operations myproc_fops = {
        .owner = THIS_MODULE,  // file_operations 구조체에세 proc_mod.c에서 새로 정의한 함수들을 bind 합니다.
        .open = my_open,
        .write = my_write,
   	  .read = my_read,
};

static int __init simple_init(void) // 모듈을 커널에 init할 때 호출되는 함수입니다.
{
   printk(KERN_INFO "Simple Module Init!!\n");
   proc_dir = proc_mkdir(PROC_DIRNAME, NULL);   // /proc 디렉토리 생성. 
   if(proc_dir == NULL){
      printk("proc_dir : error proc_mkdir\n");
      return -EEXIST;
   }
   proc_file = proc_create(PROC_FILENAME, 0600, proc_dir, &myproc_fops); // proc 파일 생성. 위에서 선언한 myproc_fops를 사용합니다.
   if(proc_file == NULL){
      printk("proc_file : error proc_create\n");
      return -EEXIST;
   }
        return 0;
}

static void __exit simple_exit(void)   // 모듈을 커널에서 종료할 때 호출되는 함수입니다.
{
        remove_proc_entry(PROC_FILENAME, proc_dir);
   remove_proc_entry(PROC_DIRNAME, NULL);
   printk(KERN_INFO "Simple Module Exit!!\n");
        return;
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_AUTHOR("Korea University");
MODULE_DESCRIPTION("It's Simple!!");
MODULE_LICENSE("GPL");
MODULE_VERSION("NEW");
