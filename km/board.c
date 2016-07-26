
/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
//#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/param.h> /* include HZ */
#include <linux/string.h> /* string operations */
#include <linux/timer.h> /* timer gizmos */
#include <linux/list.h> /* include list data struct */
#include <linux/interrupt.h>
#include <linux/random.h>


MODULE_LICENSE("MIT");
MODULE_AUTHOR("Naved Ansari");    
MODULE_DESCRIPTION("Fake OpenBCI board device");  
MODULE_VERSION("1.0");  

/* Declaration of memory.c functions */
static int board_open(struct inode *inode, struct file *filp);
static int board_release(struct inode *inode, struct file *filp);
static ssize_t board_read(struct file *filp,char *buf, size_t count, loff_t *f_pos);
static ssize_t board_write(struct file *filp,const char *buf, size_t count, loff_t *f_pos);
static void board_exit(void);
static int board_init(void);
static void timer_handler(unsigned long data);

volatile int packetno=0; //to count no. of packets

/* Structure that declares the usual file */
/* access functions */
struct file_operations board_fops = {
	read: board_read,
	write: board_write,
	open: board_open,
	release: board_release
};

/* Declaration of the init and exit functions */
module_init(board_init);
module_exit(board_exit);

static unsigned capacity = 256; //buffer capacity
module_param(capacity, uint, S_IRUGO);

/* Global variables of the driver */
/* Major number */
static int board_major = 61;

/* Buffer to store data */
static char *board_buffer;
/* length of the current message */
static int board_len;

/* timers, not curently used */
static struct timer_list the_timer[1];
int timePer = 500;

char command = 's'; //command that is sent to boards


/* the init function is called once, when the module is inserted */
static int board_init(void) 
{
	int result, ret;
	/* Registering device */
	result = register_chrdev(board_major, "board", &board_fops);
	if (result < 0)
	{
		printk(KERN_ALERT "board: cannot obtain major number %d\n", board_major);
		return result;
	}
	/* Allocating board for the buffer */
	board_buffer = kmalloc(capacity, GFP_KERNEL);
	/* Check if all right */
	if (!board_buffer)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	} 

	memset(board_buffer, 0, capacity); //init buffer to
	board_len = 0;
	printk(KERN_EMERG "Inserting board module\n"); 

	//set up timers, not currently used
	setup_timer(&the_timer[0], timer_handler, 0);
	ret = mod_timer(&the_timer[0], jiffies + msecs_to_jiffies(timePer/2));

	return 0;
	fail: 
	board_exit(); 
	return result;
}

/*exit function is called when module is rmmod */
static void board_exit(void)
{
	/* Freeing the major number */
	unregister_chrdev(board_major, "board");
	/* Freeing buffer memory */
	if (board_buffer)
		kfree(board_buffer);
	/* Freeing timer list */
	//if (the_timer)
	//	kfree(the_timer);
	/* removing timer stuff, currently not used*/
	if (timer_pending(&the_timer[0]))
		del_timer(&the_timer[0]);
	printk(KERN_ALERT "Removing board module\n");

}
/*i still dont know what this does, apart printgint pid */
static int board_open(struct inode *inode, struct file *filp)
{
	/*printk(KERN_INFO "open: process id %d, command %s\n",
		current->pid, current->comm);*/
	/* Success */
	return 0;
}
/*don't know what this does either */
static int board_release(struct inode *inode, struct file *filp)
{
	/*printk(KERN_INFO "release called: process id %d, command %s\n",
		current->pid, current->comm);*/
	/* Success */
	return 0;
}

/*whenever someone writes to the board, this is called */
static ssize_t board_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	
	if (copy_from_user(board_buffer, buf, 1)) //copy 1 character from the user
	{
		return -EFAULT;
	}
    
    /*see what that character is and set the command*/
    if(board_buffer[0]== 'b')
    	command = 'b'; //stream
    else if(board_buffer[0] == 'v')
    	command = 'v'; //reset
    else if (board_buffer[0]=='s')
    {
    	command = 's'; //stop
    }
    else
    	command = 'u'; //unknown

    return count;
}

/*this function is called, whenever something reads from the device */
static ssize_t board_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{ 
	char tbuf[33]; //the 33 byte packet that the board will send
	char rand; //random byte
	int i; //counter variable
	printk(KERN_EMERG "\nIn board_read, command = %c", command); //just for debugging
	
	if(command == 'b')
	{
		//sprintf(tbuf,"\nI will stream data now");
		packetno++; //increment no. of packets
		//start streaming data, make the packet and then put it on tbuf
		//make fake accel data here. 
		/*float accelArray[3] = {0};
		accelArray[0] = (randomnum(time(NULL)) * 0.1 * (randomnum(time(NULL)) > 0.5 ? -1 : 1)); //x
		accelArray[1] = (randomnum(time(NULL)+2) * 0.1 * (randomnum(time(NULL)+3) > 0.5 ? -1 : 1)); //y
		accelArray[2] = (randomnum(time(NULL)) * 0.4 * (randomnum(time(NULL)) > 0.5 ? -1 : 1)); //z */
	
		printk(KERN_EMERG "\nIn kernel, starting stream, packet %d", packetno); 
		tbuf[0] = 0xA0; //init the 1st byte
		for(i=1; i<32; i++)
		{
			//tbuf[i] = 0x23;
			get_random_bytes(&rand, sizeof(rand)); //init the bytes randomly
			tbuf[i] = rand;
		}
		tbuf[32]= 0xC0; //init the last byte
	}
	else if(command == 'v')
	{
		sprintf(tbuf,"\nBoard has been reset"); 
		//handle reset
	}
	else if (command == 's')
	{
		sprintf(tbuf,"\n Streaming stopped");
		//handle stop
	}
	else
	{
		sprintf(tbuf, "\nUnknown command");
		//handle unkown condition
	}

	strcpy(board_buffer,tbuf); //put whatever we need into the board buffer

	//do not go over then end
	if (count > 256 - *f_pos)
		count = 256 - *f_pos;

	if (copy_to_user(buf,board_buffer, sizeof(tbuf))) //copy the buffer to user space
	{
		printk(KERN_ALERT "Couldn't copy to user\n"); 
		return -EFAULT;	
	}
	
	// Changing reading position as best suits 
	*f_pos += count; 
	return count; 
}

/*function that sets up the timer */
static void timer_handler(unsigned long data) 
{
	//call somefuntion to do something when timer expires
	mod_timer(&the_timer[0],jiffies + msecs_to_jiffies(timePer/2)); //reset the timer

}	






